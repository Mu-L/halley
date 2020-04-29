#include "audio_facade.h"
#include "audio_engine.h"
#include "audio_handle_impl.h"
#include "audio_voice_behaviour.h"
#include "halley/support/console.h"
#include "halley/support/logger.h"
#include "halley/core/resources/resources.h"
#include "audio_event.h"

using namespace Halley;

AudioFacade::AudioFacade(AudioOutputAPI& o, SystemAPI& system)
	: output(o)
	, system(system)
	, running(false)
	, started(false)
	, commandQueue(256)
	, exceptions(16)
	, playingSoundsQueue(4)
	, ownAudioThread(o.needsAudioThread())
{
}

AudioFacade::~AudioFacade()
{
	AudioFacade::stopPlayback();
}

void AudioFacade::setResources(Resources& res)
{
	resources = &res;
}

void AudioFacade::init()
{
	Logger::logInfo("\nInitializing audio...");
	Logger::logInfo("Audio devices available:");
	int i = 0;
	for (auto& device: getAudioDevices()) {
		Logger::logInfo("\t" + toString(i++) + ": " + device->getName());
	}
}

void AudioFacade::deInit()
{
	stopPlayback();
}

Vector<std::unique_ptr<const AudioDevice>> AudioFacade::getAudioDevices()
{
	return output.getAudioDevices();
}

void AudioFacade::startPlayback(int deviceNumber)
{
	doStartPlayback(deviceNumber, true);
}

void AudioFacade::doStartPlayback(int deviceNumber, bool createEngine)
{
	if (createEngine && started) {
		stopPlayback();
	}

	auto devices = getAudioDevices();
	if (int(devices.size()) > deviceNumber) {
		if (createEngine) {
			engine = std::make_unique<AudioEngine>();
		}

		AudioSpec format;
		format.bufferSize = 512;
		format.format = AudioSampleFormat::Float;
		format.numChannels = 2;
		format.sampleRate = 48000;

		try {
			audioSpec = output.openAudioDevice(format, devices.at(deviceNumber).get(), [this]() { onNeedBuffer(); });
			started = true;
			lastDeviceNumber = deviceNumber;

			Logger::logInfo("Audio Playback started.");
			Logger::logInfo("\tDevice: " + devices.at(deviceNumber)->getName() + " [" + toString(deviceNumber) + "]");
			Logger::logInfo("\tSample rate: " + toString(audioSpec.sampleRate));
			Logger::logInfo("\tChannels: " + toString(audioSpec.numChannels));
			Logger::logInfo("\tFormat: " + toString(audioSpec.format));
			Logger::logInfo("\tBuffer size: " + toString(audioSpec.bufferSize));

			resumePlayback();
		} catch (...) {
			// Unable to open audio device
		}
	}
}

void AudioFacade::stopPlayback()
{
	if (started) {
		pausePlayback();
		musicTracks.clear();
		engine.reset();
		output.closeAudioDevice();
		started = false;
	}
}

void AudioFacade::resumePlayback()
{
	if (started) {
		if (running) {
			pausePlayback();
		}

		engine->start(audioSpec, output);
		running = true;

		if (ownAudioThread) {
			audioThread = system.createThread("Audio", ThreadPriority::High, [this]() { run(); });
		}

		output.startPlayback();
	}
}

void AudioFacade::pausePlayback()
{
	if (running) {
		{
			running = false;
			engine->pause();
		}
		if (ownAudioThread) {
			audioThread.join();
			audioThread = {};
		}
		output.stopPlayback();
	}
}

void AudioFacade::onSuspend()
{
	pausePlayback();
}

void AudioFacade::onResume()
{
	doStartPlayback(lastDeviceNumber, false);
}

AudioHandle AudioFacade::postEvent(const String& name, AudioPosition position)
{
	if (!resources->exists<AudioEvent>(name)) {
		Logger::logWarning("Unknown audio event: \"" + name + "\"");
		uint32_t id = uniqueId++;
		return std::make_shared<AudioHandleImpl>(*this, id);
	}

	auto event = resources->get<AudioEvent>(name);
	event->loadDependencies(*resources);

	uint32_t id = uniqueId++;
	enqueue([=] () {
		engine->postEvent(id, *event, position);
	});
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::playMusic(const String& eventName, int track, float fadeInTime)
{
	bool hasFade = fadeInTime > 0.0001f;
	
	stopMusic(track, 0.5f);
	auto handle = postEvent(eventName, AudioPosition::makeFixed());
	musicTracks[track] = handle;

	if (hasFade) {
		handle->setGain(0.0f);
		handle->setBehaviour(std::make_unique<AudioVoiceFadeBehaviour>(fadeInTime, 1.0f, false));
	}

	return handle;
}

AudioHandle AudioFacade::play(std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop)
{
	uint32_t id = uniqueId++;
	enqueue([=] () {
		engine->play(id, clip, position, volume, loop);
	});
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::getMusic(int track)
{
	auto iter = musicTracks.find(track);
	if (iter != musicTracks.end()) {
		return iter->second;
	} else {
		return AudioHandle();
	}
}

void AudioFacade::stopMusic(int track, float fadeOutTime)
{
	auto iter = musicTracks.find(track);
	if (iter != musicTracks.end()) {
		stopMusic(iter->second, fadeOutTime);
		musicTracks.erase(iter);
	}
}

void AudioFacade::stopAllMusic(float fadeOutTime)
{
	for (auto& m: musicTracks) {
		stopMusic(m.second, fadeOutTime);
	}
	musicTracks.clear();
}

void AudioFacade::setMasterVolume(float volume)
{
	enqueue([=] () {
		engine->setMasterGain(volumeToGain(volume));
	});
}

void AudioFacade::setGroupVolume(const String& groupName, float volume)
{
	enqueue([=] () {
		engine->setGroupGain(groupName, volumeToGain(volume));
	});
}

void AudioFacade::setOutputChannels(std::vector<AudioChannelData> audioChannelData)
{
	enqueue([=, audioChannelData = std::move(audioChannelData)] () mutable
	{
		engine->setOutputChannels(std::move(audioChannelData));
	});
}

void AudioFacade::stopMusic(AudioHandle& handle, float fadeOutTime)
{
	if (fadeOutTime > 0.001f) {
		handle->setBehaviour(std::make_unique<AudioVoiceFadeBehaviour>(fadeOutTime, 0.0f, true));
	} else {
		handle->stop();
	}
}

void AudioFacade::onNeedBuffer()
{
	if (!ownAudioThread) {
		stepAudio();
	}
}

void AudioFacade::setListener(AudioListenerData listener)
{
	enqueue([=] () {
		engine->setListener(listener);
	});
}

void AudioFacade::onAudioException(std::exception& e)
{
	if (exceptions.canWrite(1)) {
		exceptions.writeOne(e.what());
	}
}

void AudioFacade::run()
{
	while (running) {
		stepAudio();
	}
}

void AudioFacade::stepAudio()
{
	try {
		{
			if (!running) {
				return;
			}
			if (playingSoundsQueue.canWrite(1)) {
				playingSoundsQueue.writeOne(engine->getPlayingSounds());
			}
		}

		const size_t nToRead = commandQueue.availableToRead();
		inbox.resize(nToRead);
		commandQueue.read(gsl::span<std::function<void()>>(inbox.data(), nToRead));
		for (auto& action : inbox) {
			action();
		}

		if (ownAudioThread) {
			engine->run();
		} else {
			engine->generateBuffer();
		}
	} catch (std::exception& e) {
		onAudioException(e);
	}
}

void AudioFacade::enqueue(std::function<void()> action)
{
	if (running) {
		if (commandQueue.canWrite(1)) {
			commandQueue.writeOne(std::move(action));
		} else {
			Logger::logError("Out of space on audio command queue.");
		}
	}
}

void AudioFacade::pump()
{
	if (!exceptions.empty()) {
		String e;
		while (!exceptions.empty()) {
			e = exceptions.readOne();
			Logger::logError(e);
		}
		stopPlayback();
		throw Exception(e, HalleyExceptions::AudioEngine);
	}

	if (running) {
		while (playingSoundsQueue.canRead(1)) {
			playingSounds = playingSoundsQueue.readOne();
		}
	}
}

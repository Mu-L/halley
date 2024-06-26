#include "halley/audio/audio_buffer.h"

using namespace Halley;

AudioBuffer::AudioBuffer(size_t size)
	: samples(size)
{
}

AudioBufferRef::AudioBufferRef()
	: buffer(nullptr)
	, pool(nullptr)
{	
}

AudioBufferRef::AudioBufferRef(AudioBuffer& buffer, AudioBufferPool& pool)
	: buffer(&buffer)
	, pool(&pool)
{
}

AudioBufferRef::AudioBufferRef(AudioBufferRef&& other) noexcept
	: buffer(other.buffer)
	, pool(other.pool)
{
	other.buffer = nullptr;
	other.pool = nullptr;
}

AudioBufferRef& AudioBufferRef::operator=(AudioBufferRef&& other) noexcept
{
	clear();
	buffer = other.buffer;
	pool = other.pool;
	other.buffer = nullptr;
	other.pool = nullptr;
	return *this;
}

AudioBufferRef::~AudioBufferRef()
{
	clear();
}

AudioBuffer& AudioBufferRef::getBuffer() const
{
	return *buffer;
}

AudioSamples AudioBufferRef::getSpan() const
{
	return AudioSamples(buffer->samples);
}

void AudioBufferRef::clear()
{
	if (buffer && pool) {
		pool->returnBuffer(*buffer);
		buffer = nullptr;
	}
}

AudioBuffersRef::AudioBuffersRef()
	: nBuffers(0)
	, pool(nullptr)
{
}

AudioBuffersRef::AudioBuffersRef(size_t n, std::array<AudioBuffer*, AudioConfig::maxChannels> buffers, AudioBufferPool& pool)
	: buffers(buffers)
	, nBuffers(n)
	, pool(&pool)
{
	for (size_t i = 0; i < n; ++i) {
		spans[i] = buffers[i]->samples;
		sampleSpans[i] = AudioSamples(spans[i].data(), spans[i].size());
	}
}

AudioBuffersRef::AudioBuffersRef(AudioBuffersRef&& other) noexcept
{
	*this = std::move(other);
}

AudioBuffersRef& AudioBuffersRef::operator=(AudioBuffersRef&& other) noexcept
{
	clear();

	buffers = other.buffers;
	nBuffers = other.nBuffers;
	pool = other.pool;

	other.nBuffers = 0;
	other.pool = nullptr;

	for (size_t i = 0; i < nBuffers; ++i) {
		spans[i] = buffers[i]->samples;
		sampleSpans[i] = AudioSamples(spans[i].data(), spans[i].size());
	}

	return *this;
}

AudioBuffersRef::~AudioBuffersRef()
{
	clear();
}

gsl::span<AudioBuffer*> AudioBuffersRef::getBuffers()
{
	return gsl::span<AudioBuffer*>(buffers.data(), nBuffers);
}

AudioMultiChannelSamples AudioBuffersRef::getSpans() const
{
	return spans;
}

AudioMultiChannelSamples AudioBuffersRef::getSampleSpans() const
{
	return sampleSpans;
}

AudioBuffer& AudioBuffersRef::operator[](size_t n) const
{
	return *buffers[n];
}

bool AudioBuffersRef::matches(size_t n, size_t len) const
{
	return nBuffers == n && (nBuffers == 0 || buffers[0]->samples.size() == len);
}

void AudioBuffersRef::clear()
{
	if (pool) {
		for (size_t i = 0; i < nBuffers; ++i) {
			pool->returnBuffer(*buffers[i]);
		}
	}
	pool = nullptr;
	nBuffers = 0;
}

AudioBufferRef AudioBufferPool::getBuffer(size_t numSamples)
{
	return AudioBufferRef(allocBuffer(numSamples), *this);
}

AudioBuffersRef AudioBufferPool::getBuffers(size_t n, size_t numSamples)
{
	Expects(n <= AudioConfig::maxChannels);
	std::array<AudioBuffer*, AudioConfig::maxChannels> buffers;
	for (size_t i = 0; i < n; ++i) {
		buffers[i] = &allocBuffer(numSamples);
	}
	return AudioBuffersRef(n, buffers, *this);
}

AudioBuffer& AudioBufferPool::allocBuffer(size_t numSamples)
{
	Expects(numSamples < 65536);

	const size_t idx = fastLog2Ceil(std::max(static_cast<uint32_t>(16), static_cast<uint32_t>(numSamples)));
	auto& buffers = buffersTable[idx];

	if (buffers.available.empty()) {
		// No free buffers, create new one
		auto& result = *buffers.entries.emplace_back(std::make_unique<AudioBuffer>(static_cast<size_t>(1LL << idx)));
		result.samples.resize(numSamples);
		return result;
	} else {
		// Return latest free buffer
		auto* ptr = buffers.available.back();
		buffers.available.pop_back();
		ptr->samples.resize(numSamples);
		return *ptr;
	}
}

void AudioBufferPool::returnBuffer(AudioBuffer& buffer)
{
	const size_t idx = fastLog2Ceil(uint32_t(buffer.samples.size()));
	auto& buffers = buffersTable[idx];

	buffers.available.push_back(&buffer);
}

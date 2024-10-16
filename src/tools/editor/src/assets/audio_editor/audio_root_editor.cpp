#include "audio_root_editor.h"

#include "audio_object_editor.h"
#include "halley/audio/audio_object.h"
#include "halley/properties/audio_properties.h"
using namespace Halley;

AudioRootEditor::AudioRootEditor(UIFactory& factory, AudioObjectEditor& editor, AudioObject& object)
	: UIWidget("audio_root_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, object(object)
{
	factory.loadUI(*this, "halley/audio_editor/audio_root_editor");
}

void AudioRootEditor::onMakeUI()
{
	getWidgetAs<UIDropdown>("bus")->setOptions(editor.getAudioProperties().getBusIds());

	bindData("bus", object.getBus(), [=] (String value)
	{
		object.setBus(std::move(value));
		editor.markModified(false);
	});

	bindData("gainMin", object.getGain().start, [=] (float value)
	{
		object.getGain().start = value;
		editor.markModified(false);
	});

	bindData("gainMax", object.getGain().end, [=] (float value)
	{
		object.getGain().end = value;
		editor.markModified(false);
	});

	bindData("pitchMin", object.getPitch().start, [=] (float value)
	{
		object.getPitch().start = value;
		editor.markModified(false);
	});

	bindData("pitchMax", object.getPitch().end, [=] (float value)
	{
		object.getPitch().end = value;
		editor.markModified(false);
	});

	bindData("dopplerScale", object.getDopplerScale(), [=] (float value)
	{
		object.setDopplerScale(value);
		editor.markModified(false);
	});

	auto bindAttenuationParams = [this] (const AudioAttenuation& attenuation)
	{
		bindData("refDistance", attenuation.referenceDistance, [this] (float value)
		{
			object.getMutableAttenuationOverride().referenceDistance = value;
			editor.markModified(false);
		});

		bindData("maxDistance", attenuation.maximumDistance, [this] (float value)
		{
			object.getMutableAttenuationOverride().maximumDistance = value;
			editor.markModified(false);
		});

		bindData("rollOffFactor", attenuation.rollOffFactor, [this] (float value)
		{
			object.getMutableAttenuationOverride().rollOffFactor = value;
			editor.markModified(false);
		});

		bindData("attenuationCurve", toString(attenuation.curve), [this] (String value)
		{
			object.getMutableAttenuationOverride().curve = fromString<AudioAttenuationCurve>(value);
			editor.markModified(false);
		});
	};

	if (object.getAttenuationOverride().has_value()) {
		getWidget("attenuationContents")->setActive(true);
		bindAttenuationParams(*object.getAttenuationOverride());
	}

	bindData("overrideAttenuation", object.getAttenuationOverride().has_value(), [this, bindAttenuationParams] (bool value)
	{
		getWidget("attenuationContents")->setActive(value);

		if (value) {
			object.setAttenuationOverride(AudioAttenuation());
			bindAttenuationParams(*object.getAttenuationOverride());
		} else {
			object.setAttenuationOverride(std::nullopt);
		}
		editor.markModified(false);
	});

	bindData("pruneDistant", object.getPruneDistant(), [this] (bool value)
	{
		object.setPruneDistant(value);
		editor.markModified(false);
	});

	auto bindCooldown = [this] ()
	{
		const bool hasValue = object.getCooldown().has_value();
		getWidget("cooldownContent")->setActive(hasValue);

		if (hasValue) {
			bindData("cooldown", *object.getCooldown(), [this] (float value)
			{
				object.setCooldown(value);
				editor.markModified(false);
			});
		}
	};
	bindCooldown();

	auto bindMaxInstances = [this] ()
	{
		const bool hasValue = object.getMaxInstances().has_value();
		getWidget("maxInstances")->setActive(hasValue);

		if (hasValue) {
			bindData("maxInstances", *object.getMaxInstances(), [this] (int value)
			{
				object.setMaxInstances(value);
				editor.markModified(false);
			});
		}
	};
	bindMaxInstances();

	bindData("enableCooldown", object.getCooldown().has_value(), [this, bindCooldown] (bool value)
	{
		object.setCooldown(value ? std::optional<float>(0.0f) : std::nullopt);
		bindCooldown();
		editor.markModified(false);
	});

	bindData("enableMaxInstances", object.getMaxInstances().has_value(), [this, bindMaxInstances] (bool value)
	{
		object.setMaxInstances(value ? std::optional<int>(0) : std::nullopt);
		bindMaxInstances();
		editor.markModified(false);
	});
}

#include "entity_editor_factories.h"
#include "halley/core/graphics/sprite/animation.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_validator.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/ui/select_asset_widget.h"
#include "halley/core/scene_editor/component_field_parameters.h"
#include "halley/core/scene_editor/component_editor_context.h"

using namespace Halley;

class ComponentEditorTextFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::String";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		String value = data.getFieldData().asString("");

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "textValue", context.getFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		field->bindData("textValue", value, [&, data](String newVal)
		{
			data.getFieldData() = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		return field;
	}
};

class ComponentEditorIntFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "int";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		int value = data.getFieldData().asInt(defaultValue.isInteger() ? defaultValue.toInteger() : 0);

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "intValue", context.getFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, false));
		field->bindData("intValue", value, [&, data](int newVal)
		{
			data.getFieldData() = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		return field;
	}
};

class ComponentEditorFloatFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "float";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		const float value = data.getFieldData().asFloat(defaultValue.isNumber() ? defaultValue.toFloat() : 0.0f);

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "floatValue", context.getFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, true));
		field->bindData("floatValue", value, [&, data](float newVal)
		{
			data.getFieldData() = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		return field;
	}
};

class ComponentEditorAngle1fFieldFactory : public ComponentEditorFloatFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Angle1f";
	}
};

class ComponentEditorBoolFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "bool";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		bool value = data.getFieldData().asBool(defaultValue == "true");

		auto field = std::make_shared<UICheckbox>("boolValue", context.getFactory().getStyle("checkbox"), value);
		field->bindData("boolValue", value, [&, data](bool newVal)
		{
			data.getFieldData() = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		auto sizer = std::make_shared<UISizer>(UISizerType::Horizontal, 4.0f);
		sizer->add(field);

		return sizer;
	}
};

class ComponentEditorVector2fFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Vector2f";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		Vector2f value;
		if (data.getFieldData().getType() != ConfigNodeType::Undefined) {
			value = data.getFieldData().asVector2f();
		}

		const auto& keyboard = context.getFactory().getKeyboard();
		const auto& style = context.getFactory().getStyle("inputThin");

		auto dataOutput = std::make_shared<bool>(true);
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));

		container->add(std::make_shared<UITextInput>(keyboard, "xValue", style, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		container->bindData("xValue", value.x, [&, data, dataOutput] (float newVal)
		{
			if (*dataOutput) {
				auto& node = data.getFieldData();
				node = ConfigNode(Vector2f(newVal, node.asVector2f(Vector2f()).y));
				context.onEntityUpdated();
			}
		});

		container->add(std::make_shared<UITextInput>(keyboard, "yValue", style, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		container->bindData("yValue", value.y, [&, data, dataOutput](float newVal)
		{
			if (*dataOutput) {
				auto& node = data.getFieldData();
				node = ConfigNode(Vector2f(node.asVector2f(Vector2f()).x, newVal));
				context.onEntityUpdated();
			}
		});

		container->setHandle(UIEventType::ReloadData, pars.componentName + ":" + data.getName(), [=] (const UIEvent& event)
		{
			Vector2f newVal;
			if (data.getFieldData().getType() != ConfigNodeType::Undefined) {
				newVal = data.getFieldData().asVector2f();
			}
			*dataOutput = false;
			event.getCurWidget().getWidgetAs<UITextInput>("xValue")->setText(toString(newVal.x));
			event.getCurWidget().getWidgetAs<UITextInput>("yValue")->setText(toString(newVal.y));
			*dataOutput = true;
		});

		return container;
	}
};

class ComponentEditorSpriteFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Sprite";
	}

	bool isCompound() const override
	{
		return true;
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		auto& fieldData = data.getFieldData();
		if (fieldData.getType() == ConfigNodeType::Undefined) {
			fieldData = ConfigNode::MapType();
		}

		const auto& keyboard = context.getFactory().getKeyboard();
		const auto& inputStyle = context.getFactory().getStyle("inputThin");
		const auto& checkStyle = context.getFactory().getStyle("checkbox");

		auto pivotContainer = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));
		pivotContainer->add(std::make_shared<UITextInput>(keyboard, "pivotX", inputStyle, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		pivotContainer->add(std::make_shared<UITextInput>(keyboard, "pivotY", inputStyle, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("image"));
		container->add(std::make_shared<SelectAssetWidget>("image", context.getFactory(), AssetType::Sprite, context.getGameResources()));
		container->add(context.makeLabel("material"));
		container->add(std::make_shared<SelectAssetWidget>("material", context.getFactory(), AssetType::MaterialDefinition, context.getGameResources()));
		container->add(context.makeLabel("colour"));
		container->add(std::make_shared<UITextInput>(keyboard, "colour", inputStyle));
		container->add(context.makeLabel("pivot"));
		container->add(pivotContainer);
		container->add(context.makeLabel("flip"));
		container->add(std::make_shared<UICheckbox>("flip", checkStyle), 0, {}, UISizerAlignFlags::Left);
		container->add(context.makeLabel("visible"));
		container->add(std::make_shared<UICheckbox>("visible", checkStyle), 0, {}, UISizerAlignFlags::Left);

		container->bindData("image", fieldData["image"].asString(""), [&, data](String newVal)
		{
			data.getFieldData()["image"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("material", fieldData["material"].asString(""), [&, data](String newVal)
		{
			data.getFieldData()["material"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("colour", fieldData["colour"].asString("#FFFFFF"), [&, data](String newVal)
		{
			data.getFieldData()["colour"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("pivotX", fieldData["pivot"].asVector2f(Vector2f()).x, [&, data] (float newVal)
		{
			auto& node = data.getFieldData()["pivot"];
			node = ConfigNode(Vector2f(newVal, node.asVector2f(Vector2f()).y));
			context.onEntityUpdated();
		});

		container->bindData("pivotY", fieldData["pivot"].asVector2f(Vector2f()).y, [&, data] (float newVal)
		{
			auto& node = data.getFieldData()["pivot"];
			node = ConfigNode(Vector2f(node.asVector2f(Vector2f()).x, newVal));
			context.onEntityUpdated();
		});

		container->bindData("flip", fieldData["flip"].asBool(false), [&, data](bool newVal)
		{
			data.getFieldData()["flip"] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		container->bindData("visible", fieldData["visible"].asBool(true), [&, data](bool newVal)
		{
			data.getFieldData()["visible"] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		return container;
	}
};

class ComponentEditorAnimationPlayerFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::AnimationPlayer";
	}

	bool isCompound() const override
	{
		return true;
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		auto& fieldData = data.getFieldData();
		if (fieldData.getType() == ConfigNodeType::Undefined) {
			fieldData = ConfigNode::MapType();
		}

		auto& resources = context.getGameResources();
		const auto& keyboard = context.getFactory().getKeyboard();
		const auto& inputStyle = context.getFactory().getStyle("inputThin");
		const auto& checkStyle = context.getFactory().getStyle("checkbox");
		const auto& dropStyle = context.getFactory().getStyle("dropdown");
		const auto& scrollStyle = context.getFactory().getStyle("scrollbar");
		const auto& listStyle = context.getFactory().getStyle("list");

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("animation"));
		container->add(std::make_shared<SelectAssetWidget>("animation", context.getFactory(), AssetType::Animation, context.getGameResources()));
		container->add(context.makeLabel("sequence"));
		container->add(std::make_shared<UIDropdown>("sequence", dropStyle, scrollStyle, listStyle));
		container->add(context.makeLabel("direction"));
		container->add(std::make_shared<UIDropdown>("direction", dropStyle, scrollStyle, listStyle));
		container->add(context.makeLabel("playbackSpeed"));
		container->add(std::make_shared<UITextInput>(keyboard, "playbackSpeed", inputStyle, "", LocalisedString(), std::make_shared<UINumericValidator>(false, true)));
		container->add(context.makeLabel("applyPivot"));
		container->add(std::make_shared<UICheckbox>("applyPivot", checkStyle), 0, {}, UISizerAlignFlags::Left);

		auto updateAnimation = [container, data, &resources] (const String& animName)
		{
			std::vector<String> sequences;
			std::vector<String> directions;

			if (!animName.isEmpty()) {
				const auto anim = resources.get<Animation>(animName);
				directions = anim->getDirectionNames();
				sequences = anim->getSequenceNames();
			}

			auto sequence = container->getWidgetAs<UIDropdown>("sequence");
			auto direction = container->getWidgetAs<UIDropdown>("direction");
			sequence->setOptions(sequences);
			direction->setOptions(directions);
			sequence->setSelectedOption(data.getFieldData()["sequence"].asString(""));
			direction->setSelectedOption(data.getFieldData()["direction"].asString(""));
		};
		updateAnimation(fieldData["animation"].asString(""));

		container->bindData("animation", fieldData["animation"].asString(""), [&, data, updateAnimation](String newVal)
		{
			updateAnimation(newVal);
			data.getFieldData()["animation"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("sequence", fieldData["sequence"].asString(""), [&, data](String newVal)
		{
			data.getFieldData()["sequence"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("direction", fieldData["direction"].asString(""), [&, data](String newVal)
		{
			data.getFieldData()["direction"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("playbackSpeed", fieldData["playbackSpeed"].asFloat(1.0f), [&, data](float newVal)
		{
			data.getFieldData()["playbackSpeed"] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		container->bindData("applyPivot", fieldData["applyPivot"].asBool(true), [&, data](bool newVal)
		{
			data.getFieldData()["applyPivot"] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		return container;
	}
};

class ComponentEditorPolygonFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Polygon";
	}

	virtual bool isOpenPolygon()
	{
		return false;
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;
		auto componentNames = pars.componentNames;
		auto componentName = pars.componentName;

		auto style = context.getFactory().getStyle("buttonThin");

		auto field = std::make_shared<UIButton>("editPolygon", style, LocalisedString::fromHardcodedString("Edit..."));
		field->setMinSize(Vector2f(30, 22));

		field->setHandle(UIEventType::ButtonClicked, "editPolygon", [=, &context] (const UIEvent& event)
		{
			ConfigNode options = ConfigNode(ConfigNode::MapType());
			options["isOpenPolygon"] = isOpenPolygon();

			ConfigNode::SequenceType compNames;
			for (const auto& name : componentNames) {
				compNames.emplace_back(ConfigNode(name));
			}
			options["componentNames"] = std::move(compNames);

			context.setTool(SceneEditorTool::Polygon, componentName, data.getName(), std::move(options));
		});

		return field;
	}
};

class ComponentEditorVertexListFieldFactory : public ComponentEditorPolygonFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::VertexList";
	}

	bool isOpenPolygon() override
	{
		return true;
	}
};

std::vector<std::unique_ptr<IComponentEditorFieldFactory>> EntityEditorFactories::getDefaultFactories()
{
	std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories;

	factories.emplace_back(std::make_unique<ComponentEditorTextFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorFloatFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorAngle1fFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorBoolFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVector2fFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorSpriteFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorAnimationPlayerFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorPolygonFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVertexListFieldFactory>());

	return factories;
}

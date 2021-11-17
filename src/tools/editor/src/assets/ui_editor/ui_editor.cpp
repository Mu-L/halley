#include "ui_editor.h"

#include "ui_widget_editor.h"
#include "ui_widget_list.h"
#include "halley/tools/project/project.h"
#include "src/scene/choose_asset_window.h"
using namespace Halley;

UIEditor::UIEditor(UIFactory& factory, Resources& gameResources, Project& project, const HalleyAPI& api)
	: AssetEditor(factory, gameResources, project, AssetType::UIDefinition)
{
	gameI18N = std::make_unique<I18N>(gameResources, I18NLanguage("en-GB"));
	gameFactory = project.getGameInstance()->createUIFactory(api, gameResources, *gameI18N);
	factory.loadUI(*this, "halley/ui_editor");
}

void UIEditor::onMakeUI()
{
	display = getWidget("display");
	widgetList = getWidgetAs<UIWidgetList>("widgetList");
	widgetList->setUIEditor(*this);
	widgetEditor = getWidgetAs<UIWidgetEditor>("widgetEditor");
	widgetEditor->setGameResources(gameResources);
	widgetEditor->setUIEditor(*this);

	setHandle(UIEventType::ListSelectionChanged, "widgetsList", [=] (const UIEvent& event)
	{
		setSelectedWidget(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "addWidget", [=] (const UIEvent& event)
	{
		addWidget();
	});

	setHandle(UIEventType::ButtonClicked, "removeWidget", [=] (const UIEvent& event)
	{
		removeWidget();
	});

	doLoadUI();
}

void UIEditor::onWidgetModified()
{
	uiDefinition->increaseAssetVersion();
	modified = true;
}

bool UIEditor::isModified()
{
	return modified;
}

void UIEditor::save()
{
	if (modified) {
		modified = false;

		const auto assetPath = Path("ui/" + uiDefinition->getAssetId() + ".yaml");
		const auto strData = uiDefinition->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
	}
}

UIFactory& UIEditor::getGameFactory()
{
	return *gameFactory;
}

void UIEditor::reload()
{
	doLoadUI();
}

std::shared_ptr<const Resource> UIEditor::loadResource(const String& id)
{
	uiDefinition = std::make_shared<UIDefinition>(*gameResources.get<UIDefinition>(id));

	widgetList->setDefinition(uiDefinition);

	return uiDefinition;
}

void UIEditor::doLoadUI()
{
	if (uiDefinition && display && !loaded) {
		display->clear();
		gameFactory->loadUI(*display, *uiDefinition);
		loaded = true;
	}
}

void UIEditor::setSelectedWidget(const String& id)
{
	widgetEditor->setSelectedWidget(id, uiDefinition->findUUID(id).result);
}

void UIEditor::addWidget()
{
	const auto window = std::make_shared<ChooseAssetWindow>(factory, [=] (std::optional<String> result)
	{
		if (result) {
			addWidget(result.value());
		}
	}, false);
	window->setAssetIds(gameFactory->getWidgetClassList(), "widget");
	window->setTitle(LocalisedString::fromHardcodedString("Choose Widget"));
	getRoot()->addChild(window);
}

void UIEditor::addWidget(const String& widgetClass)
{
	// TODO
}

void UIEditor::removeWidget()
{
	// TODO
}
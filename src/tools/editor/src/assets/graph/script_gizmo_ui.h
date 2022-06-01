#pragma once

#include "halley/ui/ui_widget.h"
#include "src/scene/gizmos/scripting/scripting_base_gizmo.h"

namespace Halley {
	class ScriptGizmoUI : public UIWidget {
	public:
		using ModifiedCallback = ScriptingBaseGizmo::ModifiedCallback;

		ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, ModifiedCallback modifiedCallback);

		void onAddedToRoot(UIRoot& root) override;

		void load(ScriptGraph& graph);

		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

		void setZoom(float zoom);
		bool isHighlighted() const;
		
		std::shared_ptr<UIWidget> makeUI();

	protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;

	private:
		UIFactory& factory;
		Resources& resources;
		ScriptingBaseGizmo gizmo;
		SceneEditorInputState inputState;
		ModifiedCallback modifiedCallback;

		void onModified();
	};
}
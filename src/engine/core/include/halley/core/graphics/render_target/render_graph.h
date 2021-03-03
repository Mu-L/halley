#pragma once
#include <memory>
#include <vector>

#include "render_graph_pin_type.h"
#include "graphics/texture_descriptor.h"

namespace Halley {
	class RenderGraphDefinition;
	class Camera;
	class TextureRenderTarget;
	class Texture;
	class RenderTarget;
	class VideoAPI;
	class RenderGraph;
	class RenderContext;
	class Painter;
	class Material;
	class RenderGraphNode;
	
	class RenderGraph {
	public:
		using PaintMethod = std::function<void(Painter&)>;

		RenderGraph();
		explicit RenderGraph(std::shared_ptr<const RenderGraphDefinition> graphDefinition);
		
		RenderGraphNode& addNode(gsl::span<const RenderGraphPinType> inputPins, gsl::span<const RenderGraphPinType> outputPins);
		RenderGraphNode& getRenderContextNode();

		void render(const RenderContext& rc, VideoAPI& video);

		const Camera& getCamera(std::string_view id) const;
		void setCamera(std::string_view id, const Camera& camera);

		const PaintMethod& getPaintMethod(std::string_view id) const;
		void setPaintMethod(std::string_view id, PaintMethod method);

	private:
		std::vector<std::unique_ptr<RenderGraphNode>> nodes;
		std::map<String, RenderGraphNode*> nodeMap;
		
		std::map<String, Camera> cameras;
		std::map<String, PaintMethod> paintMethods;
	};
}

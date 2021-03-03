#include "graphics/render_target/render_graph_definition.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;

RenderGraphDefinition::RenderGraphDefinition(const ConfigNode& config)
{
	for (const auto& node: config["nodes"].asSequence()) {
		nodes.emplace_back(node);
	}
	for (const auto& connection: config["connections"].asSequence()) {
		connections.emplace_back(connection);
	}
}

RenderGraphDefinition::Node::Node(const ConfigNode& node)
{
	id = node["id"].asString();
	method = node["method"].asString();
	methodParameters = ConfigNode(node["methodParameters"]);
}

RenderGraphDefinition::Connection::Connection(const ConfigNode& node)
{
	const auto& fromNode = node["from"];
	const auto& toNode = node["to"];
	fromId = fromNode["node"].asString();
	fromPin = gsl::narrow_cast<uint8_t>(fromNode["pin"].asInt());
	toId = toNode["node"].asString();
	toPin = gsl::narrow_cast<uint8_t>(toNode["pin"].asInt());
}

void RenderGraphDefinition::serialize(Serializer& s) const
{
	s << nodes;
	s << connections;
}

void RenderGraphDefinition::deserialize(Deserializer& s)
{
	s >> nodes;
	s >> connections;
}

void RenderGraphDefinition::Node::serialize(Serializer& s) const
{
	s << id;
	s << method;
	s << methodParameters;
}

void RenderGraphDefinition::Node::deserialize(Deserializer& s)
{
	s >> id;
	s >> method;
	s >> methodParameters;
}

void RenderGraphDefinition::Connection::serialize(Serializer& s) const
{
	s << fromId;
	s << toId;
	s << fromPin;
	s << toPin;
}

void RenderGraphDefinition::Connection::deserialize(Deserializer& s)
{
	s >> fromId;
	s >> toId;
	s >> fromPin;
	s >> toPin;
}

std::unique_ptr<RenderGraphDefinition> RenderGraphDefinition::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<RenderGraphDefinition>();
	Deserializer::fromBytes(*result, loader.getStatic()->getSpan());
	return result;
}

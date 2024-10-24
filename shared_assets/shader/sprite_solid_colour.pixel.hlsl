#include "halley/sprite_attribute.hlsl"

Texture2D image : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
	float4 col = image.Sample(sampler0, input.texCoord0.xy);

	float alpha = col.a * input.colour.a;
	return float4(input.colour.rgb * alpha, alpha);
}

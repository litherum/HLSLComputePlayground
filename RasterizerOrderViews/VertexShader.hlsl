cbuffer Uniforms : register(b0) {
	uint width;
	uint height;
};

struct VertexShaderInput {
	float2 pos : POSITION;
};

struct VertexShaderOutput {
	float4 position : SV_Position;
	float2 tex : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	output.position = float4(input.pos, 0, 1);
	output.tex = (input.pos + 1) / 2 * float2(width, height);
	return output;
}
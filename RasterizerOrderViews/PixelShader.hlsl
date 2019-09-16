RasterizerOrderedTexture2D<uint> theTexture : register(u0);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

void main(PixelShaderInput input)
{
	theTexture[input.tex] = theTexture[input.tex] + 1;
}
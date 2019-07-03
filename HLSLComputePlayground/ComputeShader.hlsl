RWStructuredBuffer<float4x4> buffer1 : register(u0);
RWStructuredBuffer<float4x4> buffer2 : register(u1);

[numthreads(1, 1, 1)]
void main()
{
	float4x4 a = buffer1[0];
	float4x4 b = buffer1[1];
	float4x4 c = mul(a, b);
	buffer2[0] = c;
}
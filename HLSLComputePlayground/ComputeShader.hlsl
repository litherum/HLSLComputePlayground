RWStructuredBuffer<float4x4> buffer1 : register(u0);
RWStructuredBuffer<float4x4> buffer2 : register(u1);

[numthreads(1, 1, 1)]
void main()
{
	buffer2[0] = mul(buffer1[0], buffer1[1]);
}
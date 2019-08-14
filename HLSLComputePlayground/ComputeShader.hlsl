RWStructuredBuffer<float> buffer1 : register(u0);
RWStructuredBuffer<float> buffer2 : register(u1);

[numthreads(1, 1, 1)]
void main()
{
	buffer2[0] = ldexp(2.5, 3.5);
}
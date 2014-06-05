#include "Common.fx"

cbuffer per_once : register(b1) {
	int c_windowWidth;
	int c_windowHeight;
};

RWTexture2D<float4> output : register(u0);
RWStructuredBuffer<HitData> g_hitInput : register(u1);
RWStructuredBuffer<Sphere> g_spheres : register(u2);
RWStructuredBuffer<Tri> g_triangles : register(u3);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	uint index = threadID.y * c_windowWidth + threadID.x;

	HitData hit = g_hitInput[index];
	if (!hit.m_hit) {
		output[threadID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
	} else {
		if (hit.m_primitiveType == PRIMITIVE_TYPE_SPHERE) {
			output[threadID.xy] = g_spheres[hit.m_primitiveIndex].m_color;
		} else if (hit.m_primitiveType == PRIMITIVE_TYPE_TRIANGLE) {
			output[threadID.xy] = float4(0.0f, 1.0f, 0.0f, 1.0f);
		}
	}
}
#include "Common.fx"

cbuffer per_once : register(b1) {
	int c_windowWidth;
	int c_windowHeight;
};

cbuffer pointLights: register(b2) {
	PointLight c_pointLights[10];
};

RWTexture2D<float4> output : register(u0);
RWStructuredBuffer<HitData> g_hitInput : register(u1);
RWStructuredBuffer<Sphere> g_spheres : register(u2);
RWStructuredBuffer<Tri> g_triangles : register(u3);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	uint index = threadID.y * c_windowWidth + threadID.x;
	float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

	HitData hit = g_hitInput[index];
	if (hit.m_hit) {
		if (hit.m_primitiveType == PRIMITIVE_TYPE_SPHERE) {
			color = g_spheres[hit.m_primitiveIndex].m_color;
		} else if (hit.m_primitiveType == PRIMITIVE_TYPE_TRIANGLE) {
			color = float4(0.0f, 1.0f, 0.0f, 1.0f);
		}

		
		for (uint i = 0; i < POINT_LIGHT_COUNT; ++i) {
			float3 toLight = normalize(c_pointLights[i].m_position - hit.m_position.xyz);
			color *= saturate(dot(toLight, hit.m_normal.xyz)) * c_pointLights[i].m_intensity;
		}
		
	}

	output[threadID.xy] = color;
}
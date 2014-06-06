#include "Common.fx"

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	uint index = threadID.y * c_windowWidth + threadID.x;

	// Get the ray.
	Ray ray = g_rays[index];
	Ray reflected;
	reflected.m_origin = float4(0, 0, 0, 0);
	reflected.m_direction = float4(0, 0, 0, 0);

	// Intersect versus the scene.
	HitData closest_hit = ray_vs_scene(ray);

	// Calculate the reflected ray.
	if (closest_hit.m_hit) {
		reflected.m_direction = float4(reflect(ray.m_direction.xyz, closest_hit.m_normal.xyz), 0.0f);
		reflected.m_origin = closest_hit.m_position;
	}

	g_hits[index] = closest_hit;
	g_rays[index] = reflected;
}


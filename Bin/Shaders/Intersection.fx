#include "Common.fx"

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	uint index = threadID.y * c_windowWidth + threadID.x;

	// Get the ray.
	Ray ray = g_rays[index];

	HitData closest_hit;

	// Intersect versus the scene.
	if (c_loopCount == 0) {
		closest_hit = ray_vs_scene(ray);
		closest_hit.m_previousSharpness = 1.0f;
	}
	else {
		closest_hit = g_hits[index];
		float previousSharpness = closest_hit.m_previousSharpness;
		if (closest_hit.m_hit)
			closest_hit = ray_vs_scene(ray);
		closest_hit.m_previousSharpness = previousSharpness;
	}

	Ray reflected;
	reflected.m_origin = float4(0, 0, 0, 0);
	reflected.m_direction = float4(0, 0, 0, 0);

	

	// Calculate the reflected ray.
	if (closest_hit.m_hit) {
		reflected.m_direction = float4(reflect(ray.m_direction.xyz, closest_hit.m_normal.xyz), 0.0f);
		reflected.m_origin = closest_hit.m_position;
	}

	g_hits[index] = closest_hit;
	g_rays[index] = reflected;
}


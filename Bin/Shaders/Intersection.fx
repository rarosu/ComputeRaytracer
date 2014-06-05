#include "Common.fx"

RWTexture2D<Ray> g_rayIO : register(u0);
RWTexture2D<HitData> g_hitOutput : register(u1);
RWStructuredBuffer<Sphere> g_spheres : register(u2);
RWStructuredBuffer<Tri> g_triangles : register(u3);

HitData ray_vs_sphere(Ray _r, Sphere _s);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	// Retrieve the amount of primitives.
	uint garbage = 0;
	uint num_spheres = 0; 
	uint num_triangles = 0; 

	g_spheres.GetDimensions(num_spheres, garbage);
	g_triangles.GetDimensions(num_triangles, garbage);

	// Get the ray.
	Ray ray = g_rayIO[threadID.xy];
	Ray reflected;
	reflected.m_origin = float4(0);
	reflected.m_direction = float4(0);

	// Intersect versus spheres.
	HitData closest_hit;
	closest_hit.m_t = 999999.9f;
	for (uint i = 0; i < num_spheres; ++i) {
		HitData current_hit = ray_vs_sphere(ray, g_spheres[i]);

		if (current_hit.m_t >= 0.0f && current_hit.m_t < closest_hit.m_t) {
			closest_hit = current_hit;
			closest_hit.m_primitiveType = PRIMITIVE_TYPE_SPHERE;
			closest_hit.m_primitiveIndex = i;
		}
	}

	// TODO: Intersection versus triangles.

	if (closest_hit.m_t > 0.0f) {
		closest_hit.m_position = ray.m_origin + ray.m_direction * current_hit.m_t;
		closest_hit.m_normal = normalize(closest_hit.m_position - g_spheres[i].m_position.xyz);

		reflected.m_direction = float4(reflect(ray.m_direction.xyz, closest_hit.m_normal.xyz), 0.0f);
		reflected.m_origin = closest_hit.m_position;
	}

	g_hitOutput[threadID.xy] = closest_hit;
	g_rayIO[threadID.xy] = reflected;
}

HitData ray_vs_sphere(Ray _r, Sphere _s)  {
	HitData result;
	result.m_t = -1.0f;
	
	float3 l = _s.m_position.xyz - _r.m_origin.xyz;
	float s = dot(l, _r.m_direction.xyz);
	float l_squared = dot(l, l);

	if (s < 0 && l_squared > _s.m_position.w * _s.m_position.w)
		return result;

	float m_squared = l_squared - (s * s);
	if (m_squared > _s.m_position.w * _s.m_position.w)
		return result;

	float q = sqrt((_s.m_position.w * _s.m_position.w) - m_squared);
	if (l_squared > _s.m_position.w * _s.m_position.w)
		result.m_t = s - q;
	else
		result.m_t = s + q;

	return result;
}

/*





[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	
	
	float4 near, far;
	near = float4((threadID.xy / 799.0f) * 2.0f - 1.0f, 0, 1);
	far = float4((threadID.xy / 799.0f) * 2.0f - 1.0f, 1, 1);
	
	near = mul(near, c_inv_vp); near /= near.w;
	far = mul(far, c_inv_vp); far /= far.w;
	
	ray l_ray;
	l_ray.m_origin = near;
	l_ray.m_direction = normalize(far - near);
	
	hit_data closest_hit;
	closest_hit.m_t = 999999.9f;
	closest_hit.m_color = float4(0.0f, 1.0f, 0.0f, 0.0f);

	for (uint i = 0; i < num_spheres; ++i) {
		hit_data current_hit = ray_vs_sphere(l_ray, g_spheres[i]);

		if (current_hit.m_t >= 0.0f && current_hit.m_t < closest_hit.m_t)
			closest_hit = current_hit;
	}

	//output[threadID.xy] = float4(float3(1,0,1) * (1 - length(threadID.xy - float2(400, 400)) / 400.0f), 1);
	output[threadID.xy] = closest_hit.m_color;
}


*/
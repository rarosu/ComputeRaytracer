//--------------------------------------------------------------------------------------
// BasicCompute.fx
// Direct3D 11 Shader Model 5.0 Demo
// Copyright (c) Stefan Petersson, 2012
//--------------------------------------------------------------------------------------

//https://github.com/elfrank/raytracer-gpupro4

struct sphere {
	float4 m_position; // w = radius
	float4 m_color;
};

struct tri {
	float4 m_corners[3];
	float4 m_colors[3];
};

struct ray {
	float4 m_origin;
	float4 m_direction;
};

struct hit_data {
	float m_t;
	float4 m_color;
};

cbuffer per_frame : register(b0) {
	float4x4 c_view;
	float4x4 c_projection;
	float4x4 c_inv_vp;
};

hit_data ray_vs_sphere(ray _r, sphere _s);

RWTexture2D<float4> output : register(u0);
RWStructuredBuffer<sphere> g_spheres : register(u1);
RWStructuredBuffer<tri> g_triangles : register(u2);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	const int num_spheres = 1;
	const int num_triangles = 0;
	
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
	closest_hit.m_color = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for (int i = 0; i < num_spheres; ++i) {
		hit_data current_hit = ray_vs_sphere(l_ray, g_spheres[i]);
		if (current_hit.m_t >= 0.0f && current_hit.m_t < closest_hit.m_t)
			closest_hit = current_hit;
	}

	//output[threadID.xy] = float4(float3(1,0,1) * (1 - length(threadID.xy - float2(400, 400)) / 400.0f), 1);
	output[threadID.xy] = closest_hit.m_color;
}

hit_data ray_vs_sphere(ray _r, sphere _s)  {
	hit_data result;
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

	result.m_color = _s.m_color;

	return result;
}
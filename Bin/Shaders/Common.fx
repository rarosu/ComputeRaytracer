#define THREAD_GROUPS 32
static const uint POINT_LIGHT_COUNT = 10;

/**
	STRUCTURES!
*/
struct Sphere {
	float4 m_position; // w = radius
	float4 m_diffuse;
	float4 m_specular;
	float m_sharpness;
};

struct Tri {
	float4 m_normal;
	float4 m_corners[3];
	float2 m_uv[3];
	uint m_materialIndex;
};

struct Ray {
	float4 m_origin;
	float4 m_direction;
};

static const uint PRIMITIVE_TYPE_NONE = 0;
static const uint PRIMITIVE_TYPE_SPHERE = 1;
static const uint PRIMITIVE_TYPE_TRIANGLE = 2;

/**
	Information about a ray intersection.
*/
struct HitData {
	float m_previousSharpness;
	float m_t;
	bool m_hit;
	float4 m_position;
	float4 m_normal;
	uint m_primitiveType;
	uint m_primitiveIndex;

	// Triangle specific.
	float2 m_barycentricCoords;
};

struct PointLight {
	float3 m_position;
	float m_radius;
	float4 m_diffuse;
	float4 m_specular;
};

struct Material {
	float4 m_diffuse;
	float4 m_specular;
	float m_sharpness;
};









/** 
	BUFFERS!
*/
cbuffer per_frame : register(b0) {
	float4x4 c_view;
	float4x4 c_projection;
	float4x4 c_inv_vp;
	float4 c_cameraPos;
};

cbuffer per_once : register(b1) {
	int c_windowWidth;
	int c_windowHeight;
};

cbuffer pointLights: register(b2) {
	PointLight c_pointLights[POINT_LIGHT_COUNT];
};

cbuffer aabbBuffer : register(b3) {
	float4 c_aabbMin;
	float4 c_aabbMax;
};

cbuffer per_bounce : register(b4) {
	uint c_loopCount;
}

cbuffer materialBuffer : register(b5) {
	Material c_materials[2];
}

RWTexture2D<float4> g_output : register(u0);
RWStructuredBuffer<float4> g_accumulatedOutput : register(u1);
RWStructuredBuffer<Ray> g_rays : register(u2);
RWStructuredBuffer<HitData> g_hits : register(u3);
RWStructuredBuffer<Sphere> g_spheres : register(u4);
RWStructuredBuffer<Tri> g_triangles : register(u5);

Texture2D g_shipDiffuse;





/**
	INTERSECTIONS!
*/
#define EPSILON 0.000001
#define DELTA 0.001

HitData ray_vs_sphere(Ray _r, Sphere _s)  {
	HitData result;
	result.m_t = -1.0f;
	result.m_hit = false;
	result.m_position = float4(0.0f, 0.0f, 0.0f, 0.0f);
	result.m_normal = float4(0.0f, 0.0f, 0.0f, 0.0f);
	result.m_primitiveType = PRIMITIVE_TYPE_NONE;
	result.m_primitiveIndex = 0;
	result.m_barycentricCoords = float2(0.0f, 0.0f);
	
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

	result.m_hit = true;
	result.m_primitiveType = PRIMITIVE_TYPE_SPHERE;
	result.m_position = _r.m_origin + _r.m_direction * (result.m_t - DELTA);
	result.m_normal = float4(normalize(result.m_position.xyz - _s.m_position.xyz), 0.0f);

	return result;
}

HitData ray_vs_triangle(Ray _r, Tri _t) {
	HitData result;
	result.m_t = -1.0f;
	result.m_hit = false;
	result.m_position = float4(0.0f, 0.0f, 0.0f, 0.0f);
	result.m_normal = float4(0.0f, 0.0f, 0.0f, 0.0f);
	result.m_primitiveType = PRIMITIVE_TYPE_NONE;
	result.m_primitiveIndex = 0;
	result.m_barycentricCoords = float2(0.0f, 0.0f);

	float3 e1 = _t.m_corners[1].xyz - _t.m_corners[0].xyz;
	float3 e2 = _t.m_corners[2].xyz - _t.m_corners[0].xyz;

	float3 P = cross(_r.m_direction.xyz, e2);
	float det = dot(e1, P);

	//NOT CULLING
	if(det > -EPSILON && det < EPSILON)
		return result;

	float inv_det = 1.0f / det;

	float3 T = _r.m_origin.xyz - _t.m_corners[0].xyz;
	float u = dot(T, P) * inv_det;

	// The intersection lies outside of the triangle
	if (u < 0.0f || u > 1.0f)
		return result;

	float3 Q = cross(T, e1);

	float v = dot(_r.m_direction.xyz, Q) * inv_det;

	// The intersection lies outside of the triangle
	if (v < 0.0f || u + v > 1.0f)
		return result;

	float t = dot(e2, Q) * inv_det;

	if (t <= EPSILON)
		return result;

	result.m_t = t;
	result.m_hit = true;
	result.m_position = _r.m_origin + (t - DELTA) * _r.m_direction;
	result.m_normal = _t.m_normal;
	result.m_primitiveType = PRIMITIVE_TYPE_TRIANGLE;
	result.m_barycentricCoords = float2(u, v);

	return result;
}

bool ray_vs_aabb(Ray _r) {
	float3 invdir = 1.0f / _r.m_direction.xyz;
 
    float t1 = (c_aabbMin.x - _r.m_origin.x) * invdir.x;
    float t2 = (c_aabbMax.x - _r.m_origin.x) * invdir.x;
    float t3 = (c_aabbMin.y - _r.m_origin.y) * invdir.y;
    float t4 = (c_aabbMax.y - _r.m_origin.y) * invdir.y;
    float t5 = (c_aabbMin.z - _r.m_origin.z) * invdir.z;
    float t6 = (c_aabbMax.z - _r.m_origin.z) * invdir.z;

    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    if(tmax < 0)
    	return false;
    if(tmin > tmax)
    	return false;

    return true;
}

HitData ray_vs_scene(Ray ray) {
	// Retrieve the amount of primitives.
	uint garbage = 0;
	uint num_spheres = 0; 
	uint num_triangles = 0; 

	g_spheres.GetDimensions(num_spheres, garbage);
	g_triangles.GetDimensions(num_triangles, garbage);

	HitData closest_hit;
	closest_hit.m_t = 999999.9f;
	closest_hit.m_hit = false;
	closest_hit.m_position = float4(0.0f, 0.0f, 0.0f, 0.0f);
	closest_hit.m_normal = float4(0.0f, 0.0f, 0.0f, 0.0f);
	closest_hit.m_primitiveType = PRIMITIVE_TYPE_NONE;
	closest_hit.m_primitiveIndex = 0;

	//Intersection versus spheres
	for (uint i = 0; i < num_spheres; ++i) {
		HitData current_hit = ray_vs_sphere(ray, g_spheres[i]);

		if (current_hit.m_hit && current_hit.m_t < closest_hit.m_t) {
			closest_hit = current_hit;
			closest_hit.m_primitiveIndex = i;
		}
	}

	// Intersection versus indoor box triangles.
	[unroll]
	for (i = 0; i < 10; ++i) {
		HitData current_hit = ray_vs_triangle(ray, g_triangles[i]);

		if (current_hit.m_hit && current_hit.m_t < closest_hit.m_t) {
			closest_hit = current_hit;
			closest_hit.m_primitiveIndex = i;
		}
	}

	//Intersection versus triangles.
	if (ray_vs_aabb(ray)) {
		for (i = 10; i < num_triangles; ++i) {
			HitData current_hit = ray_vs_triangle(ray, g_triangles[i]);

			if (current_hit.m_hit && current_hit.m_t < closest_hit.m_t) {
				closest_hit = current_hit;
				closest_hit.m_primitiveIndex = i;
			}
		
		}
	}

	return closest_hit;
}
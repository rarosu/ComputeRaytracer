
struct Sphere {
	float4 m_position; // w = radius
	float4 m_color;
};

struct Tri {
	float4 m_corners[3];
	float2 m_uv[3];
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
	float4 m_intensity;
};

static const uint POINT_LIGHT_COUNT = 1;
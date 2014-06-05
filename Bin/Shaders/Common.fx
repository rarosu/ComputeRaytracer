
struct Sphere {
	float4 m_position; // w = radius
	float4 m_color;
};

struct Tri {
	float4 m_corners[3];
	float4 m_colors[3];
};

struct Ray {
	float4 m_origin;
	float4 m_direction;
};

const int PRIMITIVE_TYPE_SPHERE = 1;
const int PRIMITIVE_TYPE_TRIANGLE = 2;

/**
	Information about a ray intersection.

	If m_t == -1.0f, the ray has not intersected.
*/
struct HitData {
	float m_t;
	float4 m_position;
	float4 m_normal;
	uint m_primitiveType;
	uint m_primitiveIndex;

	// Triangle specific.
	float2 m_barycentricCoords;
};
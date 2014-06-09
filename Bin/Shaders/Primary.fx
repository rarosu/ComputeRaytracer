#include "Common.fx"

[numthreads(THREAD_GROUPS, THREAD_GROUPS, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	uint index = threadID.y * c_windowWidth + threadID.x;

	float4 near, far;
	near = float4((threadID.xy / (float)(c_windowWidth - 1)) * 2.0f - 1.0f, 0, 1);
	far = float4((threadID.xy / (float)(c_windowHeight - 1)) * 2.0f - 1.0f, 1, 1);
	
	near = mul(near, c_inv_vp); near /= near.w;
	far = mul(far, c_inv_vp); far /= far.w;
	
	Ray ray;
	ray.m_origin = near;
	ray.m_direction = normalize(far - near);

	g_rays[index] = ray;
}
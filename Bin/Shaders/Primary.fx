#include "Common.fx"

cbuffer per_frame : register(c0) {
	float4x4 c_view;
	float4x4 c_projection;
	float4x4 c_inv_vp;
};

RWTexture2D<Ray> output : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	float4 near, far;
	near = float4((threadID.xy / 799.0f) * 2.0f - 1.0f, 0, 1);
	far = float4((threadID.xy / 799.0f) * 2.0f - 1.0f, 1, 1);
	
	near = mul(near, c_inv_vp); near /= near.w;
	far = mul(far, c_inv_vp); far /= far.w;
	
	Ray ray;
	ray.m_origin = near;
	ray.m_direction = normalize(far - near);

	output[threadID.xy] = ray;
}
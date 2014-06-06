#include "Common.fx"

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
	uint index = threadID.y * c_windowWidth + threadID.x;
	float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

	HitData hit = g_hits[index];
	if (hit.m_hit) {
		float4 diffuseM = float4(0.0f, 0.0f, 0.0f, 1.0f);
		float4 specularM = float4(0.0f, 0.0f, 0.0f, 1.0f);
		if (hit.m_primitiveType == PRIMITIVE_TYPE_SPHERE) {
			diffuseM = g_spheres[hit.m_primitiveIndex].m_diffuse;
			specularM = g_spheres[hit.m_primitiveIndex].m_specular;
		} else if (hit.m_primitiveType == PRIMITIVE_TYPE_TRIANGLE) {
			diffuseM = float4(0.0f, 1.0f, 0.0f, 1.0f);
			specularM = float4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		// Da phong model yo
		// color = ambientL * diffuseM + 
		//		   diffuseL * diffuseM * d + 
		//		   specularL * specularM * s;

		color = float4(0.1f, 0.1f, 0.1f, 0.0f) * diffuseM;
		for (uint i = 0; i < POINT_LIGHT_COUNT; ++i) {
			PointLight light = c_pointLights[i];

			float3 L = light.m_position - hit.m_position.xyz;
			float D = saturate(dot(normalize(L), hit.m_normal.xyz));
			float lightLength = length(L);
			float f = saturate(sign(light.m_radius - lightLength));

			D = f * lerp(D, 0, lightLength / light.m_radius);
			color += D * light.m_diffuse * diffuseM;


			float3 H = normalize(L + c_cameraPos.xyz);
			float S = saturate(dot(H, hit.m_normal.xyz));
			S = pow(S, 50.0f);
			
			S = f * lerp(S, 0, lightLength / light.m_radius);
			color += S * light.m_specular * specularM;		
		}
		
	}

	g_output[threadID.xy] = color;
}
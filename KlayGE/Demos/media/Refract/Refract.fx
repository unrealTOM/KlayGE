#include "../Common/util.fx"

float4x4 model;
float4x4 mvp;
float4x4 mv;
float4x4 modelit;
float4x4 inv_vp;
float3 eye_pos;

float3 eta_ratio;

// fresnel approximation
half fast_fresnel(half3 eye, half3 normal, half R0)
{
	// R0 = pow(1.0 - refractionIndexRatio, 2.0) / pow(1.0 + refractionIndexRatio, 2.0);

	half edn = max(0, dot(eye, normal));
	return R0 + (1.0 - R0) * pow(1.0 - edn, 5);
}

float3x3 my_refract3(float3 i, float3 n, float3 eta)
{
	float cosi = dot(i, n);
	float3 cost2 = 1.0 - eta * eta * (1.0 - cosi * cosi);
	float3 tmp = eta * cosi + sqrt(abs(cost2));

	float3x3 ret;
	ret[0] = (cost2.x < 0) ? 0 : (eta.x * i - tmp.x * n);
	ret[1] = (cost2.y < 0) ? 0 : (eta.y * i - tmp.y * n);
	ret[2] = (cost2.z < 0) ? 0 : (eta.z * i - tmp.z * n);
    return ret;
}

void RefractVS(float4 pos			: POSITION,
				float3 normal		: NORMAL,
				out float4 oPos		: POSITION,
				out float3 out_normal	: TEXCOORD0,
				out float4 incident : TEXCOORD1,
				out float4 refract_vec : TEXCOORD2,
				out float4 pos_ss	: TEXCOORD3,
				out float4 dir_ss	: TEXCOORD4)
{
	oPos = mul(pos, mvp);
	pos_ss = oPos;

	out_normal = mul(normal, (float3x3)modelit);
	incident.xyz = mul(pos, model).xyz - eye_pos;
	incident.w = length(incident.xyz);
	
	float3 n_incident = normalize(incident.xyz);
	float3 n_normal = normalize(normal);
	refract_vec.xyz = refract(n_incident, n_normal, eta_ratio.g);
	refract_vec.w = fast_fresnel(-n_incident, n_normal, 0.0977f);
	dir_ss = mul(half4(refract_vec.xyz, 0), mvp);
}


sampler skybox_YcubeMapSampler;
sampler skybox_CcubeMapSampler;

sampler BackFace_Sampler;

float4 RefractPS(float3 normal			: TEXCOORD0,
					float4 incident		: TEXCOORD1,
					float4 refract_vec	: TEXCOORD2,
					float4 pos_ss		: TEXCOORD3,
					float4 dir_ss		: TEXCOORD4) : COLOR
{
	normal = normalize(normal);
	incident.xyz = normalize(incident.xyz);

	half2 tex = pos_ss.xy / pos_ss.w;
	tex.y = -tex.y;
	tex = tex / 2 + 0.5f;
	pos_ss += dir_ss * (tex2D(BackFace_Sampler, tex).w - incident.w);

	tex = pos_ss.xy / pos_ss.w;
	tex.y = -tex.y;
	tex = tex / 2 + 0.5f;
	half3 back_face_normal = tex2D(BackFace_Sampler, tex).xyz;

	half3x3 refract_rgb = my_refract3(refract_vec.xyz, back_face_normal, 1 / eta_ratio);
	half3 reflect_vec = reflect(incident.xyz, normal);

	half4 y = half4(texCUBE(skybox_YcubeMapSampler, refract_rgb[0]).r,
									texCUBE(skybox_YcubeMapSampler, refract_rgb[1]).r,
									texCUBE(skybox_YcubeMapSampler, refract_rgb[2]).r,
									texCUBE(skybox_YcubeMapSampler, reflect_vec).r);
	y = exp2(y * 65536 / 2048 - 16);
	half4 refracted_c = half4(texCUBE(skybox_CcubeMapSampler, refract_rgb[0]).a,
									texCUBE(skybox_CcubeMapSampler, refract_rgb[1]).g,
									texCUBE(skybox_CcubeMapSampler, refract_rgb[2]).ga);
	half2 reflected_c = texCUBE(skybox_CcubeMapSampler, reflect_vec).ga;
	refracted_c *= refracted_c;
	reflected_c *= reflected_c;

	half3 refracted_clr = y.xyz * half3(refracted_c.x, (1 - refracted_c.z - refracted_c.w), refracted_c.y);
	half3 reflected_clr = y.w * half3(reflected_c.y, (1 - reflected_c.y - reflected_c.x), reflected_c.x);

	return float4(lerp(refracted_clr, reflected_clr, refract_vec.w) / half3(0.299f, 0.587f, 0.114f), 1);
}

technique Refract
{
	pass p0
	{
		ZFunc = LessEqual;
		CullMode = CCW;

		VertexShader = compile vs_1_1 RefractVS();
		PixelShader = compile ps_2_0 RefractPS();
	}
}


void RefractBackFaceVS(float4 pos : POSITION,
							float3 normal : NORMAL,
							out float4 oPos : POSITION,
							out float3 out_normal : TEXCOORD0,
							out float3 pos_es : TEXCOORD1)
{
	oPos = mul(pos, mvp);
	out_normal = mul(normal, (float3x3)modelit);
	pos_es = mul(pos, model).xyz - eye_pos;
}

float4 RefractBackFacePS(float3 normal : TEXCOORD0,
							float3 pos_es : TEXCOORD1) : COLOR
{
	return float4(-normalize(normal), length(pos_es));
}

technique RefractBackFace
{
	pass p0
	{
		ZFunc = Greater;
		CullMode = CW;

		VertexShader = compile vs_1_1 RefractBackFaceVS();
		PixelShader = compile ps_2_0 RefractBackFacePS();
	}
}

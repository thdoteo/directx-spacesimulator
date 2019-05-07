// shader.fx (old: Tutorial022.fx)

Texture2D tex : register(t0);
Texture2D tex2 : register(t1);
SamplerState samLinear : register(s0);

cbuffer CONSTANT_BUFFER : register(b0)
{
	float div_tex_x;	// dividing of the texture coordinates in x
	float div_tex_y;	// dividing of the texture coordinates in x
	float slice_x;		// which if the 4x4 images
	float slice_y;		// which if the 4x4 images
	
	matrix world;
	matrix view;
	matrix projection;

	float4 vLightDir[1];
	float4 vLightColor[1];
};

struct SimpleVertex
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : POSITION1;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
};

//
// Vertex Shader
//

PS_INPUT VShader(SimpleVertex input)
{
	PS_INPUT output;
	float4 pos = input.Pos;

	pos = mul(world, pos);
	output.WorldPos = pos.xyz;
	pos = mul(view, pos);
	pos = mul(projection, pos);
	
	matrix w = world;
	w._14 = 0;
	w._24 = 0;
	w._34 = 0;

	float4 norm;
	norm.xyz = input.Norm;
	norm.w = 1;
	norm = mul(w, norm);
	norm.x *= -1;
	output.Norm = normalize(norm.xyz);

	output.Pos = pos;
	output.Tex = input.Tex;
	return output;
}

//
// Pixel Shader
//

float4 PSLight(PS_INPUT input) : SV_Target
{
	float dist = input.WorldPos.x + 6.0f;
	float4 rColor = lerp(float4(0,0,1,1), float4(1,1,1,1), dist);

	for (int i = 0; i < 1; i++)
	{
		rColor += saturate(dot((float3)vLightDir[i],input.Norm) * vLightColor[i]);
	}
	rColor.a = 1;
	return rColor;
}

float4 PSTexture(PS_INPUT input) : SV_Target
{
	float4 color = tex.Sample(samLinear, input.Tex);
	color.a = 1;
	return color;
}

float4 PSColor(PS_INPUT input) : SV_Target
{
	return float4(1, 0, 0, 1);
}

float4 PSGradient(PS_INPUT input) : SV_Target
{
	float dist = input.WorldPos.x;
	float4 rColor = lerp(float4(0,0,1,1), float4(1,1,1,1), dist);
	return rColor;
}
// Global variables
//
//float    g_fTime;                   // App's time in seconds
//float4x4 g_mWorld;                  // World matrix for object
//float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
//

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

//void OnD3D11CreateDevice()
//{
//
//    g_pmWorldViewProjection = g_pEffect11->GetVariableByName(
//        "g_mWorldViewProjection")->AsMatrix();
//    g_pmWorld = g_pEffect11->GetVariableByName("g_mWorld")->AsMatrix();
//    g_pfTime = g_pEffect11->GetVariableByName("g_fTime")->AsScalar();
//}
//
//void OnD3D11FrameRender()
//{
//    g_pmWorldViewProjection->SetMatrix((float*)&mWorldViewProjection);
//    g_pmWorld->SetMatrix((float*)&mWorld);
//    g_pfTime->SetFloat((float)fTime);
//}




//
//
//       VS_OUTPUT RenderSceneVS(float4 vPos : POSITION, float3 vNormal : NORMAL, float2 vTexCoord0 : TEXCOORD0, uniform int nNumLights, 
//uniform bool bTexture, uniform bool bAnimate)
//{
//    VS_OUTPUT Output;
//    float3 vNormalWorldSpace;
//
//    float4 vAnimatedPos = vPos;
//
//    // Animation the vertex based on time and the vertex's object space position
//    if (bAnimate)
//        vAnimatedPos += float4(vNormal, 0) * (sin(g_fTime + 5.5) + 0.5) * 5;
//
//    // Transform the position from object space to homogeneous projection space
//    Output.Position = mul(vAnimatedPos, g_mWorldViewProjection);
//
//    // Transform the normal from object space to world space    
//    vNormalWorldSpace = normalize(mul(vNormal, (float3x3)g_mWorld));
//
//    // Compute simple directional lighting equation
//    float3 vTotalLightDiffuse = float3(0, 0, 0);
//    for (int i = 0; i < nNumLights; i++)
//        vTotalLightDiffuse += g_LightDiffuse[i] * max(0, dot(vNormalWorldSpace, g_LightDir[i]));
//
//    Output.Diffuse.rgb = g_MaterialDiffuseColor * vTotalLightDiffuse +
//        g_MaterialAmbientColor * g_LightAmbient;
//    Output.Diffuse.a = 1.0f;
//
//    // Just copy the texture coordinate through
//    if (bTexture)
//        Output.TextureUV = vTexCoord0;
//    else
//        Output.TextureUV = 0;
//
//    return Output;
//}

VS_OUTPUT RenderSceneVS(float4 Pos : POSITION, float4 Color : COLOR)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Color = Color;
    return output;
}



struct PS_OUTPUT
{
    float4 RGBColor : COLOR0;  // Pixel color    
};


PS_OUTPUT RenderScenePS(VS_OUTPUT In)//, float time)
{
   /* PS_OUTPUT Output;
    Output.r = In.Color.r * sin(time);
    Output.g = In.Color.g * sin(time);
    Output.b = In.Color.b * sin(time);
    Output.a = In.Color.a;*/

    return In.Color;
}


technique RenderScene
{
    pass P0
    {
        VertexShader = compile vs_1_1 RenderSceneVS();
        PixelShader = compile ps_1_1 RenderScenePS();// g_fTime);
    }
}
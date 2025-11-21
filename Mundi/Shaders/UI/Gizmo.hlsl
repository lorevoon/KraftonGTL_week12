//================================================================================================
// Filename:      Gizmo.hlsl (Gizmo Shader)
// Description:   Simplified shader for Gizmo rendering (Arrow, Rotate, Scale components)
//                - Lambertian diffuse lighting
//                - No texture support
//                - Custom color per gizmo with highlight support
//================================================================================================

// --- Constant Buffers ---

// b0: ModelBuffer (VS) - World transform
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
    row_major float4x4 WorldInverseTranspose; // Used for correct normal transformation
}

// b1: ViewProjBuffer (VS) - Camera matrices
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
}

// b3: ColorBuffer (PS) - Color and UUID for object picking
cbuffer ColorBuffer : register(b3)
{
    float4 LerpColor;   // Base color for the gizmo component
    uint UUID;          // Object ID for picking
}

// --- Input/Output Structures ---

struct VS_INPUT
{
    float3 position : POSITION;     // Vertex position
    float3 normal : NORMAL;         // Vertex normal
};

struct PS_INPUT
{
    float4 position : SV_POSITION;  // Clip-space position
    float3 worldNormal : TEXCOORD0; // World-space normal for Lighting
    float3 worldPos : TEXCOORD1;    // World-space position for Lighting
    float4 color : COLOR;           // Gizmo color (from vertex shader)
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;      // Final color output
    uint UUID : SV_Target1;         // Object ID for picking
};

//================================================================================================
// Vertex Shader
//================================================================================================
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // Calculate world position
    float4 worldPos = mul(float4(input.position, 1.0f), WorldMatrix);
    output.worldPos = worldPos.xyz;
    
    // Transform vertex position: Model -> World -> View -> Clip space
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    output.position = mul(float4(input.position, 1.0f), MVP);
    
    // Transform normal from model space to world space for lighting
    output.worldNormal = normalize(mul(input.normal, (float3x3) WorldInverseTranspose));
    output.color = LerpColor;

    return output;
}

//================================================================================================
// Pixel Shader
//================================================================================================
PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT Output;

    // Face normal calculation
    float3 ddx_pos = ddx(input.worldPos);
    float3 ddy_pos = ddy(input.worldPos);
    float3 faceNormal = normalize(cross(ddy_pos, ddx_pos));
    float3 N = faceNormal;
    
    // View-based lighting calculation
    float3 cameraPos = InverseViewMatrix[3].xyz;
    float3 viewDir = normalize(cameraPos - input.worldPos);
    float NdotL = abs(dot(N, -viewDir));  // lightDir = -viewDir
    
    // Define ambient and diffuse lighting
    float ambient = 0.22f;          // Minimum shadow brightness
    float diffuseIntensity = 0.9f;  // Additional brightness
    float diffuse = ambient + diffuseIntensity * NdotL;
    
    // Final color is base color modulated by lighting
    Output.Color = input.color * diffuse;
    Output.UUID = UUID;

    return Output;
}
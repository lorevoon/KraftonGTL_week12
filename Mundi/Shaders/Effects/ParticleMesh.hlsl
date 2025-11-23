// Particle Mesh Shader (Instancing-based)
// Slot 0: Mesh vertices (Position, Normal, UV)
// Slot 1: Instance data (Transform matrix + Color)

// b1: ViewProjBuffer (VS)
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

struct VS_INPUT
{
    // Per-vertex (Slot 0): Mesh vertex data
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;

    // Per-instance (Slot 1): Instance transform (4x4 matrix = 4 float4 rows)
    float4 InstanceRow0 : TEXCOORD1;  // Transform matrix row 0
    float4 InstanceRow1 : TEXCOORD2;  // Transform matrix row 1
    float4 InstanceRow2 : TEXCOORD3;  // Transform matrix row 2
    float4 InstanceRow3 : TEXCOORD4;  // Transform matrix row 3
    float4 InstanceColor : TEXCOORD5; // Instance color
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldNormal : NORMAL;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};

Texture2D ParticleTexture : register(t0);
SamplerState LinearSampler : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // Reconstruct instance transform matrix from 4 rows
    row_major float4x4 InstanceTransform = float4x4(
        input.InstanceRow0,
        input.InstanceRow1,
        input.InstanceRow2,
        input.InstanceRow3
    );

    // 1. Transform vertex position by instance transform
    float4 worldPos = mul(float4(input.Position, 1.0f), InstanceTransform);

    // 2. Transform to view and projection space
    float4 viewPos = mul(worldPos, ViewMatrix);
    output.Position = mul(viewPos, ProjectionMatrix);

    // 3. Transform normal (use 3x3 part of transform, ignore translation)
    float3x3 normalMatrix = (float3x3)InstanceTransform;
    output.WorldNormal = normalize(mul(input.Normal, normalMatrix));

    // 4. Pass UV
    output.UV = input.UV;

    // 5. Pass instance color
    output.Color = input.InstanceColor;

    return output;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;

    // Sample texture
    float4 texColor = ParticleTexture.Sample(LinearSampler, input.UV);

    // If texture alpha is near zero, assume no texture bound and use white
    if (texColor.a < 0.001f)
    {
        texColor = float4(1, 1, 1, 1);
    }

    // Simple lighting (ambient + directional)
    float3 lightDir = normalize(float3(0.5f, -1.0f, 0.5f));
    float ndotl = max(dot(input.WorldNormal, -lightDir), 0.0f);
    float ambient = 0.3f;
    float lighting = ambient + (1.0f - ambient) * ndotl;

    // Multiply texture * instance color * lighting
    output.Color = texColor * input.Color * float4(lighting, lighting, lighting, 1.0f);

    // Alpha test
    if (output.Color.a < 0.01f)
        discard;

    // No UUID output for particles
    output.UUID = 0;

    return output;
}

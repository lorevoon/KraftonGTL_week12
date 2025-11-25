// Particle Beam Shader (Dynamic vertex buffer)
// Slot 0: Beam vertices (Position, UV, Color)

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
    float3 Position : POSITION;     // World position (12 bytes)
    float2 UV : TEXCOORD0;          // UV coordinates (8 bytes)
    float Padding : TEXCOORD1;      // Padding (4 bytes)
    float4 Color : COLOR0;          // Vertex color (16 bytes)
    // Total: 40 bytes
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
};

Texture2D ParticleTexture : register(t0);
SamplerState LinearSampler : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // Transform to clip space
    float4 viewPos = mul(float4(input.Position, 1.0f), ViewMatrix);
    output.Position = mul(viewPos, ProjectionMatrix);

    // Pass UV and color
    output.UV = input.UV;
    output.Color = input.Color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target0
{
    // Sample texture
    float4 texColor = ParticleTexture.Sample(LinearSampler, input.UV);

    // If texture alpha is near zero, assume no texture bound and use white
    if (texColor.a < 0.001f)
    {
        texColor = float4(1, 1, 1, 1);
    }

    // Multiply with vertex color
    float4 finalColor = texColor * input.Color;

    // Soft alpha edge (smoothstep으로 부드러운 엣지)
    float alphaMask = smoothstep(0.0f, 0.05f, finalColor.a);
    finalColor.a *= alphaMask;

    // Alpha test
    if (finalColor.a < 0.001f)
        discard;

    return finalColor;
}

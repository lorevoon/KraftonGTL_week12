// Particle Sprite Shader (Instancing-based)
// Slot 0: Quad vertices (FVector2D)
// Slot 1: Instance data (FParticleSpriteInstance)

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
    // Per-vertex (Slot 0): Quad local offset
    float2 QuadPos : POSITION;

    // Per-instance (Slot 1): Particle instance data (16-byte aligned)
    float4 WorldPosAndPad : TEXCOORD0;  // .xyz = WorldPosition, .w = padding
    float4 SizeRotAndPad : TEXCOORD1;   // .xy = Size, .z = Rotation, .w = padding
    float4 Color : TEXCOORD2;           // .rgba = Color
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
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

    // Extract data from packed inputs
    float3 WorldPosition = input.WorldPosAndPad.xyz;
    float2 Size = input.SizeRotAndPad.xy;
    float Rotation = input.SizeRotAndPad.z;

    // 1. Rotate quad vertex
    float2 rotatedPos = input.QuadPos;
    if (Rotation != 0.0f)
    {
        float cosRot = cos(Rotation);
        float sinRot = sin(Rotation);
        rotatedPos = float2(
            input.QuadPos.x * cosRot - input.QuadPos.y * sinRot,
            input.QuadPos.x * sinRot + input.QuadPos.y * cosRot
        );
    }

    // 2. Scale by particle size
    float2 scaledPos = rotatedPos * Size;

    // 3. Billboard to face camera (align with view plane)
    float3 billboardOffset = mul(float4(scaledPos, 0.0f, 0.0f), InverseViewMatrix).xyz;

    // 4. Translate to world position
    float3 worldPos = WorldPosition + billboardOffset;

    // 5. Transform to clip space
    float4 viewPos = mul(float4(worldPos, 1.0f), ViewMatrix);
    output.Position = mul(viewPos, ProjectionMatrix);

    // 6. UV: Map quad [-0.5, 0.5] to [0, 1], flip Y for correct texture orientation
    output.UV = input.QuadPos + 0.5f;
    output.UV.y = 1.0f - output.UV.y;

    // 7. Pass color
    output.Color = input.Color;

    return output;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;

    // Sample texture (use white if texture is not bound)
    float4 texColor = ParticleTexture.Sample(LinearSampler, input.UV);

    // If texture alpha is near zero, assume no texture bound and use white
    if (texColor.a < 0.001f)
    {
        texColor = float4(1, 1, 1, 1);
    }

    // Multiply with vertex color
    output.Color = texColor * input.Color;

    // Alpha test
    if (output.Color.a < 0.01f)
        discard;

    // For now, no UUID output for particles
    output.UUID = 0;

    return output;
}

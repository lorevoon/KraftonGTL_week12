// b0: ModelBuffer (VS) - ModelBufferType과 정확히 일치 (128 bytes)
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix; // 64 bytes
    row_major float4x4 WorldInverseTranspose; // 64 bytes - 올바른 노멀 변환을 위함
};

// b1: ViewProjBuffer (VS) - ViewProjBufferType과 일치
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;   // 0행 광원의 월드 좌표 + 스포트라이트 반경
    row_major float4x4 InverseProjectionMatrix;
};

// b6: BoneMatrices (VS) - GPU Skinning용 본 행렬 배열
#ifdef GPU_SKINNING
#define MAX_BONES 256
cbuffer BoneMatricesBuffer : register(b6)
{
    row_major float4x4 BoneMatrices[MAX_BONES];
};
#endif

// --- 셰이더 입출력 구조체 ---
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Tangent : TANGENT0;
    float4 Color : COLOR;
#ifdef GPU_SKINNING
    uint4 BoneIndices : BLENDINDICES;    // 영향을 주는 본 인덱스 (최대 4개)
    float4 BoneWeights : BLENDWEIGHT;    // 본 가중치 (합=1.0)
#endif
};

// 출력은 오직 클립 공간 위치만 필요
struct VS_OUT
{
    float4 Position : SV_Position;
    float3 WorldPosition : TEXCOORD0;
};

VS_OUT mainVS(VS_INPUT Input)
{
    VS_OUT Output = (VS_OUT) 0;

#ifdef GPU_SKINNING
    // GPU 스키닝: 본 행렬로 버텍스 블렌딩
    float3 skinnedPosition = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 4; i++)
    {
        uint boneIndex = Input.BoneIndices[i];
        float weight = Input.BoneWeights[i];

        if (weight > 0.0f)
        {
            // Position 블렌딩
            skinnedPosition += mul(float4(Input.Position, 1.0f), BoneMatrices[boneIndex]).xyz * weight;
        }
    }

    // 스키닝된 결과를 사용
    float3 localPosition = skinnedPosition;
#else
    // CPU 스키닝: 이미 변환된 버텍스 사용
    float3 localPosition = Input.Position;
#endif

    // 모델 좌표 -> 월드 좌표 -> 뷰 좌표 -> 클립 좌표
    float4 WorldPos = mul(float4(localPosition, 1.0f), WorldMatrix);
    float4 ViewPos = mul(WorldPos, ViewMatrix);
    Output.Position = mul(ViewPos, ProjectionMatrix);
    Output.WorldPosition = WorldPos.xyz;

    return Output;
}
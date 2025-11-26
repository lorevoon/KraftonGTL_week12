#include "pch.h"
#include "ParticleModule.h"
#include "Source/Runtime/Core/Object/Property.h"
#include "ResourceManager.h"
#include "Material.h"
#include "Shader.h"

UParticleModule::UParticleModule()
    : bSpawnModule(false)
    , bUpdateModule(false)
    , bFinalUpdateModule(false)
    , bEnabled(true)
    , ModuleName("ParticleModule")
{
}

void UParticleModule::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    UClass* ModuleClass = GetClass();
    if (!ModuleClass)
        return;

    if (bInIsLoading)
    {
        // Load - use reflection to deserialize all properties
        const TArray<FProperty>& Properties = ModuleClass->GetAllProperties();
        for (const FProperty& Prop : Properties)
        {
            if (Prop.bIsEditAnywhere && InOutHandle.hasKey(Prop.Name))
            {
                const JSON& PropValue = InOutHandle.at(Prop.Name);

                switch (Prop.Type)
                {
                case EPropertyType::Bool:
                {
                    bool* Value = Prop.GetValuePtr<bool>(this);
                    if (PropValue.JSONType() == JSON::Class::Boolean)
                        *Value = PropValue.ToBool();
                    break;
                }
                case EPropertyType::Int32:
                {
                    int32* Value = Prop.GetValuePtr<int32>(this);
                    if (PropValue.JSONType() == JSON::Class::Integral)
                        *Value = static_cast<int32>(PropValue.ToInt());
                    break;
                }
                case EPropertyType::Float:
                {
                    float* Value = Prop.GetValuePtr<float>(this);
                    if (PropValue.JSONType() == JSON::Class::Floating)
                        *Value = static_cast<float>(PropValue.ToFloat());
                    break;
                }
                case EPropertyType::FVector:
                {
                    FVector* Value = Prop.GetValuePtr<FVector>(this);
                    if (PropValue.JSONType() == JSON::Class::Array && PropValue.size() == 3)
                    {
                        Value->X = static_cast<float>(PropValue.at(0).ToFloat());
                        Value->Y = static_cast<float>(PropValue.at(1).ToFloat());
                        Value->Z = static_cast<float>(PropValue.at(2).ToFloat());
                    }
                    break;
                }
                case EPropertyType::FLinearColor:
                {
                    FLinearColor* Value = Prop.GetValuePtr<FLinearColor>(this);
                    if (PropValue.JSONType() == JSON::Class::Array && PropValue.size() == 4)
                    {
                        Value->R = static_cast<float>(PropValue.at(0).ToFloat());
                        Value->G = static_cast<float>(PropValue.at(1).ToFloat());
                        Value->B = static_cast<float>(PropValue.at(2).ToFloat());
                        Value->A = static_cast<float>(PropValue.at(3).ToFloat());
                    }
                    break;
                }
                case EPropertyType::FString:
                {
                    FString* Value = Prop.GetValuePtr<FString>(this);
                    if (PropValue.JSONType() == JSON::Class::String)
                        *Value = PropValue.ToString();
                    break;
                }
                case EPropertyType::Material:
                {
                    UMaterialInterface** Value = Prop.GetValuePtr<UMaterialInterface*>(this);

                    // Case 1: 문자열 (기존 방식 - preload된 Material 경로)
                    if (PropValue.JSONType() == JSON::Class::String)
                    {
                        FString MaterialPath = PropValue.ToString();
                        if (!MaterialPath.empty())
                        {
                            *Value = UResourceManager::GetInstance().Load<UMaterial>(MaterialPath);
                        }
                        else
                        {
                            *Value = nullptr;
                        }
                    }
                    // Case 2: 오브젝트 (Shader/Texture 정보로 Material 재구성)
                    else if (PropValue.JSONType() == JSON::Class::Object)
                    {
                        FString ShaderPath;
                        FString DiffuseTexture;

                        if (PropValue.hasKey("ShaderPath"))
                        {
                            ShaderPath = PropValue.at("ShaderPath").ToString();
                        }
                        if (PropValue.hasKey("DiffuseTexture"))
                        {
                            DiffuseTexture = PropValue.at("DiffuseTexture").ToString();
                        }

                        if (!ShaderPath.empty())
                        {
                            // ResourceManager를 통해 Material 로드 (캐싱됨)
                            // UMaterial::Load()는 .hlsl 경로를 받으면 해당 Shader를 로드하여 Material 생성
                            UMaterial* Mat = UResourceManager::GetInstance().Load<UMaterial>(ShaderPath);

                            // Texture 설정이 필요한 경우
                            if (Mat && !DiffuseTexture.empty())
                            {
                                FMaterialInfo MatInfo = Mat->GetMaterialInfo();
                                MatInfo.DiffuseTextureFileName = DiffuseTexture;
                                Mat->SetMaterialInfo(MatInfo);
                                Mat->ResolveTextures();
                            }

                            *Value = Mat;
                        }
                        else
                        {
                            *Value = nullptr;
                        }
                    }
                    break;
                }
                }
            }
        }
    }
    else
    {
        // Save - use reflection to serialize all properties
        InOutHandle["Type"] = ModuleClass->Name;

        const TArray<FProperty>& Properties = ModuleClass->GetAllProperties();
        for (const FProperty& Prop : Properties)
        {
            if (Prop.bIsEditAnywhere)
            {
                switch (Prop.Type)
                {
                case EPropertyType::Bool:
                {
                    bool* Value = Prop.GetValuePtr<bool>(this);
                    InOutHandle[Prop.Name] = *Value;
                    break;
                }
                case EPropertyType::Int32:
                {
                    int32* Value = Prop.GetValuePtr<int32>(this);
                    InOutHandle[Prop.Name] = static_cast<long>(*Value);
                    break;
                }
                case EPropertyType::Float:
                {
                    float* Value = Prop.GetValuePtr<float>(this);
                    InOutHandle[Prop.Name] = static_cast<double>(*Value);
                    break;
                }
                case EPropertyType::FVector:
                {
                    FVector* Value = Prop.GetValuePtr<FVector>(this);
                    InOutHandle[Prop.Name] = FJsonSerializer::VectorToJson(*Value);
                    break;
                }
                case EPropertyType::FLinearColor:
                {
                    FLinearColor* Value = Prop.GetValuePtr<FLinearColor>(this);
                    JSON ColorArray = JSON::Make(JSON::Class::Array);
                    ColorArray.append(Value->R, Value->G, Value->B, Value->A);
                    InOutHandle[Prop.Name] = ColorArray;
                    break;
                }
                case EPropertyType::FString:
                {
                    FString* Value = Prop.GetValuePtr<FString>(this);
                    InOutHandle[Prop.Name] = Value->c_str();
                    break;
                }
                case EPropertyType::Material:
                {
                    UMaterialInterface** Value = Prop.GetValuePtr<UMaterialInterface*>(this);
                    if (*Value)
                    {
                        // FilePath가 있으면 기존 방식 (preload된 Material)
                        FString MatPath = (*Value)->GetFilePath();
                        if (!MatPath.empty())
                        {
                            InOutHandle[Prop.Name] = MatPath.c_str();
                        }
                        else
                        {
                            // FilePath가 없으면 Shader/Texture 정보를 별도 저장 (NewObject로 생성된 경우)
                            JSON MatInfo = JSON::Make(JSON::Class::Object);

                            // Shader 경로 저장
                            UShader* Shader = (*Value)->GetShader();
                            if (Shader)
                            {
                                MatInfo["ShaderPath"] = Shader->GetFilePath().c_str();
                            }

                            // Texture 경로 저장 (Diffuse)
                            const FMaterialInfo& MInfo = (*Value)->GetMaterialInfo();
                            if (!MInfo.DiffuseTextureFileName.empty())
                            {
                                MatInfo["DiffuseTexture"] = MInfo.DiffuseTextureFileName.c_str();
                            }

                            InOutHandle[Prop.Name] = MatInfo;
                        }
                    }
                    else
                    {
                        InOutHandle[Prop.Name] = "";
                    }
                    break;
                }
                }
            }
        }
    }
}

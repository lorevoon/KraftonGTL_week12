#include "pch.h"
#include "ParticleModule.h"
#include "Source/Runtime/Core/Object/Property.h"

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
                }
            }
        }
    }
}

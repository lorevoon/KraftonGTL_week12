#pragma once
#include "Character.h"
#include "ADancingCharacter.generated.h"

class UAudioComponent;

UCLASS(DisplayName = "댄싱 캐릭터", Description = "애니메이션 키프레임 데이터 저장소")
class ADancingCharacter : public ACharacter
{
    GENERATED_REFLECTION_BODY()

public:
    ADancingCharacter();

protected:
    ~ADancingCharacter() override;

    virtual void BeginPlay() override;
    virtual void HandleAnimNotify(const FAnimNotifyEvent& NotifyEvent) override;

private:
    USound* SorrySound;
    USound* HitSound;
};

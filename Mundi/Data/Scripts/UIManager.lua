local PreState = "None"

local BillboardComponent

function BeginPlay()
    BillboardComponent = GetComponent(Obj, "UBillboardComponent")
end

function EndPlay()
    
end

function Tick(dt)
    -- 1. 카메라 액터를 가져옵니다.
    CameraActor = GetCamera()
    if not CameraActor then return end

    -- 2. 카메라의 위치와 정면 벡터를 가져옵니다.
    local CamPosition = CameraActor:GetActorLocation()
    local CamForward = CameraActor:GetActorRight() -- C++에서 바인딩한 새 함수

    -- 3. 최종 위치 계산: 카메라 위치 + (카메라 정면 방향 * 거리)
    Obj.Location = CamPosition + (CamForward * 2)

    if GlobalConfig.GameState == PreState then
        return
    end

    if GlobalConfig.GameState == "Start" then
        BillboardComponent:SetTexture("Data/Textures/gamestart.png")
    elseif GlobalConfig.GameState == "End" then
        BillboardComponent:SetTexture("Data/Textures/gameover.png")
    else
        BillboardComponent:SetTexture("")
    end

    PreState = GlobalConfig.GameState
end
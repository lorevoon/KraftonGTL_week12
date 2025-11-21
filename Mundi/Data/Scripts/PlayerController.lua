-- ────────────────────────────────────────────────────────────────────────────
-- PlayerController.lua
-- 플레이어 캐릭터 컨트롤러: WASD 입력으로 캐릭터 이동 처리 + 애니메이션 상태 머신
-- ────────────────────────────────────────────────────────────────────────────

-- Character 참조 (BeginPlay에서 초기화됨)
local Character = nil
local SkeletalMeshComp = nil
local StateMachine = nil

-- 카메라 회전 감도
local YawSensitivity = 0.5      -- 좌우 회전 감도 (마우스)
local PitchSensitivity = 0.5    -- 상하 회전 감도 (마우스)

-- 이동 속도 배율
local MovementSpeed = 1.0
local RunSpeed = 2.0

-- 애니메이션 상태 이름
local STATE_IDLE = "Idle"
local STATE_WALKING = "Walking"
local STATE_WALKING_BACKWARDS = "WalkingBackwards"
local STATE_RUNNING = "Running"
local STATE_JUMPING = "Jumping"
local STATE_STRAFE_LEFT = "StrafeLeft"
local STATE_STRAFE_RIGHT = "StrafeRight"
local STATE_STRAFE_LEFT_WALK = "StrafeLeftWalk"
local STATE_STRAFE_RIGHT_WALK = "StrafeRightWalk"

-- 애니메이션 에셋 경로
local ANIM_IDLE = "Data/Male Locomotion Pack/Idle_mixamo.com"
local ANIM_WALKING = "Data/Male Locomotion Pack/Walking_mixamo.com"
local ANIM_WALKING_BACKWARDS = "Data/Male Locomotion Pack/Walking Backwards_mixamo.com"
local ANIM_RUNNING = "Data/Male Locomotion Pack/Standard Run_mixamo.com"
local ANIM_JUMPING = "Data/Male Locomotion Pack/Jump_mixamo.com"
local ANIM_STRAFE_LEFT = "Data/Male Locomotion Pack/Left Strafe_mixamo.com"
local ANIM_STRAFE_RIGHT = "Data/Male Locomotion Pack/Right Strafe_mixamo.com"
local ANIM_STRAFE_LEFT_WALK = "Data/Male Locomotion Pack/Left Strafe Walking_mixamo.com"
local ANIM_STRAFE_RIGHT_WALK = "Data/Male Locomotion Pack/Right Strafe Walking_mixamo.com"

-- Movement direction enumeration
local MOVE_NONE = 0
local MOVE_FORWARD = 1
local MOVE_BACKWARD = 2
local MOVE_LEFT = 3
local MOVE_RIGHT = 4

-- 현재 이동 상태 추적
local bIsMoving = false
local bIsRunning = false
local bWasJumping = false
local MoveDirection = MOVE_NONE

function BeginPlay()
    print("[PlayerController] BeginPlay called")

    -- GetOwnerAs를 사용해 Obj의 Owner를 ACharacter로 캐스팅
    -- ACharacter의 메서드들은 UFUNCTION(LuaBind)로 노출되어 있음
    Character = GetOwnerAs(Obj, "ACharacter")
    if Character then
        print("[PlayerController] Character found and cast successfully!")
    else
        print("[PlayerController] ERROR: Could not cast owner to ACharacter")
        return
    end

    -- SkeletalMeshComponent 가져오기
    SkeletalMeshComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalMeshComp then
        print("[PlayerController] ERROR: No USkeletalMeshComponent found on character")
        return
    end

    -- 애니메이션 상태 머신 설정
    SetupAnimationStateMachine()
end

function SetupAnimationStateMachine()
    print("[PlayerController] Setting up Animation State Machine")

    -- 상태 머신 사용 활성화 및 인스턴스 가져오기
    SkeletalMeshComp:UseStateMachine()
    StateMachine = SkeletalMeshComp:GetOrCreateStateMachine()

    if not StateMachine then
        print("[PlayerController] ERROR: Failed to create state machine")
        return
    end

    -- 상태 추가 (이름, 에셋 경로, 재생 속도, 루핑 여부)
    StateMachine:AddState(STATE_IDLE, ANIM_IDLE, 1.0, true)
    StateMachine:AddState(STATE_WALKING, ANIM_WALKING, 1.0, true)
    StateMachine:AddState(STATE_WALKING_BACKWARDS, ANIM_WALKING_BACKWARDS, 1.0, true)
    StateMachine:AddState(STATE_RUNNING, ANIM_RUNNING, 1.0, true)
    StateMachine:AddState(STATE_JUMPING, ANIM_JUMPING, 1.0, false)
    StateMachine:AddState(STATE_STRAFE_LEFT_WALK, ANIM_STRAFE_LEFT_WALK, 1.0, true)
    StateMachine:AddState(STATE_STRAFE_RIGHT_WALK, ANIM_STRAFE_RIGHT_WALK, 1.0, true)
    StateMachine:AddState(STATE_STRAFE_LEFT, ANIM_STRAFE_LEFT, 1.0, true)
    StateMachine:AddState(STATE_STRAFE_RIGHT, ANIM_STRAFE_RIGHT, 1.0, true)

    -- 전이(Transition) 추가
    -- Idle <-> Walking/Running/Backwards
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_WALKING, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_IDLE, 0.2)
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_WALKING_BACKWARDS, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_IDLE, 0.2)
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_RUNNING, 0.2)
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_IDLE, 0.2)

    -- Walking <-> Running
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_RUNNING, 0.15)
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_WALKING, 0.15)

    -- Walking <-> Walking Backwards
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_WALKING_BACKWARDS, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_WALKING, 0.2)

    -- Idle <-> Strafing (Walking)
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_STRAFE_LEFT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_IDLE, 0.2)
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_STRAFE_RIGHT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_IDLE, 0.2)

    -- Idle <-> Strafing (Running)
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_STRAFE_LEFT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_IDLE, 0.2)
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_STRAFE_RIGHT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_IDLE, 0.2)

    -- Strafe Walking <-> Strafe Running
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_STRAFE_LEFT, 0.15)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_STRAFE_LEFT_WALK, 0.15)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_STRAFE_RIGHT, 0.15)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_STRAFE_RIGHT_WALK, 0.15)

    -- Walking <-> Strafing (same speed)
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_STRAFE_LEFT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_WALKING, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_STRAFE_RIGHT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_WALKING, 0.2)

    -- Running <-> Strafing (same speed)
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_STRAFE_LEFT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_RUNNING, 0.2)
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_STRAFE_RIGHT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_RUNNING, 0.2)

    -- Left <-> Right strafing
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_STRAFE_RIGHT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_STRAFE_LEFT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_STRAFE_RIGHT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_STRAFE_LEFT, 0.2)

    -- Running <-> Walking Backwards
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_WALKING_BACKWARDS, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_RUNNING, 0.2)

    -- Walking Backwards <-> Strafing (Walking)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_STRAFE_LEFT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_WALKING_BACKWARDS, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_STRAFE_RIGHT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_WALKING_BACKWARDS, 0.2)

    -- Walking Backwards <-> Strafing (Running)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_STRAFE_LEFT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_WALKING_BACKWARDS, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_STRAFE_RIGHT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_WALKING_BACKWARDS, 0.2)

    -- Walking <-> Strafing (cross-speed: Walking to Running Strafe)
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_STRAFE_LEFT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_WALKING, 0.2)
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_STRAFE_RIGHT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_WALKING, 0.2)

    -- Running <-> Strafing (cross-speed: Running to Walking Strafe)
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_STRAFE_LEFT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_RUNNING, 0.2)
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_STRAFE_RIGHT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_RUNNING, 0.2)

    -- Cross-strafing (Left Walk <-> Right Run)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_STRAFE_RIGHT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_STRAFE_LEFT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_STRAFE_LEFT, 0.2)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_STRAFE_RIGHT_WALK, 0.2)

    -- Any -> Jumping
    StateMachine:AddTransitionByName(STATE_IDLE, STATE_JUMPING, 0.1)
    StateMachine:AddTransitionByName(STATE_WALKING, STATE_JUMPING, 0.1)
    StateMachine:AddTransitionByName(STATE_WALKING_BACKWARDS, STATE_JUMPING, 0.1)
    StateMachine:AddTransitionByName(STATE_RUNNING, STATE_JUMPING, 0.1)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT_WALK, STATE_JUMPING, 0.1)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT_WALK, STATE_JUMPING, 0.1)
    StateMachine:AddTransitionByName(STATE_STRAFE_LEFT, STATE_JUMPING, 0.1)
    StateMachine:AddTransitionByName(STATE_STRAFE_RIGHT, STATE_JUMPING, 0.1)

    -- Jumping -> Any movement state
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_IDLE, 0.2)
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_WALKING, 0.2)
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_WALKING_BACKWARDS, 0.2)
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_RUNNING, 0.2)
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_STRAFE_LEFT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_STRAFE_RIGHT_WALK, 0.2)
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_STRAFE_LEFT, 0.2)
    StateMachine:AddTransitionByName(STATE_JUMPING, STATE_STRAFE_RIGHT, 0.2)

    -- 초기 상태: Idle
    StateMachine:SetState(STATE_IDLE, 0.0)

    print("[PlayerController] Animation State Machine setup complete!")
end

function Tick(DeltaTime)
    if not Character or not StateMachine then
        return
    end

    -- 이전 프레임 이동 상태 저장
    local wasMoving = bIsMoving
    local wasRunning = bIsRunning

    -- 현재 프레임 이동 상태 초기화
    bIsMoving = false
    bIsRunning = false
    MoveDirection = MOVE_NONE

    -- Shift 키로 달리기
    if InputManager:IsKeyDown('SHIFT') then
        bIsRunning = true
    end

    -- WASD 입력 처리
    HandleMovementInput(DeltaTime)

    -- 스페이스바로 점프
    if InputManager:IsKeyPressed(' ') then
        if Character:CanJump() then
            Character:Jump()
            bWasJumping = true
            StateMachine:SetState(STATE_JUMPING, -1.0)
        end
    end

    -- 점프 중단 (스페이스를 뗐을 때)
    if InputManager:IsKeyReleased(' ') then
        Character:StopJumping()
    end

    -- C키로 웅크리기/일어서기 토글
    if InputManager:IsKeyPressed('C') then
        if Character:IsCrouched() then
            Character:UnCrouch()
        else
            Character:Crouch()
        end
    end

    -- 애니메이션 상태 업데이트
    UpdateAnimationState()
end

function HandleMovementInput(DeltaTime)
    -- Z-up left-handed 좌표계에서:
    -- Forward = Y축 (0,1,0), Right = X축 (1,0,0)
    -- 따라서 W/S는 Right, A/D는 Forward를 사용

    local speed = bIsRunning and RunSpeed or MovementSpeed
    local forwardInput = 0.0
    local rightInput = 0.0

    -- 전후 입력 처리 (W/S)
    if InputManager:IsKeyDown('W') then
        rightInput = rightInput + 1.0
    end
    if InputManager:IsKeyDown('S') then
        rightInput = rightInput - 1.0
    end

    -- 좌우 입력 처리 (A/D)
    if InputManager:IsKeyDown('A') then
        forwardInput = forwardInput - 1.0
    end
    if InputManager:IsKeyDown('D') then
        forwardInput = forwardInput + 1.0
    end

    -- 입력이 있으면 이동 처리
    if forwardInput ~= 0.0 or rightInput ~= 0.0 then
        bIsMoving = true

        -- 대각선 이동 시 속도 정규화 (√2로 나눔)
        local inputMagnitude = math.sqrt(forwardInput * forwardInput + rightInput * rightInput)
        if inputMagnitude > 1.0 then
            forwardInput = forwardInput / inputMagnitude
            rightInput = rightInput / inputMagnitude
        end

        -- 이동 적용
        if forwardInput ~= 0.0 then
            Character:MoveForward(forwardInput * speed)
        end
        if rightInput ~= 0.0 then
            Character:MoveRight(rightInput * speed)
        end

        -- 애니메이션을 위한 주 방향 결정
        if rightInput > 0 then
            -- 전진 중: 주 방향에 따라 애니메이션 결정 (달리기 가능)
            if math.abs(rightInput) > math.abs(forwardInput) then
                MoveDirection = MOVE_FORWARD
            else
                if forwardInput > 0 then
                    MoveDirection = MOVE_RIGHT
                else
                    MoveDirection = MOVE_LEFT
                end
            end
        elseif rightInput < 0 then
            -- 후진 중: 좌우 스트레이프 걷기 사용 (달리기 불가)
            bIsRunning = false  -- 후진 대각선 이동 시 달리기 방지
            if forwardInput > 0 then
                MoveDirection = MOVE_LEFT  -- 오른쪽(D) 입력 시 왼쪽을 봄
            elseif forwardInput < 0 then
                MoveDirection = MOVE_RIGHT  -- 왼쪽(A) 입력 시 오른쪽을 봄
            else
                MoveDirection = MOVE_BACKWARD
            end
        else
            -- 좌우만 이동 (달리기 가능)
            if forwardInput > 0 then
                MoveDirection = MOVE_RIGHT
            else
                MoveDirection = MOVE_LEFT
            end
        end
    else
        bIsMoving = false
        MoveDirection = MOVE_NONE
    end
end

function UpdateAnimationState()
    if not StateMachine then
        return
    end

    local currentState = StateMachine:GetCurrentStateName()

    -- DEBUG: Print current state and movement flags
    -- print(string.format("[AnimDebug] Current=%s, bIsMoving=%s, bIsRunning=%s", currentState, tostring(bIsMoving), tostring(bIsRunning)))

    -- 점프 중이면 점프 애니메이션이 끝날 때까지 대기
    if currentState == STATE_JUMPING then
        -- 점프 애니메이션이 거의 끝났는지 확인
        local jumpTime = StateMachine:GetStateTime(STATE_JUMPING)
        -- 점프 애니메이션이 충분히 재생되었으면 다른 상태로 전환 가능
        if jumpTime > 0.8 then -- 80% 이상 재생됨
            bWasJumping = false
        else
            return -- 점프 애니메이션 재생 중
        end
    end

    -- Determine desired state based on input and movement direction
    local desiredState = STATE_IDLE
    if bIsMoving then
        if MoveDirection == MOVE_FORWARD then
            -- 전진: 걷기 또는 달리기
            if bIsRunning then
                desiredState = STATE_RUNNING
            else
                desiredState = STATE_WALKING
            end
        elseif MoveDirection == MOVE_BACKWARD then
            -- 후진: 뒤로 걷기 애니메이션 사용
            desiredState = STATE_WALKING_BACKWARDS
        elseif MoveDirection == MOVE_LEFT then
            -- 좌측 스트레이프
            if bIsRunning then
                desiredState = STATE_STRAFE_LEFT
            else
                desiredState = STATE_STRAFE_LEFT_WALK
            end
        elseif MoveDirection == MOVE_RIGHT then
            -- 우측 스트레이프
            if bIsRunning then
                desiredState = STATE_STRAFE_RIGHT
            else
                desiredState = STATE_STRAFE_RIGHT_WALK
            end
        end
    end

    -- Only transition if we need to change states
    if currentState ~= desiredState then
        -- print(string.format("[AnimDebug] Transitioning from %s to %s", currentState, desiredState))
        StateMachine:SetState(desiredState, -1.0)
    end
end

function EndPlay()
    print("[PlayerController] EndPlay called")
    Character = nil
    SkeletalMeshComp = nil
    StateMachine = nil
end

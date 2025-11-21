-- GameState
-- Init     : Tile, 동상, Player Spawn (각각의 Spawner에서 Tick 검사-> State는 PlayerSpawner만 바꾼다)
-- Playing
-- End

function BeginPlay()
    GetCameraManager():StartGamma(1.0 /2.2)
    GlobalConfig.GameState = "Intro"
    GlobalConfig.UIManager = SpawnPrefab("Data/Prefabs/UIManager.prefab")
    InputManager:SetCursorVisible(true)

    GlobalConfig.Camera1 = FindObjectByName("Camera1")
    GlobalConfig.Camera2 = FindObjectByName("Camera2")
    GlobalConfig.Camera3 = FindObjectByName("Camera3")
    GlobalConfig.Camera4 = FindObjectByName("Camera4")
    GlobalConfig.Camera5 = FindObjectByName("Camera5")

    StartCoroutine(MoveCameras)

    -- GetCameraManager():StartCameraShake(20, 0.1, 0.1,40)
    -- Color(0.0, 0.2, 0.4, 1.0)
    -- GetCameraManager():StartFade(5.0, 0, 1.0, Color(0.0, 0.2, 0.4, 1.0), 0)
    -- GetCameraManager():StartLetterBox(2.0, 1.777, 0, Color(0.0, 0.2, 0.4, 1.0))
    -- GetCameraManager():StartVignette(4.0, 0.2, 0.5, 10.0, 2.0, Color(0.9, 0.0, 0.2, 0.0), 0)
end

function MoveCameras()    
    GetCameraManager():SetViewTarget(GetComponent(GlobalConfig.Camera1, "UCameraComponent"))
    coroutine.yield("wait_time", 4)
    DeleteObject(GlobalConfig.teamName)
    GetCameraManager():SetViewTargetWithBlend(GetComponent(GlobalConfig.Camera2, "UCameraComponent"), 4)
    coroutine.yield("wait_time", 4)
    GetCameraManager():SetViewTargetWithBlend(GetComponent(GlobalConfig.Camera3, "UCameraComponent"), 4)
    coroutine.yield("wait_time", 4)
    GetCameraManager():SetViewTargetWithBlend(GetComponent(GlobalConfig.Camera4, "UCameraComponent"), 4)
    coroutine.yield("wait_time", 2.5)
    GetCameraManager():SetViewTargetWithBlend(GetComponent(GlobalConfig.Camera5, "UCameraComponent"), 2.5)
    coroutine.yield("wait_time", 4)
    GetCameraManager():SetViewTargetWithBlend(GetComponent(GlobalConfig.Camera1, "UCameraComponent"), 4)
    coroutine.yield("wait_time", 4)
    GlobalConfig.GameState = "Init"
    
    GetCameraManager():StartGamma(3.0 /2.2)
end

function EndPlay()
    InputManager:SetCursorVisible(true)
    GlobalConfig.GameState = "End"
end

function OnBeginOverlap(OtherActor)
end

function OnEndOverlap(OtherActor)
end

CurVisibilty = true
function Tick(dt)
    if GlobalConfig.GameState == "Init" then
        InputManager:SetCursorVisible(true)
        GlobalConfig.GameState = "Start"
        GlobalConfig.CoachLevel = 1        
    
    elseif GlobalConfig.GameState == "Start" then
        if InputManager:IsKeyDown("R") then
             
            if GlobalConfig and GlobalConfig.DestroyAllFireball then
                GlobalConfig.DestroyAllFireball()
            end
            
            if GlobalConfig and GlobalConfig.AudioComp then
                GlobalConfig.AudioComp:PlayOneShot(0)
                print("call first 대사")
            end
            SpawnPrefab("Data/Prefabs/Player.prefab")
            GlobalConfig.PlayerState = "Alive"
            GlobalConfig.GameState = "Playing" 
            GlobalConfig.CoachLevel = 1  
            InputManager:SetCursorVisible(false)
        end
        
    elseif GlobalConfig.GameState == "Playing" then
        if InputManager:IsKeyPressed("C") then
            SetCursorVisible(not CurVisibilty)
        end
        if GlobalConfig.PlayerState == "Dead" then
            SetCursorVisible(true)
            GlobalConfig.GameState = "End"
            StartCoroutine(WaitAndInit)
        end

    elseif GlobalConfig.GameState == "End" then
        InputManager:SetCursorVisible(true)
    end
end

function WaitAndInit()
    coroutine.yield("wait_time", 1.0)
    GlobalConfig.GameState = "Init"
end

function SetCursorVisible(Show)
    InputManager:SetCursorVisible(Show)
    CurVisibilty = Show
end

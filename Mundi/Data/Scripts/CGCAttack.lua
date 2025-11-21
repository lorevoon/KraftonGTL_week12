-- Minimal: create FireballSpawnArea count equal to CoachLevel.
-- Only create on increases; do not delete.

local SpawnedCount = 0
local CurrentLevel = 0
local SpawnerCache = {} 

local function EnsureSpawned(target)
    target = math.max(0, tonumber(target or 0) or 0)
    while SpawnedCount < target do
        local s = SpawnPrefab("Data/Prefabs/FireballSpawnArea.prefab")
        SpawnerCache[#SpawnerCache + 1] = s

        s.Location = Obj.Location + Vector(4.5 * target ,0,0)
        if s then
            SpawnedCount = SpawnedCount + 1
        else
            break
        end
    end
end
local function PruneCacheTo(n) 
    -- 뒤에서부터 제거
    for i = #SpawnerCache, n + 1, -1 do 
        DeleteObject(SpawnerCache[i])
        SpawnerCache[i] = nil
        SpawnedCount = SpawnedCount - 1
    end
end


function BeginPlay() 
    CurrentLevel = GlobalConfig.CoachLevel or 0
    EnsureSpawned(CurrentLevel)
end

function EndPlay() 
end

function Tick(dt)


    local NewLevel = GlobalConfig.CoachLevel or 0    
    if NewLevel == 1 then 
        PruneCacheTo(1)
    end

    if GlobalConfig.GameState ~= "Playing" then
        return
    end

    if NewLevel ~= CurrentLevel then
        EnsureSpawned(NewLevel)
        CurrentLevel = NewLevel
    end


end


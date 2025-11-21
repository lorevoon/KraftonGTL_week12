function BeginPlay()
     
end

function EndPlay()
     
end

function OnEndOverlap(OtherActor)
end

function OnBeginOverlap(OtherActor)
     
    --[[Obj:PrintLocation()]]--
     
    GlobalConfig.ResetFireballs(OtherActor)
end

function Tick(dt)
    --Obj.Location = Obj.Location + Obj.Velocity * dt
    --[[Obj:PrintLocation()]]--
    --[[print("[Tick] ")]]--
end
local USE_ACTOR_LOCAL_DIRECTION = true

local function flatten_and_normalize(v)
    if v == nil then
        return Vec3(0.0, 0.0, 0.0)
    end

    local flat = Vec3(v.X, v.Y, 0.0)
    if flat:Length() <= 0.0001 then
        return Vec3(0.0, 0.0, 0.0)
    end

    return flat:Normalized()
end

local function get_fallback_basis()
    if USE_ACTOR_LOCAL_DIRECTION and obj ~= nil then
        return flatten_and_normalize(obj.Forward), flatten_and_normalize(obj.Right)
    end

    return Vec3(1.0, 0.0, 0.0), Vec3(0.0, 1.0, 0.0)
end

local function read_move_axes()
    local x = 0.0
    local y = 0.0

    if Input.GetKey("W") then y = y + 1.0 end
    if Input.GetKey("S") then y = y - 1.0 end
    if Input.GetKey("D") then x = x + 1.0 end
    if Input.GetKey("A") then x = x - 1.0 end

    return x, y
end

local function make_move_direction(x, y)
    if x == 0.0 and y == 0.0 then
        return Vec3(0.0, 0.0, 0.0)
    end

    local forward, right = get_fallback_basis()
    return (forward * y + right * x):Normalized()
end

local function move_character(x, y)
    if obj == nil then
        return
    end

    if x == 0.0 and y == 0.0 then
        return
    end

    local direction = make_move_direction(x, y)
    if direction:Length() <= 0.0001 then
        return
    end

    obj:CallFunction("AddMovementInput", direction, 1.0)
end

function Tick(dt)
    local x, y = read_move_axes()
    move_character(x, y)
end

ScriptSystem = {
	GetEntity = function(name)
		local ent = GetObjectByName(name);
		if not ent then return nil; end
		return Entity(ent)
	end,
	GetCameraPos = function()
		local x, y, z = GetCameraPos()
		return { x = x , y = y , z = z }
	end,
	SpawnModel = function(name, path, pos, mass)
		mass = mass or 0.0;
		local x, y, z = pos.x, pos.y, pos.z;
		local ent = SpawnModel(name, path, x, y, z, mass);
		if not ent then return nil; end
		return Entity(ent)
	end
};

Entity = {};

_time = 0

function CreateClass(name)
	_G[name].__index = _G[name];
	setmetatable(_G[name], {
		__call = function(c, ...)
			local tbl={};
			setmetatable(tbl, c);
			c.new(tbl, ...);
			return tbl;
		end
	});
end

CreateClass("Entity")

function Entity:new(pointer)
	self.pointer = pointer;
end
function Entity:SetPos(pos)
	SetObjectPos(self.pointer, pos.x, pos.y, pos.z)
end
function Entity:GetPos()
	local x, y, z = GetObjectPos(self.pointer)
	return { x = x , y = y , z = z }
end

function Distance(a, b)
	local dx, dy, dz = a.x - b.x, a.y - b.y, a.z - b.z;
	return math.sqrt(dx^2 + dy^2 + dz^2)
end

function OnUpdate(dt)
	--Something(1, 2)
	--[[_time = _time + dt;
	if _time > 5 then
		local ent = ScriptSystem.GetEntity("Entity")
		if not ent then
			local pos = ScriptSystem.GetCameraPos();
			pos.x = pos.x - 10
			pos.y = pos.y - 10
			pos.z = pos.z - 1.9;
			ent = ScriptSystem.SpawnModel("Entity"..os.clock(), "stuff/statue.obj", pos);
			_time = 0;
			return;
		end
	end--]]
end
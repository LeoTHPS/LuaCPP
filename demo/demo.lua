local a = 0;
local b = 0;

local function the_thing(a, b)
	return a + b;
end

print(do_the_thing(a, b, the_thing));

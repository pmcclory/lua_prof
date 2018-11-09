extern = require "test2"

local function subfunction3()
  for i=10,1,-1 do
    print(i)
  end
  extern.foo()
end

local function subfunction2()
  subfunction3()
end

local function subfunction()
  subfunction2()
end

local function thisisatest()
  subfunction()
end

thisisatest()

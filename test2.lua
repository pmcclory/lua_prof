function externalFunc()
  while true do
    print("hello world - extern")
    print("hello world 2")
  end
end

local mod = {}
mod['foo'] = externalFunc
return mod

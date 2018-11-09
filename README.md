usage:

start a lua program:

```
lua ./test.lua
```

get a stack trace:

```
➜  > ./trace.sh $(pidof lua)
???
@./test2.lua:3
@./test.lua:7
@./test.lua:11
@./test.lua:15
@./test.lua:19
@./test.lua:22
???
```

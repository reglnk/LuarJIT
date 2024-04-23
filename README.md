# README for LuaJIT 2.1

LuaJIT is a Just-In-Time (JIT) compiler for the Lua programming language.

Project Homepage: https://luajit.org/

LuaJIT is Copyright (C) 2005-2023 Mike Pall.
LuaJIT is free software, released under the MIT license.
See full Copyright Notice in the COPYRIGHT file or in luajit.h.

Documentation for LuaJIT is available in HTML format.
Please point your favorite browser to:

 doc/luajit.html

# Changes:

The new parser mode can be enabled with `[luar]` tag, and switched back to
classical Lua with `[lua]`. The changes only affect the first mode.
Luar syntax (as I've called this dialect) is more C-like, with support for curly brackets for fundamental parts: loops, branches, and functions.

## 1. Functions

Keyword `function` is replaced with `fn`, meanwhile the `function` becomes not reserved,
and vice versa for classical Lua mode. Some function definition examples are listed below.

```lua
[luar]
-- all following statements define a function with empty body 
local fn empty1();
local fn empty2() {}
local empty3 = fn();
local empty4 = fn() {}

function = fn() { return 8; }
print(function()); --> 8
[lua]
fn = function() return 7 end
print(fn()); --> 7
print(_G["function"]()); --> 8
[luar]
print(_G["fn"]()) --> 7
```

A function may even not have braces, but the behaviour is changed:
```lua
[luar]
-- these 2 are equivalent
local fn square(x) x * x;
local fn square(x) {
    return x * x
}

-- these are too
local fn length(x, y, z) math.sqrt (
    x * x + y * y + z * z
)
local fn length(x, y, z) {
    return math.sqrt(x * x + y * y + z * z);
}
```

## 2. Flexible branches syntax

If some rounded bracked are wrapped about the condition, it's not necessary to write `then`, but you still may.
Otherwise if brackets are missing, `then` is required. This is made to minimize number of brackets and not lose
code readability compared to classical syntax.
`elseif` keyword is saved and may be used, but, thanks to some C-like change, you can use `else if`.
```lua
[luar]
if (foo) {
    -- do something...
}
elseif (bar) {
    -- do other things...
}
else if baz
    then baz();
else print('not found');
```

The noticed C-like change is that only 1 statement is required in places, where in classical Lua it has to be ended with `end` token.
So, `then` may precede curly brackets or a single statement, the same applies to `do`.

## 3. Loops

`repeat-until` loop is almost untouched because it hasn't any `end` tokens.
The only change is the possibility to add {} braces inside, for consistent C style.
```lua
[luar]
repeat {
    print('hello world!')
}
until #io.read() > 0;
```
`for` and `while` are changed so that, instead of body and `end` token, you should write either
- body, or
- body in curly braces

Like this:
```lua
[luar]
for i = 1, 50 do
    print(i);

for i = 1, 50 do {
    print(i)
}

for i, v in ipairs(numbers) do print(i, v)
for i, v in ipairs(numbers) do {
    print(i, v)
}
```

## 4. The block {} may be used alone.

So, instead of writing
```lua
do
    local v = dosmth1();
    if v ~= 0 then
        dosmth2();
    end
end
```
You can write:
```lua
[luar]
{
    local v = dosmth1();
    if v ~= 0 then
        dosmth2();
}
```

## 5. Conslusion

The new syntax doesn't need `end` keyword.
Also it's not made indendation-dependent, meanwhile is not ambiguous, because the parser
always knows where the statement ends. So the following is valid code for both versions:
```lua
local i = something() print(i)
```
Semicolons remained optional, but became more useful for defining empty functions.
The following 2 code pieces are not the same:
```lua
[luar]
local f = fn()
print("hola")
```
```lua
[luar]
local f = fn();
print("hola")
```
Only the second one will output 'hola', while the first is equivalent to
```lua
local f = function()
    return print("hola")
end
```
Also note that, in order not to parse `[luar]` or `[lua]` as indexing, it should be sometimes preceded with semicolon (as shown in first example).

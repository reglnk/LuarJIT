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

The parser mode can be switched to new style with `[luar]` tag, and switched back to
classical Lua with `[lua]`. New style is default. The changes don't affect usual Lua mode as far as possible.
Luar syntax (as I've called this dialect) is more C-like, with support for curly brackets for fundamental parts:
loops, branches, and functions; and custom operators.

## 1. Comments

single-line: `//`
multiline: `[//` + `=` * N + `[` ... `]//` + `=` * N + `]`

For example:
```luar
[//===[
This part is commented.
]//]
This is too.
]//====]
This too.
]//===]
[//[ end of previous multiline comment ]//]
```

## 2. Functions

Keyword `function` is replaced with `fn`, meanwhile the `function` becomes not reserved,
and vice versa for classical Lua mode. Some function definition examples are listed below.

```luar
// all following statements define a function with empty body
local fn empty1();
local fn empty2() {}
local empty3 = fn();
local empty4 = fn() {};

fn function() { return 8; }
[//[ or so:
function = fn() { return 8; };
]//]
print(function()); // 8
[lua]
fn = function() return 7 end
print(fn()); --> 7
print(_G["function"]()); --> 8
[luar]
print(_G["fn"]()) // 7
```

A function may even not have braces, but the behaviour is changed:
```luar
// these 2 are equivalent
local fn square(x) x * x;
local fn square(x) {
    return x * x
}

// these are too
local fn length(x, y, z) math.sqrt (
    x * x + y * y + z * z
)
local fn length(x, y, z) {
    return math.sqrt(x * x + y * y + z * z);
}

print(square(5)); // 25
print(length(7, 7, 7)); // 12.124355652982
```

## 3. Flexible branches syntax

If some rounded bracked are wrapped about the condition, it's not necessary to write `then`, but you still may.
Otherwise if brackets are missing, `then` is required. This is made to minimize number of brackets and not lose
code readability compared to classical syntax.
`elseif` keyword is saved and may be used, but, thanks to some C-like change, you can use `else if`.
```luar
if (foo) {
    // do something...
}
elseif (bar) {
    // do other things...
}
else if baz
    then baz();
else print('not found');
```

## 4. Loops

`repeat-until` loop is almost untouched because it hasn't any `end` tokens.
The only change is the possibility to add {} braces inside, for consistent C style.
```luar
repeat {
    print('hello world!')
}
until #io.read() > 0;
```
`for` and `while` are changed so that, instead of body and `end` token, you should write either
- a single statement, or
- body in curly braces

Like this:
```luar
for i = 1, 50 do
    print(i);

for i = 1, 50 do {
    print(i)
}

local numbers = {7, 8, 9};
for i, v in ipairs(numbers) do print(i, v)
for i, v in ipairs(numbers) do {
    print(i, v)
}
```

## 5. Alone {} block, like in C

Instead of writing
```lua
do
    local v = dosmth1()
    if v ~= 0 then
        dosmth2()
    end
end
```
You can write:
```luar
{
    local v = dosmth1();
    if v ~= 0 then
        dosmth2();
}
```

## 6. Custom operators

There are now really custom postfix and infix operators.
Let's divide them into identifier ones and symbolic ones.

Symbolic operators may consist of symbols `+-*/%~&|^!?=<>`, and can be either infix or postfix.
So far there's one limitation: you can define only one custom operator starting with `=`, and this is `===`.
This syntax is reserved for planned feature (for writing `obj.field =+ 20`).
So, the statement `foo =?? bar;` will just output a syntax error
(`??` will be considered a variable, and `bar` an infix operator, and its right operand is missing, that's it).

Some examples:
```luar
local fn &&(a, b) a and b;
local fn ||(a, b) a or b;
print(nil || 15); // 15
print(nil && 20); // nil
```
```luar
local obj = {
    v = 0
};
local fn ++(o) {
    o.v = o.v + 1;
    print(o.v);
}
obj++; // 1
obj++; // 2
```

All operators are encoded to identifiers starting with `__op_`, so can be defined in usual Lua:
```luar
[lua]
local bit = require "bit"
function __op_mnxmnxor(a, b)
    if type(a) == "number" then
        return bit.bxor(a, b)
    end
    local h = ""
    for i = 1, b do
        h = h .. a
    end
    return h
end
[luar]
print(7 ^&*^&* 7); // 0
print(8 ^&*^&* 7); // 15
print("qwer" ^&*^&* 5); // qwerqwerqwerqwerqwer

```
Custom operators have priority 1 (the lowest among operators) and are parsed left-to-right
(the same applies to custom identifier ones), so think twice before redefinition of existing operators.

Custom identifier operators consist of just a variable identifier (single TK_name) and can only be infixes.
My favorite usage:
```luar
local fn tail(a, b) b(a);

local result = 5
    tail fn(x) x * x
    tail fn(x) x * 2
    tail fn(x) "result is " .. x;

print(result); // result is 50
```
Say you need an operator to determite if some object is an instance of some class.
I think metatables are enough for demonstration:
```luar
local fn is(obj, t) (
    getmetatable(obj) == t
);

local Foo = {};
Foo.__index = Foo;
fn Foo: +=(val) {
    print(self.v);
    self.v = self.v + val;
}

local Bar = {};
Bar.__index = Bar;
fn Bar: +=(val) {
    self.v = self.v + val;
    print(self.v);
}

// define common += to work for both
fn +=(a, b) a: +=(b);

local obj1 = setmetatable({v = 0}, Foo);
local obj2 = setmetatable({v = 0}, Bar);

obj1 += 20; // 0
obj2 += 40; // 40

print(obj1 is Foo, obj2 is Bar); // true, true
print(obj1 is Bar, obj2 is Foo); // false, false
```

## 7. More on semicolons

They're no more so optional as in Lua, mostly because of identifier infix operators.
While the following won't generate an error and will parse as intended
(cause the reserved word cannot be a part of an operand):
```luar
local h = obj++
local j = obj--
```
The following will fail miserably:
```luar
h = obj++
j = obj--
```
Also don't forget that, in order not to parse `[luar]` or `[lua]` as indexing, they should sometimes be preceded with semicolon.

## 8. Conclusion

You might say it's better just to have a language with curly brackets and without these silly `;` everywhere,
and you would be right for Lua dialect, but I planned this as a something more.
You're always welcome to send issues, pull requests, yet more customized forks and ideas (contact me).
Basic syntax highlighting for KATE can be found in `misc` folder.

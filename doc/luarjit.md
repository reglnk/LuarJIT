# LuarJIT

A LuaJIT derivative with 2 frontends.
This document assumes Lua is well-known to the reader and aims only to highlight differences of the second frontend.

## Definitions and general information

Variable identifier, or simply - identifier, or `vID`: a name that can be used for Lua variable. The token TK_name stores that stuff.\
Syntax mode: a new global state variable that affects the behaviour of lexer and parser. It is stored in `global_State::pars::mode` field.

The modes are the following:
- Mode 0: Classical backwards-compatible Lua syntax mode. All code that is valid in plain LuaJIT should work in the same way in LuarJIT running the Mode 0.
- Mode 1: The mode that is most related to all these changes (the second frontend).

A language of the second frontend tends to
1. have more syntactic sugar than Lua;
2. be fully compatible with Lua, in bounds of LuaJIT VM.
\
Thus, being like Kotlin compared to Java.

2 syntax switching directives are accepted by the parser:
- `[lua]` tells to switch to Mode 0.
- `[luar]` tells to switch to Mode 1.
\
These affect the mode immediately when the statement is read, and to work so, they must be a beginning of statement
(the end is right there - no semicolon is needed after the directive).

2 new API functions have been added for manipulation of syntax mode:
```c
LUA_API int lua_getsyntaxmode(lua_State *L)
```
```c
LUA_API void lua_setsyntaxmode(lua_State *L, int mode)
```
The syntax mode affects luaL_loadfile, luaL_loadstring and all functions that parse code.\
However, luaL_loadstring also affects the syntax mode: if the file has extension ".luar", it switches to Mode 1 before loading, and switches back after.
Let this feature be called 'format detection'.
Builtin global function `__syntax_mode` is made ontop of 2 above functions to manipulate the mode in runtime, for example, preceding `load`.
Call it without arguments to get the current mode, or pass in the mode to set.
Some feature is planned to override the format detection behaviour for setting any syntax mode for `dofile` and `require` without limitations.

### For syntax description, the following 'description language' will be used:
Required part is written inside the `@<>` block.\
Optional part is written inside the `@[]` block.\
To escape the `>` or `]`, `\` may be used.\
Words written inside `@[]` have the direct meaning. For `@<>`, the meaning is descriptive.\
Thus,\
    `@[\]]` describes an optional symbol ']',\
    `@[function]` does an optional `function` token, and\
    `@<function>` does "some function applicable in given context".\
The `...` operator inside the brackets means that everything within these brackets preceding the `...` is repeated.\
To escape `...` operator, a single backslash should be used.\
Thus,\
    `@<string...>` describes 1 or more strings,\
    `@[@<string>...]` does 0 or more, and\
    `{@[@<string>: @<string> @[, @<string>: @<string> ...]]}` does something like a single-level primitive JSON.

## Reserved words

_(changes only)_
| mode 0     | mode 1   |
| ---------- | -------- |
| function   | fn       |
|            | operator |
|            | nameof   |
| end        |          |

Keyword `function` is replaced with `fn`, meanwhile the `function` becomes not reserved,
and vice versa for when switching to classical Lua mode.

## Comments

single-line: `//` \
multiline: `[//` + `=` * N + `[` ... `]//` + `=` * N + `]`

Example:
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

## Functions

The full definition syntax of functions, including operators:
```luar
@[local] fn @[operator @[<newindex>]] @<name>(@[@<arg>...]) { @[@<statement>...] } // 1
@[local] fn @[operator @[<newindex>]] @<name>(@[@<arg>...]) @<expression> // 2
@[local] fn @[operator @[<newindex>]] @<name>(@[@<arg>...]) // 3
```
1. A complete definition statement.
2. Syntactic sugar over (1), equivalent to
```luar
fn @[operator @[<newindex>]] @<name>(@[arg...]) { return @<expression> }
```
3. Syntactic sugar over (1), equivalent to
```luar
fn @[operator @[<newindex>]] @<name>(@[arg...]) {}
```

A lambda function, or anonymous function, can be expressed as follows:
```luar
fn (@[@<arg>...]) { @[@<statement>...] } // 1
fn (@[@<arg>...]) @<expression> // 2
fn (@[@<arg>...]) // 3
```

Explanation of blocks used:
- `@[operator @[<newindex>]]`: for mangling symbols, thus allowing to reference different symbols with the same name in different contexts.
  The names preceded with `operator<newindex>` have different mangling than ones preceded with (or having implied) `operator`.
- `@<name>`: A `vID` or a sequence of operator symbols. The second case implies the `operator` keyword.
- `(@[@<arg>...])`: Arguments list as in Lua.

### Examples
```luar

// variant 1
local fn empty1() {}
local fn empty2(a, b) {}
local fn add(a, b) { return a + b }
local fn qwer(a, b, c) {
    local f = a * b * 0.283;
    return f / c
}

// variant 2
local fn add(a, b) a + b;
local fn qwer(a, b, c) (a * b * 0.283) / c;

// variant 3
local fn empty1();
local fn empty2(a, b);

fn function() { return 8; }
function = fn() { return 8; };

print(function()); // 8

[lua]
fn = function() return 7 end
print(fn()); --> 7
print(_G["function"]()); --> 8

[luar]
print(_G["fn"]()) // 7

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

### Operators

There are now really custom postfix and infix operators.

1. Custom symbolic operators may consist of symbols `+-*/%~&|^!=<>.`, and can be either infix or postfix.
2. Ones that contain a dot (field operators) can only be infix.
3. Identifier operators consist of `vID` and can only be infix.
4. A dot in symbolic operator may only be the last symbol. This syntax is reserved for planned feature (for writing `obj.field =+ 20`).
5. Only one custom operator starting with `=` can be defined: `===`.
6. Custom operators have priority 1 (the lowest among operators) and are parsed left-to-right

For example, the statement `foo =<< bar;` will output a syntax error
(`<<` will be considered a variable, and `bar` an infix operator, and its right operand is missing, that's it).

Full operator calling syntax:
```luar
@<expr1> @<symbolic_op> @<expr2> // 1
@<expr1> @<symbolic_op> // 2
@<expr1> @<ident_op> @<expr2> //  3
@<expr1> @<field_op> @<TK_name1> = @<expr2> // 4
```
1. An expression, equivalent to `operator @<symbolic_op>(@<expr1>, @<expr2>)`
2. An expression, equivalent to `operator @<symbolic_op>(@<expr1>)`
3. An expression, equivalent to `operator @<ident_op>(@<expr1>, @<expr2>)`
4. An expression, equivalent to `operator<newindex> @<field_op>(@<expr1>, @<TK_name1 as string>, @<expr2>)`

#### Examples
```luar
local fn &&(a, b) a and b;
local fn ||(a, b) a or b;
print(nil || 15); // 15
print(nil && 20); // nil

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

##### Field operators
```luar
local fn operator ~.(obj, field) {
    return obj.__data[field]
}
local fn operator<newindex> ~.(obj, field, value) {
    obj.__data[field] = value;
    return value
}

local foo = {__data = {bar = 8, baz = 9}};
print(foo~.bar); // 8
print(foo~.baz); // 9
local result = foo~.bar = 27;
foo~.baz = 290;
print(foo~.bar, foo~.baz); // 27 290
assert(result == foo~.bar);
```

### Mangling
The code in Luar is generally transcribable to Lua, including operators (which use identifier symbols for encoding).
Hence, all operators' symbol names are mangled, and any name starting with `__LR` is reserved.
And so, the code for definition of Luar operator would look in Lua like following
(use at your own risk - the mangling may change at any time):
```lua
local bit = require "bit"
function __LRop_mnxmnxor(a, b)
    if type(a) == "number" then
        return bit.bxor(a, b)
    end
    local h = ""
    for i = 1, b do
        h = h .. a
    end
    return h
end

-- Usage
[luar]
print(7 ^&*^&* 7); // 0
print(8 ^&*^&* 7); // 15
print("qwer" ^&*^&* 5); // qwerqwerqwerqwerqwer
```

For convenient definition of custom operators as methods, the new keyword `nameof` is added.
When beginning a primary expression, it will make the symbol name parse as a constant string.
Some examples:
```luar
assert(nameof foo == "foo");
local foo = nameof operator foo;
local eadd = nameof +=;
local psub = nameof operator ->;
local tmul = nameof operator<newindex> *.;
```

Possible usage:
```luar
local fn defineStdOP(name) {
	_G[name] = fn(self, a) getmetatable(self)[name](self, a);
}
defineStdOP(nameof +=);
defineStdOP(nameof -=);

local obj = setmetatable({v = 0}, {
	[nameof +=] = fn(self, val) {self.v = self.v + val},
	[nameof -=] = fn(self, val) {self.v = self.v - val}
});
obj += 2;
print(obj.v); // 2
obj -= 3;
print(obj.v); // -1
```

### Extra examples

My favorite usage:
```luar
local fn operator |>(a, b) b(a);

local result = 5
    |> (fn(x) x * x)
    |> (fn(x) x * 2)
    |> (fn(x) "result is " .. x);

print(result); // result is 50
```

Say you need an operator to determite if some object is an instance of some class.
Perhaps metatables are enough for demonstration:
```luar
local fn operator is(obj, t) (
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

## Optional chaining operator

`?` operator is now added as part of language to be a part of primary expressions in pair with `.`, `:`, `[]`, `()` continuations.
The behaviour of `?.` and `?[]` combinations is similar to that ones in C#.
Generally, it checks if the value of an expression part before it is 'a false value', and if so, the result of primary expression is immediately set to false or nil.
Otherwise, the chain of primary expression is executed further, in the same way as if this operator was missing.
A primary statement with this operator cannot be a destination of assignment.

### Example

```luar
print(foo?.bar?.field); // nil
print(foo?["bar"]?["field"]); // nil

local foo = {};
print(foo?.bar?.field); // nil
print(foo?["bar"]?["field"] or "nil2"); // "nil2"

foo = {bar = {field = 358}};
print(foo?.bar?.field); // 358
print(foo?["bar"]?["field"]); // 358

foo.func?(); // nothing
foo.func = fn() { print("works"); };
foo.func?(); // "works"
```

## Miscellaneous

### More on semicolons

They're no more so optional as in Lua, mostly because of custom operators.
While the following won't generate an error and will parse as intended
(because the reserved word cannot be a part of an operand):
```luar
local h = obj++
local j = obj--
```
The following will fail miserably (there's currently no way to define an operator strictly either as infix or postfix):
```luar
h = obj++
j = obj--
```

Also don't forget that, in order not to parse the syntax switching directives as indexing, they should sometimes be preceded with semicolon.
```luar
[lua]
local q = settings.prop i = i + localset.prop
[luar] // localset.prop[luar] ??
```

### More on syntax mode

Here's the reference implementation of function to correctly import plain Lua modules from Luar code.
It's deprecated and useless after the update with 'format detection' feature.
Some changes to __syntax_mode are planned to make it possible to override this behaviour.
```luar
local require = require; fn luaimport(pkg)
{
	local prevmode = __syntax_mode();
	if (prevmode == 1)
		__syntax_mode(0);
	local stuff = require(pkg);
	__syntax_mode(prevmode);
	return stuff;
}
```

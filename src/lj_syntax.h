#ifndef lj_syntax_h
#define lj_syntax_h

#include <stddef.h>
#include <stdio.h>

#include "lua.h"

/* Switching language frontends. */
LUA_API int lua_getsyntaxmode(lua_State *L);
LUA_API void lua_setsyntaxmode(lua_State *L, int mode);

#endif

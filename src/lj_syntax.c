/*
** Language frontends manipulation.
*/

#define lj_syntax_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_lex.h"
#include "lj_parse.h"


LUA_API int lua_getsyntaxmode(lua_State *L)
{
  global_State *g = G(L);
  ParserState *ps = &g->pars;
  return ps->mode;
}

LUA_API void lua_setsyntaxmode(lua_State *L, int mode)
{
  lj_assertX(mode < 2, "invalid syntax mode");
  global_State *g = G(L);
  ParserState *ps = &g->pars;
  if (ps->mode == mode) return;
  ps->mode = mode;
  if (mode == 1) {
    ps->funcstr->reserved = 0;
    ps->end_str->reserved = 0;
    ps->fnstr->reserved = lj_lex_token2reserved(TK_function); /* so that TK_fn is unused in parser */
    ps->operstr->reserved = lj_lex_token2reserved(TK_operator);
    ps->nameof_str->reserved = lj_lex_token2reserved(TK_nameof);
  } else {
    ps->funcstr->reserved = lj_lex_token2reserved(TK_function);
    ps->end_str->reserved = lj_lex_token2reserved(TK_end);
    ps->fnstr->reserved = 0;
    ps->operstr->reserved = 0;
    ps->nameof_str->reserved = 0;
  }
}


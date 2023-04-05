#pragma once

// Compile standard ASSLIB library functions
#define ASS_STDLIB(AST_ID, ID, NAME, ARGS) ASSLIB_##ID,
enum AsslibID
{
#include "../../STDLIB"
};
#undef ASS_STDLIB

const char* get_asslib_ident( AsslibID id);

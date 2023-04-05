#include "asslib_support.h"

#define ASS_STDLIB(AST_ID, ID, NAME, ARGS) NAME,
static const char* kAsslibIdents[] = {
#include "../../STDLIB"
};
#undef ASS_STDLIB

const char*
get_asslib_ident( AsslibID id)
{
    return kAsslibIdents[id];
}

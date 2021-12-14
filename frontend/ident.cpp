#include <assert.h>
#include <stdio.h>
#include <frontend/keyword.h>

const char *keyword_string(int id)
{
#define   LINKABLE(xxx) xxx
#define UNLINKABLE(xxx) xxx
#define KEYWORD(name, keyword, ident) case KW_##name: { return ident; }

        switch (id) {
#include "../KEYWORDS"
        default:        { fprintf(stderr, "INVALID: %d or %c\n", id, id);  return "empty"; }
        }

#undef KEYWORD
#undef LINKABLE
#undef UNLINKABLE
}

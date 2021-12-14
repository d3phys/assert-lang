#ifndef KEYWORD_H
#define KEYWORD_H

enum keyword_type {
#define   LINKABLE(xxx) xxx
#define UNLINKABLE(xxx) xxx
#define KEYWORD(name, keyword, ident) KW_##name = keyword,

#include "../KEYWORDS"

#undef KEYWORD
#undef LINKABLE
#undef UNLINKABLE
};

const char *keyword_string(int id);


#endif /* KEYWORD_H */

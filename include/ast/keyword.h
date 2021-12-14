#ifndef AST_KEYWORD_H
#define AST_KEYWORD_H

enum ast_types {
#define AST(name, id, str) AST_##name = id,
#include "../../AST"
#undef AST
};


#endif /* AST_KEYWORD_H */

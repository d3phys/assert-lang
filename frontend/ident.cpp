#include <assert.h>
#include <stdio.h>
#include <frontend/grammar.h>

const char *ast_keyword_ident(int id)
{
        switch (id) {
        case AST_ELSE:   { return "else";      }
        case AST_IF:     { return "if";        }
        case AST_WHILE:  { return "while";     }
        case AST_RETURN: { return "return";    }
        case AST_GREAT:  { return ">";         }
        case AST_LOW:    { return "<";         }
        case AST_ASSIGN: { return "=";         }
        case AST_EQUAL:  { return "==";        }
        case AST_GEQUAL: { return ">=";        }
        case AST_NEQUAL: { return "!=";        }
        case AST_AND:    { return "&&";        }
        case AST_OR:     { return "||";        }
        case AST_ADD:    { return "+";         }
        case AST_SUB:    { return "-";         }
        case AST_DIV:    { return "/";         }
        case AST_MUL:    { return "*";         }
        case AST_POW:    { return "^";         }
        case AST_DEFINE: { return "define";    }
        case AST_INIT:   { return "initializer"; }
        case AST_STMT:   { return "statement"; }
        case AST_PARAM:  { return "parameter"; }
        case AST_CALL:   { return "call";      }
        case AST_CONST:  { return "const";     }
        case AST_DECISN: { return "decision";  }
        case AST_FUNC:   { return "function";  }
        case AST_SIN:    { return "sin";       }
        case AST_COS:    { return "cos";       }
        default:         { fprintf(stderr, "INVALID AST: %d or %c\n", id, id);  return "empty"; }
        }
}

const char *keyword_ident(int id)
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

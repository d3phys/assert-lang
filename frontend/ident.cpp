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
        case AST_STMT:   { return "statement"; }
        case AST_PARAM:  { return "parameter"; }
        case AST_CALL:   { return "call";      }
        case AST_DECISN: { return "decision";  }
        case AST_FUNC:   { return "function";  }
        default:         { fprintf(stderr, "INVALID: %d or %c\n", id);  return "empty"; }
        }
}

const char *keyword_ident(int id)
{
        switch (id) {
        case KW_IF:     { return "if";        }
        case KW_ELSE:   { return "else";      }
        case KW_WHILE:  { return "while";     }
        case KW_RETURN: { return "return";    }
        case KW_CONST:  { return "const";     }
        case KW_THEN:   { return "then";      }
        case KW_ASSIGN: { return "=";         }
        case KW_GREAT:  { return "<";         }
        case KW_LOW:    { return ">";         }
        case KW_EQUAL:  { return "==";        }
        case KW_GEQUAL: { return ">=";        }
        case KW_LEQUAL: { return "<=";        }
        case KW_NEQUAL: { return "!=";        }
        case KW_AND:    { return "&&";        }
        case KW_OR:     { return "||";        }
        case KW_ADD:    { return "+";         }
        case KW_SUB:    { return "-";         }
        case KW_DIV:    { return "/";         }
        case KW_MUL:    { return "*";         }
        case KW_POW:    { return "^";         }
        case KW_BEGIN:  { return "{";         } 
        case KW_END:    { return "}";         }
        case KW_SEP:    { return ";";         }
        case KW_OPEN:   { return "(";         }
        case KW_CLOSE:  { return ")";         }
        case KW_CALL:   { return "call";      }
        case KW_DEFINE: { return "define";    }
        case KW_STOP:   { return "$";         }
        default:        { fprintf(stderr, "INVALID: %d or %c\n", id);  return "empty"; }
        }
}

#include <frontend/grammar.h>

const char *toktostr(int id)
{
        switch (id) {
        case '+':    { return "+";     }
        case '-':    { return "-";     }
        case '/':    { return "/";     }
        case '*':    { return "*";     }
        case '^':    { return "^";     }
        case '(':    { return "(";     }
        case ')':    { return ")";     }
        case '{':    { return "{";     }
        case '}':    { return "}";     }
        case '=':    { return "=";     }
        case ';':    { return ";";     }
        case IF:     { return "if";    }
        case ELSE:   { return "else";  }
        case WHILE:  { return "while"; }
        case CONST:  { return "const"; }
        case GE_OP:  { return ">=";    }
        case LE_OP:  { return "<=";    }
        case EQ_OP:  { return "==";    }
        case NEQ_OP: { return "!=";    }
        case OR_OP:  { return "|";     }
        case AND_OP: { return "&";     }
        default:     { return nullptr; }
        }
}

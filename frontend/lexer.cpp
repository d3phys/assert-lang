#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <frontend/lexer.h>

enum states {
        INITIAL = 0,
        NUMBER  = 1,
        IDENT   = 2,
        FINAL   = 3,
};

enum signals {
        SYMBOL    = 0,
        WHSPACE   = 1,
        DIGIT     = 2,
        DOT       = 3,
        SEMICOLON = 4,
        END       = 5,
};

typedef void (*callback)(const char *str);

struct transition {
        states next;
        callback action;
};


void read_number(const char *str);
void read_ident (const char *str);

transition FSM[4][6] = {};

__attribute__((constructor))
static void init()
{
        FSM[INITIAL][SYMBOL]    = {IDENT,   nullptr};
        FSM[INITIAL][DOT]       = {IDENT,   nullptr};
        FSM[INITIAL][SEMICOLON] = {IDENT,   nullptr};
        FSM[INITIAL][WHSPACE]   = {INITIAL, nullptr};
        FSM[INITIAL][DIGIT]     = {NUMBER,  nullptr};
        FSM[INITIAL][END]       = {FINAL,   nullptr};

        FSM[NUMBER][SYMBOL]    = {INITIAL, read_number};
        FSM[NUMBER][DOT]       = {NUMBER,  nullptr};
        FSM[NUMBER][SEMICOLON] = {INITIAL, read_number};
        FSM[NUMBER][WHSPACE]   = {INITIAL, read_number};
        FSM[NUMBER][DIGIT]     = {NUMBER,  nullptr};
        FSM[NUMBER][END]       = {FINAL,   read_number};

        FSM[IDENT][SYMBOL]    = {IDENT,   nullptr};
        FSM[IDENT][DOT]       = {INITIAL, read_ident};
        FSM[IDENT][SEMICOLON] = {INITIAL, read_ident};
        FSM[IDENT][WHSPACE]   = {INITIAL, read_ident};
        FSM[IDENT][DIGIT]     = {IDENT,   nullptr};
        FSM[IDENT][END]       = {FINAL,   read_ident};

        FSM[FINAL][SYMBOL]    = {FINAL, nullptr};
        FSM[FINAL][END]       = {FINAL, nullptr};
        FSM[FINAL][DIGIT]     = {FINAL, nullptr};
        FSM[FINAL][WHSPACE]   = {FINAL, nullptr};
        FSM[FINAL][DOT]       = {FINAL, nullptr};
        FSM[FINAL][SEMICOLON] = {FINAL, nullptr};
}

void read_number(const char *str)
{
        assert(str);

        printf("NUMBER: %s\n", str);
}

void read_ident(const char *str)
{
        assert(str);

        printf("IDENTIFIER: %s\n", str);
}

void read_ident (const char *str);

signals get_signal(const char *str)
{
        assert(str);

        signals signal = SYMBOL;

        if (*str == '\0')
                signal = END;
        else if (*str == '.')
                signal = DOT;
        else if (*str == ';')
                signal = SEMICOLON;
        else if (isdigit(*str))
                signal = DIGIT;
        else if (isspace(*str))
                signal = WHSPACE;
        else 
                signal = SYMBOL;

        return signal;
}

int tokenize(const char *str)
{
        assert(str);

        states current = INITIAL;
        signals signal = END;

        const char *token = str;
        while (current != FINAL) {

                 signal  = get_signal(str);
                 states next = FSM[current][signal].next;

                 if (current == INITIAL)
                         token = str;

                 callback action = FSM[current][signal].action;
                 if (action)
                         action(token);
                 else 
                         str++;

                 current = next;
        }

        return 0;
}

int main()
{
        const char *str = "12.212h 32.21 he 321";
        tokenize(str);
        return 0;
}
















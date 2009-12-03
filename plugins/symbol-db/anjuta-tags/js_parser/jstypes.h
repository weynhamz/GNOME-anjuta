#ifndef JSTYPES_H
#define JSTYPES_H

#include <glib.h>

typedef enum JSNodeArity {
    PN_FUNC     = -3,
    PN_LIST     = -2,
    PN_TERNARY  =  3,
    PN_BINARY   =  2,
    PN_UNARY    =  1,
    PN_NAME     = -1,
    PN_NULLARY  =  0
} JSNodeArity;

enum {
	JSOP_FALSE,
	JSOP_TRUE,
	JSOP_NULL,
	JSOP_THIS,
};

typedef struct {
    long          begin;          /* first character and line of token */
    long          end;            /* index 1 past last char, last line */
} JSTokenPos;

typedef enum JSTokenType {
    TOK_ERROR = -1,                     /* well-known as the only code < EOF */
    TOK_EOF = 0,                        /* end of file */
    TOK_EOL = 1,                        /* end of line */
    TOK_SEMI = 2,                       /* semicolon */
    TOK_COMMA = 3,                      /* comma operator */
    TOK_ASSIGN = 4,                     /* assignment ops (= += -= etc.) */
    TOK_HOOK = 5, TOK_COLON = 6,        /* conditional (?:) */
    TOK_OR = 7,                         /* logical or (||) */
    TOK_AND = 8,                        /* logical and (&&) */
    TOK_BITOR = 9,                      /* bitwise-or (|) */
    TOK_BITXOR = 10,                    /* bitwise-xor (^) */
    TOK_BITAND = 11,                    /* bitwise-and (&) */
    TOK_EQOP = 12,                      /* equality ops (== !=) */
    TOK_RELOP = 13,                     /* relational ops (< <= > >=) */
    TOK_SHOP = 14,                      /* shift ops (<< >> >>>) */
    TOK_PLUS = 15,                      /* plus */
    TOK_MINUS = 16,                     /* minus */
    TOK_STAR = 17, TOK_DIVOP = 18,      /* multiply/divide ops (* / %) */
    TOK_UNARYOP = 19,                   /* unary prefix operator */
    TOK_INC = 20, TOK_DEC = 21,         /* increment/decrement (++ --) */
    TOK_DOT = 22,                       /* member operator (.) */
    TOK_LB = 23, TOK_RB = 24,           /* left and right brackets */
    TOK_LC = 25, TOK_RC = 26,           /* left and right curlies (braces) */
    TOK_LP = 27, TOK_RP = 28,           /* left and right parentheses */
    TOK_NAME = 29,                      /* identifier */
    TOK_NUMBER = 30,                    /* numeric constant */
    TOK_STRING = 31,                    /* string constant */
    TOK_OBJECT = 32,                    /* RegExp or other object constant */
    TOK_PRIMARY = 33,                   /* true, false, null, this, super */
    TOK_FUNCTION = 34,                  /* function keyword */
    TOK_EXPORT = 35,                    /* export keyword */
    TOK_IMPORT = 36,                    /* import keyword */
    TOK_IF = 37,                        /* if keyword */
    TOK_ELSE = 38,                      /* else keyword */
    TOK_SWITCH = 39,                    /* switch keyword */
    TOK_CASE = 40,                      /* case keyword */
    TOK_DEFAULT = 41,                   /* default keyword */
    TOK_WHILE = 42,                     /* while keyword */
    TOK_DO = 43,                        /* do keyword */
    TOK_FOR = 44,                       /* for keyword */
    TOK_BREAK = 45,                     /* break keyword */
    TOK_CONTINUE = 46,                  /* continue keyword */
    TOK_IN = 47,                        /* in keyword */
    TOK_VAR = 48,                       /* var keyword */
    TOK_WITH = 49,                      /* with keyword */
    TOK_RETURN = 50,                    /* return keyword */
    TOK_NEW = 51,                       /* new keyword */
    TOK_DELETE = 52,                    /* delete keyword */
    TOK_DEFSHARP = 53,                  /* #n= for object/array initializers */
    TOK_USESHARP = 54,                  /* #n# for object/array initializers */
    TOK_TRY = 55,                       /* try keyword */
    TOK_CATCH = 56,                     /* catch keyword */
    TOK_FINALLY = 57,                   /* finally keyword */
    TOK_THROW = 58,                     /* throw keyword */
    TOK_INSTANCEOF = 59,                /* instanceof keyword */
    TOK_DEBUGGER = 60,                  /* debugger keyword */
    TOK_XMLSTAGO = 61,                  /* XML start tag open (<) */
    TOK_XMLETAGO = 62,                  /* XML end tag open (</) */
    TOK_XMLPTAGC = 63,                  /* XML point tag close (/>) */
    TOK_XMLTAGC = 64,                   /* XML start or end tag close (>) */
    TOK_XMLNAME = 65,                   /* XML start-tag non-final fragment */
    TOK_XMLATTR = 66,                   /* XML quoted attribute value */
    TOK_XMLSPACE = 67,                  /* XML whitespace */
    TOK_XMLTEXT = 68,                   /* XML text */
    TOK_XMLCOMMENT = 69,                /* XML comment */
    TOK_XMLCDATA = 70,                  /* XML CDATA section */
    TOK_XMLPI = 71,                     /* XML processing instruction */
    TOK_AT = 72,                        /* XML attribute op (@) */
    TOK_DBLCOLON = 73,                  /* namespace qualified name op (::) */
    TOK_ANYNAME = 74,                   /* XML AnyName singleton (*) */
    TOK_DBLDOT = 75,                    /* XML descendant op (..) */
    TOK_FILTER = 76,                    /* XML filtering predicate op (.()) */
    TOK_XMLELEM = 77,                   /* XML element node type (no token) */
    TOK_XMLLIST = 78,                   /* XML list node type (no token) */
    TOK_YIELD = 79,                     /* yield from generator function */
    TOK_ARRAYCOMP = 80,                 /* array comprehension initialiser */
    TOK_ARRAYPUSH = 81,                 /* array push within comprehension */
    TOK_LEXICALSCOPE = 82,              /* block scope AST node label */
    TOK_LET = 83,                       /* let keyword */
    TOK_BODY = 84,                      /* synthetic body of function with
                                           destructuring formal parameters */
    TOK_RESERVED,                       /* reserved keywords */
    TOK_LIMIT                           /* domain size */
} JSTokenType;

#endif

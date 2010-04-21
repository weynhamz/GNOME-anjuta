
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 1

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 3 "./parser/Grammar.y"


/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *  Copyright (C) 2009 Maxim Ermilov <zaspire@rambler.ru>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include "jstypes.h"
#include "js-node.h"
#define YYMAXDEPTH 10000
#define YYENABLE_NLS 0

/* default values for bison */

void yyerror (char* msg);
int yylex(void *a);

#define YYERROR_VERBOSE 1 // SHOW NORMAL ERROR MESSAGE
#define YYDEBUG 1 // Set to 1 to debug a parse error.
#define jscyydebug 1 // Set to 1 to debug a parse error.

JSNode *global = NULL;
GList  *line_missed_semicolon = NULL;

static JSNode*
node_new (int type, int arity)
{
	JSNode *node = g_object_new (JS_TYPE_NODE, NULL);
	node->pn_type = type;
	node->pn_arity = arity;

	return node;
}

static void
node_correct_position (JSNode *self, JSNode *inner)
{
	if (!self)
		return;
	if (!inner)
		return;
	if (!self->pn_pos.begin)
		self->pn_pos.begin = inner->pn_pos.begin;
	if (!self->pn_pos.end)
		self->pn_pos.end = inner->pn_pos.end;
	if (inner->pn_pos.begin && self->pn_pos.begin > inner->pn_pos.begin)
		self->pn_pos.begin = inner->pn_pos.begin;
	if (self->pn_pos.end < inner->pn_pos.end)
		self->pn_pos.end = inner->pn_pos.end;
}

static int
node_get_line (JSNode *self)
{
	if (!self)
		return 0;
	return self->pn_pos.end;
}

static void
node_correct_position_end (JSNode *self, int end)
{
	if (!self->pn_pos.begin)
		self->pn_pos.begin = end;
	if (self->pn_pos.end < end)
		self->pn_pos.end = end;
}

static void
AUTO_SEMICOLON (int line)
{
	line_missed_semicolon = g_list_append (line_missed_semicolon, GINT_TO_POINTER (line));
}

#define PRINT_LINE /*printf("%s(%d)\n", __FILE__ , __LINE__)*/
#define PRINT_LINE_TODO /*printf("TODO:%s(%d)\n", __FILE__ , __LINE__)*/
#define PRINT_LINE_NOTNEED /*printf("NOTNEED:%s(%d)\n", __FILE__ , __LINE__)*/

//#define YYPARSE_PARAM globalPtr
//#define YYLEX_PARAM globalPtr



/* Line 189 of yacc.c  */
#line 180 "./parser/js-parser-y-tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NULLTOKEN = 258,
     TRUETOKEN = 259,
     FALSETOKEN = 260,
     BREAK = 261,
     CASE = 262,
     DEFAULT = 263,
     FOR = 264,
     NEW = 265,
     VAR = 266,
     CONSTTOKEN = 267,
     CONTINUE = 268,
     FUNCTION = 269,
     RETURN = 270,
     VOIDTOKEN = 271,
     DELETETOKEN = 272,
     IF = 273,
     THISTOKEN = 274,
     DO = 275,
     WHILE = 276,
     INTOKEN = 277,
     INSTANCEOF = 278,
     TYPEOF = 279,
     SWITCH = 280,
     WITH = 281,
     RESERVED = 282,
     THROW = 283,
     TRY = 284,
     CATCH = 285,
     FINALLY = 286,
     DEBUGGER = 287,
     IF_WITHOUT_ELSE = 288,
     ELSE = 289,
     EQEQ = 290,
     NE = 291,
     STREQ = 292,
     STRNEQ = 293,
     LE = 294,
     GE = 295,
     OR = 296,
     AND = 297,
     PLUSPLUS = 298,
     MINUSMINUS = 299,
     LSHIFT = 300,
     RSHIFT = 301,
     URSHIFT = 302,
     PLUSEQUAL = 303,
     MINUSEQUAL = 304,
     MULTEQUAL = 305,
     DIVEQUAL = 306,
     LSHIFTEQUAL = 307,
     RSHIFTEQUAL = 308,
     URSHIFTEQUAL = 309,
     ANDEQUAL = 310,
     MODEQUAL = 311,
     XOREQUAL = 312,
     OREQUAL = 313,
     OPENBRACE = 314,
     CLOSEBRACE = 315,
     IDENT_IN = 316,
     NUMBER = 317,
     STRING = 318,
     AUTOPLUSPLUS = 319,
     AUTOMINUSMINUS = 320
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 109 "./parser/Grammar.y"

    int intValue;
    JSNode* node;
    struct {
	char    *iname;
	JSTokenPos pos;
    } name;



/* Line 214 of yacc.c  */
#line 292 "./parser/js-parser-y-tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#ifndef YYPUSH_DECLS
#  define YYPUSH_DECLS
struct yypstate;
typedef struct yypstate yypstate;
enum { YYPUSH_MORE = 4 };

#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#if defined __STDC__ || defined __cplusplus
int yypush_parse (yypstate *yyps, int yypushed_char, YYSTYPE const *yypushed_val);
#else
int yypush_parse ();
#endif
#if defined __STDC__ || defined __cplusplus
int yypull_parse (yypstate *yyps);
#else
int yypull_parse ();
#endif
#if defined __STDC__ || defined __cplusplus
yypstate * yypstate_new (void);
#else
yypstate * yypstate_new ();
#endif
#if defined __STDC__ || defined __cplusplus
void yypstate_delete (yypstate *yyps);
#else
void yypstate_delete ();
#endif
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 337 "./parser/js-parser-y-tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  207
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1389

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  92
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  110
/* YYNRULES -- Number of rules.  */
#define YYNRULES  335
/* YYNRULES -- Number of states.  */
#define YYNSTATES  599

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   320

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    82,     2,     2,    69,    87,    90,     2,
      78,    79,    71,    72,    75,    85,    76,    67,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    80,    91,
      88,    81,    89,    70,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    83,    77,    84,    68,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    73,    66,    74,    86,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    15,    20,    21,    24,
      26,    28,    31,    33,    35,    37,    40,    42,    44,    46,
      50,    55,    61,    63,    65,    68,    71,    73,    77,    83,
      89,    95,    97,   101,   106,   108,   110,   114,   117,   119,
     121,   123,   125,   127,   131,   135,   139,   147,   156,   158,
     162,   164,   167,   171,   176,   178,   180,   182,   184,   188,
     192,   196,   202,   205,   210,   211,   213,   215,   218,   220,
     222,   224,   229,   233,   237,   239,   244,   248,   252,   254,
     257,   259,   262,   265,   268,   273,   277,   280,   283,   288,
     292,   295,   299,   301,   305,   307,   309,   311,   313,   315,
     318,   321,   323,   326,   329,   332,   335,   338,   341,   344,
     347,   350,   353,   356,   359,   362,   364,   366,   368,   370,
     372,   376,   380,   384,   386,   390,   394,   398,   400,   404,
     408,   410,   414,   418,   420,   424,   428,   432,   434,   438,
     442,   446,   448,   452,   456,   460,   464,   468,   472,   474,
     478,   482,   486,   490,   494,   496,   500,   504,   508,   512,
     516,   520,   522,   526,   530,   534,   538,   540,   544,   548,
     552,   556,   558,   562,   566,   570,   574,   576,   580,   582,
     586,   588,   592,   594,   598,   600,   604,   606,   610,   612,
     616,   618,   622,   624,   628,   630,   634,   636,   640,   642,
     646,   648,   652,   654,   658,   660,   664,   666,   672,   674,
     680,   682,   688,   690,   694,   696,   700,   702,   706,   708,
     710,   712,   714,   716,   718,   720,   722,   724,   726,   728,
     730,   732,   736,   738,   742,   744,   748,   750,   752,   754,
     756,   758,   760,   762,   764,   766,   768,   770,   772,   774,
     776,   778,   780,   782,   785,   789,   793,   797,   799,   802,
     806,   811,   813,   816,   820,   825,   829,   833,   835,   839,
     841,   844,   847,   850,   852,   855,   858,   864,   872,   880,
     888,   894,   904,   915,   923,   932,   942,   943,   945,   946,
     948,   951,   954,   958,   962,   965,   968,   972,   976,   979,
     982,   986,   990,   996,  1002,  1006,  1012,  1013,  1015,  1017,
    1020,  1024,  1029,  1032,  1036,  1040,  1044,  1048,  1053,  1061,
    1071,  1074,  1077,  1085,  1094,  1101,  1109,  1117,  1126,  1128,
    1132,  1133,  1135,  1136,  1138,  1140
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     200,     0,    -1,    61,    -1,    96,    -1,    96,    66,    94,
      -1,    67,    94,    67,    -1,    67,    94,    67,    93,    -1,
      -1,    96,    97,    -1,    98,    -1,   101,    -1,   101,    99,
      -1,    68,    -1,    69,    -1,   100,    -1,   100,    70,    -1,
      71,    -1,    72,    -1,    70,    -1,    73,    62,    74,    -1,
      73,    62,    75,    74,    -1,    73,    62,    75,    62,    74,
      -1,   102,    -1,    76,    -1,    77,    93,    -1,    77,    76,
      -1,   103,    -1,    78,    94,    79,    -1,    78,    70,    80,
      94,    79,    -1,    78,    70,    81,    94,    79,    -1,    78,
      70,    82,    94,    79,    -1,    93,    -1,    83,   104,    84,
      -1,    83,    68,   104,    84,    -1,    62,    -1,    93,    -1,
     104,    85,   104,    -1,   104,   104,    -1,     3,    -1,     4,
      -1,     5,    -1,    62,    -1,    63,    -1,    93,    80,   159,
      -1,    63,    80,   159,    -1,    62,    80,   159,    -1,    93,
      93,    78,    79,    59,   199,    60,    -1,    93,    93,    78,
     198,    79,    59,   199,    60,    -1,   106,    -1,   107,    75,
     106,    -1,   109,    -1,    59,    60,    -1,    59,   107,    60,
      -1,    59,   107,    75,    60,    -1,    19,    -1,   105,    -1,
     110,    -1,    93,    -1,    78,   163,    79,    -1,    83,   112,
      84,    -1,    83,   111,    84,    -1,    83,   111,    75,   112,
      84,    -1,   112,   159,    -1,   111,    75,   112,   159,    -1,
      -1,   113,    -1,    75,    -1,   113,    75,    -1,   108,    -1,
      95,    -1,   197,    -1,   114,    83,   163,    84,    -1,   114,
      76,    93,    -1,    10,   114,   120,    -1,   109,    -1,   115,
      83,   163,    84,    -1,   115,    76,    93,    -1,    10,   114,
     120,    -1,   114,    -1,    10,   116,    -1,   115,    -1,    10,
     116,    -1,   114,   120,    -1,   118,   120,    -1,   118,    83,
     163,    84,    -1,   118,    76,    93,    -1,   115,   120,    -1,
     119,   120,    -1,   119,    83,   163,    84,    -1,   119,    76,
      93,    -1,    78,    79,    -1,    78,   121,    79,    -1,   159,
      -1,   121,    75,   159,    -1,   116,    -1,   118,    -1,   117,
      -1,   119,    -1,   122,    -1,   122,    43,    -1,   122,    44,
      -1,   123,    -1,   123,    43,    -1,   123,    44,    -1,    17,
     127,    -1,    16,   127,    -1,    24,   127,    -1,    43,   127,
      -1,    64,   127,    -1,    44,   127,    -1,    65,   127,    -1,
      72,   127,    -1,    85,   127,    -1,    86,   127,    -1,    82,
     127,    -1,   124,    -1,   126,    -1,   125,    -1,   126,    -1,
     127,    -1,   129,    71,   127,    -1,   129,    67,   127,    -1,
     129,    87,   127,    -1,   128,    -1,   130,    71,   127,    -1,
     130,    67,   127,    -1,   130,    87,   127,    -1,   129,    -1,
     131,    72,   129,    -1,   131,    85,   129,    -1,   130,    -1,
     132,    72,   129,    -1,   132,    85,   129,    -1,   131,    -1,
     133,    45,   131,    -1,   133,    46,   131,    -1,   133,    47,
     131,    -1,   132,    -1,   134,    45,   131,    -1,   134,    46,
     131,    -1,   134,    47,   131,    -1,   133,    -1,   135,    88,
     133,    -1,   135,    89,   133,    -1,   135,    39,   133,    -1,
     135,    40,   133,    -1,   135,    23,   133,    -1,   135,    22,
     133,    -1,   133,    -1,   136,    88,   133,    -1,   136,    89,
     133,    -1,   136,    39,   133,    -1,   136,    40,   133,    -1,
     136,    23,   133,    -1,   134,    -1,   137,    88,   133,    -1,
     137,    89,   133,    -1,   137,    39,   133,    -1,   137,    40,
     133,    -1,   137,    23,   133,    -1,   137,    22,   133,    -1,
     135,    -1,   138,    35,   135,    -1,   138,    36,   135,    -1,
     138,    37,   135,    -1,   138,    38,   135,    -1,   136,    -1,
     139,    35,   136,    -1,   139,    36,   136,    -1,   139,    37,
     136,    -1,   139,    38,   136,    -1,   137,    -1,   140,    35,
     135,    -1,   140,    36,   135,    -1,   140,    37,   135,    -1,
     140,    38,   135,    -1,   138,    -1,   141,    90,   138,    -1,
     139,    -1,   142,    90,   139,    -1,   140,    -1,   143,    90,
     138,    -1,   141,    -1,   144,    68,   141,    -1,   142,    -1,
     145,    68,   142,    -1,   143,    -1,   146,    68,   141,    -1,
     144,    -1,   147,    66,   144,    -1,   145,    -1,   148,    66,
     145,    -1,   146,    -1,   149,    66,   144,    -1,   147,    -1,
     150,    42,   147,    -1,   148,    -1,   151,    42,   148,    -1,
     149,    -1,   152,    42,   147,    -1,   150,    -1,   153,    41,
     150,    -1,   151,    -1,   154,    41,   151,    -1,   152,    -1,
     155,    41,   150,    -1,   153,    -1,   153,    70,   159,    80,
     159,    -1,   154,    -1,   154,    70,   160,    80,   160,    -1,
     155,    -1,   155,    70,   159,    80,   159,    -1,   156,    -1,
     122,   162,   159,    -1,   157,    -1,   122,   162,   160,    -1,
     158,    -1,   123,   162,   159,    -1,    81,    -1,    48,    -1,
      49,    -1,    50,    -1,    51,    -1,    52,    -1,    53,    -1,
      54,    -1,    55,    -1,    57,    -1,    58,    -1,    56,    -1,
     159,    -1,   163,    75,   159,    -1,   160,    -1,   164,    75,
     160,    -1,   161,    -1,   165,    75,   159,    -1,   167,    -1,
     168,    -1,   171,    -1,   196,    -1,   176,    -1,   177,    -1,
     178,    -1,   179,    -1,   182,    -1,   183,    -1,   184,    -1,
     185,    -1,   186,    -1,   192,    -1,   193,    -1,   194,    -1,
     195,    -1,    59,    60,    -1,    59,   201,    60,    -1,    11,
     169,    91,    -1,    11,   169,     1,    -1,    93,    -1,    93,
     174,    -1,   169,    75,    93,    -1,   169,    75,    93,   174,
      -1,    93,    -1,    93,   175,    -1,   170,    75,    93,    -1,
     170,    75,    93,   175,    -1,    12,   172,    91,    -1,    12,
     172,     1,    -1,   173,    -1,   172,    75,   173,    -1,    93,
      -1,    93,   174,    -1,    81,   159,    -1,    81,   160,    -1,
      91,    -1,   165,    91,    -1,   165,     1,    -1,    18,    78,
     163,    79,   166,    -1,    18,    78,   163,    79,   166,    34,
     166,    -1,    20,   166,    21,    78,   163,    79,    91,    -1,
      20,   166,    21,    78,   163,    79,     1,    -1,    21,    78,
     163,    79,   166,    -1,     9,    78,   181,    91,   180,    91,
     180,    79,   166,    -1,     9,    78,    11,   170,    91,   180,
      91,   180,    79,   166,    -1,     9,    78,   122,    22,   163,
      79,   166,    -1,     9,    78,    11,    93,    22,   163,    79,
     166,    -1,     9,    78,    11,    93,   175,    22,   163,    79,
     166,    -1,    -1,   163,    -1,    -1,   164,    -1,    13,    91,
      -1,    13,     1,    -1,    13,    93,    91,    -1,    13,    93,
       1,    -1,     6,    91,    -1,     6,     1,    -1,     6,    93,
      91,    -1,     6,    93,     1,    -1,    15,    91,    -1,    15,
       1,    -1,    15,   163,    91,    -1,    15,   163,     1,    -1,
      26,    78,   163,    79,   166,    -1,    25,    78,   163,    79,
     187,    -1,    59,   188,    60,    -1,    59,   188,   191,   188,
      60,    -1,    -1,   189,    -1,   190,    -1,   189,   190,    -1,
       7,   163,    80,    -1,     7,   163,    80,   201,    -1,     8,
      80,    -1,     8,    80,   201,    -1,    93,    80,   166,    -1,
      28,   163,    91,    -1,    28,   163,     1,    -1,    29,   167,
      31,   167,    -1,    29,   167,    30,    78,    93,    79,   167,
      -1,    29,   167,    30,    78,    93,    79,   167,    31,   167,
      -1,    32,    91,    -1,    32,     1,    -1,    14,    93,    78,
      79,    59,   199,    60,    -1,    14,    93,    78,   198,    79,
      59,   199,    60,    -1,    14,    78,    79,    59,   199,    60,
      -1,    14,    78,   198,    79,    59,   199,    60,    -1,    14,
      93,    78,    79,    59,   199,    60,    -1,    14,    93,    78,
     198,    79,    59,   199,    60,    -1,    93,    -1,   198,    75,
      93,    -1,    -1,   201,    -1,    -1,   201,    -1,   166,    -1,
     201,   166,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   215,   215,   219,   220,   224,   225,   229,   230,   234,
     235,   236,   240,   241,   246,   247,   251,   252,   253,   254,
     255,   256,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   272,   276,   277,   281,   282,   283,   284,   288,   289,
     290,   291,   292,   296,   303,   309,   315,   316,   321,   326,
     340,   341,   342,   347,   354,   355,   356,   357,   358,   362,
     363,   364,   368,   369,   374,   375,   379,   380,   384,   385,
     386,   387,   388,   395,   405,   406,   407,   414,   418,   419,
     423,   424,   428,   435,   442,   443,   453,   460,   467,   468,
     478,   479,   483,   484,   493,   494,   498,   499,   503,   504,
     505,   509,   510,   511,   515,   516,   517,   518,   519,   520,
     521,   522,   523,   524,   525,   528,   529,   533,   534,   538,
     539,   540,   541,   545,   546,   548,   550,   555,   556,   557,
     561,   562,   564,   569,   570,   571,   572,   576,   577,   578,
     579,   583,   584,   585,   586,   587,   588,   589,   593,   594,
     595,   596,   597,   598,   603,   604,   605,   606,   607,   608,
     610,   615,   616,   617,   618,   619,   623,   624,   626,   628,
     630,   635,   636,   638,   639,   641,   646,   647,   651,   652,
     657,   658,   662,   663,   667,   668,   673,   674,   679,   680,
     684,   685,   690,   691,   696,   697,   701,   702,   707,   708,
     713,   714,   718,   719,   724,   725,   729,   730,   735,   736,
     741,   742,   747,   748,   759,   760,   771,   772,   783,   784,
     785,   786,   787,   788,   789,   790,   791,   792,   793,   794,
     798,   799,   803,   804,   808,   809,   813,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,   824,   825,   826,
     827,   828,   829,   833,   838,   847,   848,   852,   857,   864,
     874,   889,   894,   901,   903,   908,   909,   914,   919,   932,
     933,   941,   945,   949,   953,   954,   958,   960,   965,   966,
     967,   968,   970,   972,   974,   976,   981,   982,   986,   987,
     991,   992,   993,   994,   998,   999,  1000,  1001,  1005,  1006,
    1007,  1012,  1021,  1025,  1029,  1030,  1035,  1036,  1040,  1041,
    1045,  1046,  1050,  1051,  1055,  1059,  1060,  1064,  1065,  1066,
    1071,  1072,  1076,  1085,  1099,  1107,  1117,  1126,  1140,  1145,
    1159,  1160,  1164,  1165,  1169,  1174
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NULLTOKEN", "TRUETOKEN", "FALSETOKEN",
  "BREAK", "CASE", "DEFAULT", "FOR", "NEW", "VAR", "CONSTTOKEN",
  "CONTINUE", "FUNCTION", "RETURN", "VOIDTOKEN", "DELETETOKEN", "IF",
  "THISTOKEN", "DO", "WHILE", "INTOKEN", "INSTANCEOF", "TYPEOF", "SWITCH",
  "WITH", "RESERVED", "THROW", "TRY", "CATCH", "FINALLY", "DEBUGGER",
  "IF_WITHOUT_ELSE", "ELSE", "EQEQ", "NE", "STREQ", "STRNEQ", "LE", "GE",
  "OR", "AND", "PLUSPLUS", "MINUSMINUS", "LSHIFT", "RSHIFT", "URSHIFT",
  "PLUSEQUAL", "MINUSEQUAL", "MULTEQUAL", "DIVEQUAL", "LSHIFTEQUAL",
  "RSHIFTEQUAL", "URSHIFTEQUAL", "ANDEQUAL", "MODEQUAL", "XOREQUAL",
  "OREQUAL", "OPENBRACE", "CLOSEBRACE", "IDENT_IN", "NUMBER", "STRING",
  "AUTOPLUSPLUS", "AUTOMINUSMINUS", "'|'", "'/'", "'^'", "'$'", "'?'",
  "'*'", "'+'", "'{'", "'}'", "','", "'.'", "'\\\\'", "'('", "')'", "':'",
  "'='", "'!'", "'['", "']'", "'-'", "'~'", "'%'", "'<'", "'>'", "'&'",
  "';'", "$accept", "IDENT", "Disjunction", "RegExp", "Alternative",
  "Term", "Assertion", "Quantifier", "QuantifierPrefix", "RAtom",
  "PatternCharacter", "CharacterClass", "ClassRanges", "Literal",
  "Property", "PropertyList", "PrimaryExpr", "PrimaryExprNoBrace",
  "ArrayLiteral", "ElementList", "ElisionOpt", "Elision", "MemberExpr",
  "MemberExprNoBF", "NewExpr", "NewExprNoBF", "CallExpr", "CallExprNoBF",
  "Arguments", "ArgumentList", "LeftHandSideExpr", "LeftHandSideExprNoBF",
  "PostfixExpr", "PostfixExprNoBF", "UnaryExprCommon", "UnaryExpr",
  "UnaryExprNoBF", "MultiplicativeExpr", "MultiplicativeExprNoBF",
  "AdditiveExpr", "AdditiveExprNoBF", "ShiftExpr", "ShiftExprNoBF",
  "RelationalExpr", "RelationalExprNoIn", "RelationalExprNoBF",
  "EqualityExpr", "EqualityExprNoIn", "EqualityExprNoBF", "BitwiseANDExpr",
  "BitwiseANDExprNoIn", "BitwiseANDExprNoBF", "BitwiseXORExpr",
  "BitwiseXORExprNoIn", "BitwiseXORExprNoBF", "BitwiseORExpr",
  "BitwiseORExprNoIn", "BitwiseORExprNoBF", "LogicalANDExpr",
  "LogicalANDExprNoIn", "LogicalANDExprNoBF", "LogicalORExpr",
  "LogicalORExprNoIn", "LogicalORExprNoBF", "ConditionalExpr",
  "ConditionalExprNoIn", "ConditionalExprNoBF", "AssignmentExpr",
  "AssignmentExprNoIn", "AssignmentExprNoBF", "AssignmentOperator", "Expr",
  "ExprNoIn", "ExprNoBF", "Statement", "Block", "VariableStatement",
  "VariableDeclarationList", "VariableDeclarationListNoIn",
  "ConstStatement", "ConstDeclarationList", "ConstDeclaration",
  "Initializer", "InitializerNoIn", "EmptyStatement", "ExprStatement",
  "IfStatement", "IterationStatement", "ExprOpt", "ExprNoInOpt",
  "ContinueStatement", "BreakStatement", "ReturnStatement",
  "WithStatement", "SwitchStatement", "CaseBlock", "CaseClausesOpt",
  "CaseClauses", "CaseClause", "DefaultClause", "LabelledStatement",
  "ThrowStatement", "TryStatement", "DebuggerStatement",
  "FunctionDeclaration", "FunctionExpr", "FormalParameterList",
  "FunctionBody", "Program", "SourceElements", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   124,    47,    94,    36,
      63,    42,    43,   123,   125,    44,    46,    92,    40,    41,
      58,    61,    33,    91,    93,    45,   126,    37,    60,    62,
      38,    59
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    92,    93,    94,    94,    95,    95,    96,    96,    97,
      97,    97,    98,    98,    99,    99,   100,   100,   100,   100,
     100,   100,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   102,   103,   103,   104,   104,   104,   104,   105,   105,
     105,   105,   105,   106,   106,   106,   106,   106,   107,   107,
     108,   108,   108,   108,   109,   109,   109,   109,   109,   110,
     110,   110,   111,   111,   112,   112,   113,   113,   114,   114,
     114,   114,   114,   114,   115,   115,   115,   115,   116,   116,
     117,   117,   118,   118,   118,   118,   119,   119,   119,   119,
     120,   120,   121,   121,   122,   122,   123,   123,   124,   124,
     124,   125,   125,   125,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   127,   127,   128,   128,   129,
     129,   129,   129,   130,   130,   130,   130,   131,   131,   131,
     132,   132,   132,   133,   133,   133,   133,   134,   134,   134,
     134,   135,   135,   135,   135,   135,   135,   135,   136,   136,
     136,   136,   136,   136,   137,   137,   137,   137,   137,   137,
     137,   138,   138,   138,   138,   138,   139,   139,   139,   139,
     139,   140,   140,   140,   140,   140,   141,   141,   142,   142,
     143,   143,   144,   144,   145,   145,   146,   146,   147,   147,
     148,   148,   149,   149,   150,   150,   151,   151,   152,   152,
     153,   153,   154,   154,   155,   155,   156,   156,   157,   157,
     158,   158,   159,   159,   160,   160,   161,   161,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     163,   163,   164,   164,   165,   165,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   167,   167,   168,   168,   169,   169,   169,
     169,   170,   170,   170,   170,   171,   171,   172,   172,   173,
     173,   174,   175,   176,   177,   177,   178,   178,   179,   179,
     179,   179,   179,   179,   179,   179,   180,   180,   181,   181,
     182,   182,   182,   182,   183,   183,   183,   183,   184,   184,
     184,   184,   185,   186,   187,   187,   188,   188,   189,   189,
     190,   190,   191,   191,   192,   193,   193,   194,   194,   194,
     195,   195,   196,   196,   197,   197,   197,   197,   198,   198,
     199,   199,   200,   200,   201,   201
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     4,     0,     2,     1,
       1,     2,     1,     1,     1,     2,     1,     1,     1,     3,
       4,     5,     1,     1,     2,     2,     1,     3,     5,     5,
       5,     1,     3,     4,     1,     1,     3,     2,     1,     1,
       1,     1,     1,     3,     3,     3,     7,     8,     1,     3,
       1,     2,     3,     4,     1,     1,     1,     1,     3,     3,
       3,     5,     2,     4,     0,     1,     1,     2,     1,     1,
       1,     4,     3,     3,     1,     4,     3,     3,     1,     2,
       1,     2,     2,     2,     4,     3,     2,     2,     4,     3,
       2,     3,     1,     3,     1,     1,     1,     1,     1,     2,
       2,     1,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     1,     1,     1,     1,     1,
       3,     3,     3,     1,     3,     3,     3,     1,     3,     3,
       1,     3,     3,     1,     3,     3,     3,     1,     3,     3,
       3,     1,     3,     3,     3,     3,     3,     3,     1,     3,
       3,     3,     3,     3,     1,     3,     3,     3,     3,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     5,     1,     5,
       1,     5,     1,     3,     1,     3,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     3,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     3,     3,     1,     2,     3,
       4,     1,     2,     3,     4,     3,     3,     1,     3,     1,
       2,     2,     2,     1,     2,     2,     5,     7,     7,     7,
       5,     9,    10,     7,     8,     9,     0,     1,     0,     1,
       2,     2,     3,     3,     2,     2,     3,     3,     2,     2,
       3,     3,     5,     5,     3,     5,     0,     1,     1,     2,
       3,     4,     2,     3,     3,     3,     3,     4,     7,     9,
       2,     2,     7,     8,     6,     7,     7,     8,     1,     3,
       0,     1,     0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
     332,    38,    39,    40,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     2,    41,    42,
       0,     0,     0,     0,     0,    64,     0,     0,   273,    57,
      55,    74,    56,    80,    96,    97,   101,   117,   118,   123,
     130,   137,   154,   171,   180,   186,   192,   198,   204,   210,
     216,   234,     0,   334,   236,   237,   238,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     239,     0,   333,   295,   294,     0,   288,     0,     0,     0,
       7,    57,    69,    68,    50,    78,    81,    70,   257,     0,
     269,     0,   267,   291,   290,     0,     0,   299,   298,    78,
      94,    95,    98,   115,   116,   119,   127,   133,   141,   161,
     176,   182,   188,   194,   200,   206,   212,   230,     0,    98,
     105,   104,     0,     0,     0,   106,     0,     0,     0,     0,
     321,   320,   107,   109,   253,     0,   108,   110,   111,     0,
     114,    66,     0,     0,    65,   112,   113,     0,     0,     0,
       0,    86,     0,     0,    87,   102,   103,   219,   220,   221,
     222,   223,   224,   225,   226,   229,   227,   228,   218,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   275,     0,   274,     1,   335,   297,
     296,     0,    98,   148,   166,   178,   184,   190,   196,   202,
     208,   214,   232,   289,     0,    78,    79,     0,     0,    51,
       0,     0,     0,    48,     0,     0,     3,     0,     0,    77,
       0,   258,   256,     0,   255,   270,   266,     0,   265,   293,
     292,     0,    82,     0,     0,    83,    99,   100,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   301,     0,   300,     0,     0,     0,     0,
       0,   316,   315,     0,     0,   254,    58,    64,    60,    59,
      62,    67,   314,    76,    90,     0,    92,     0,    89,     0,
     217,   125,   124,   126,   131,   132,   138,   139,   140,   160,
     159,   157,   158,   155,   156,   172,   173,   174,   175,   181,
     187,   193,   199,   205,     0,   235,   261,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   286,    73,     0,   328,
       0,     0,     0,     0,     0,     0,    52,     0,     5,     7,
      12,    13,    23,     0,     7,     0,    31,     8,     9,    10,
      22,    26,    72,     0,   271,   259,   268,     0,     0,    85,
       0,   213,   121,   120,   122,   128,   129,   134,   135,   136,
     147,   146,   144,   145,   142,   143,   162,   163,   164,   165,
     177,   183,   189,   195,   201,     0,   231,     0,     0,     0,
       0,     0,     0,   317,     0,     0,    91,    75,    88,     0,
       0,     0,   262,     0,   286,     0,    98,   215,   153,   151,
     152,   149,   150,   167,   168,   169,   170,   179,   185,   191,
     197,   203,     0,   233,   287,     0,   330,     0,     0,     0,
       0,    45,    44,    43,     0,    53,    49,     6,     4,    25,
      24,     0,     0,    34,     0,    35,     0,    18,    16,    17,
       0,    11,    14,    71,   260,   330,     0,    84,     0,   276,
       0,   280,   306,   303,   302,     0,    61,    63,    93,   211,
       0,   272,     0,   263,     0,     0,     0,   286,     0,   331,
     329,   330,   330,     0,     0,     0,     7,     7,     7,    27,
       0,    32,     0,    37,     0,    15,     0,   330,   207,     0,
       0,     0,     0,   307,   308,     0,     0,     0,   264,   286,
     283,   209,     0,   324,     0,     0,   330,   330,     0,     0,
       0,     0,    33,    36,    19,     0,   322,     0,   277,   279,
     278,     0,     0,   304,   306,   309,   318,   284,     0,     0,
       0,   325,   326,     0,     0,   330,    28,    29,    30,     0,
      20,   323,   310,   312,     0,     0,   285,     0,   281,   327,
      46,     0,    21,   311,   313,   305,   319,   282,    47
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    91,   235,    92,   236,   377,   378,   481,   482,   379,
     380,   381,   523,    40,   233,   234,    93,    94,    42,   152,
     153,   154,   109,    43,   110,    44,   111,    45,   161,   305,
     129,    46,   113,    47,   114,   115,    49,   116,    50,   117,
      51,   118,    52,   119,   214,    53,   120,   215,    54,   121,
     216,    55,   122,   217,    56,   123,   218,    57,   124,   219,
      58,   125,   220,    59,   126,   221,    60,   127,   222,    61,
     339,   454,   223,    62,    63,    64,    65,    99,   337,    66,
     101,   102,   241,   432,    67,    68,    69,    70,   455,   224,
      71,    72,    73,    74,    75,   493,   532,   533,   534,   564,
      76,    77,    78,    79,    80,    97,   360,   508,    81,   509
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -423
static const yytype_int16 yypact[] =
{
    1073,  -423,  -423,  -423,     5,   -33,   367,    -6,    -6,    23,
      -6,   260,  1303,  1303,    16,  -423,  1073,    20,  1303,    85,
     116,  1303,   137,    18,  1303,  1303,   842,  -423,  -423,  -423,
    1303,  1303,  1303,  1303,  1303,   138,  1303,  1303,  -423,   175,
    -423,  -423,  -423,   218,  -423,   219,   440,  -423,  -423,  -423,
      10,   114,   312,   150,   368,   151,   190,   209,   230,   -21,
    -423,  -423,    12,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,   282,  1073,  -423,  -423,    35,   719,   367,   127,   347,
    -423,  -423,  -423,  -423,  -423,   235,  -423,  -423,   211,    24,
     211,    31,  -423,  -423,  -423,    36,   239,  -423,  -423,   235,
    -423,   250,   886,  -423,  -423,  -423,    94,   121,   420,   178,
     376,   249,   280,   263,   340,    13,  -423,  -423,    43,   271,
    -423,  -423,  1303,   370,  1303,  -423,  1303,  1303,    46,    -8,
    -423,  -423,  -423,  -423,  -423,   984,  -423,  -423,  -423,   177,
    -423,  -423,   132,  1104,   290,  -423,  -423,  1073,    -6,  1188,
    1303,  -423,    -6,  1303,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  -423,  1303,
    1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,
    1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,
    1303,  1303,  1303,  1303,  -423,  1303,  -423,  -423,  -423,  -423,
    -423,    -6,   344,   420,    19,   402,   325,   348,   352,   378,
      78,  -423,  -423,   356,   332,   235,  -423,   -18,   349,  -423,
     355,   366,    25,  -423,   135,   365,   286,    -6,  1303,  -423,
    1303,  -423,  -423,    -6,  -423,  -423,  -423,    -6,  -423,  -423,
    -423,    95,  -423,    -6,  1303,  -423,  -423,  -423,  1303,  1303,
    1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,
    1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,
    1303,  1303,  1303,  -423,  1303,  -423,   194,   384,   206,   214,
     241,  -423,  -423,   385,   137,  -423,  -423,   138,  -423,  -423,
    -423,  -423,  -423,  -423,  -423,   255,  -423,   165,  -423,   173,
    -423,  -423,  -423,  -423,    94,    94,   121,   121,   121,   420,
     420,   420,   420,   420,   420,   178,   178,   178,   178,   376,
     249,   280,   263,   340,   388,  -423,     9,   117,  1303,  1303,
    1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,  1303,
    1303,  1303,  1303,  1303,  1303,  1303,  1303,  -423,   389,  -423,
     256,    99,  1303,  1303,  1303,   391,  -423,   381,    -6,  -423,
    -423,  -423,  -423,   136,   400,   244,  -423,  -423,  -423,   382,
    -423,  -423,  -423,   184,  -423,   211,  -423,   412,   261,  -423,
     196,  -423,  -423,  -423,  -423,    94,    94,   121,   121,   121,
     420,   420,   420,   420,   420,   420,   178,   178,   178,   178,
     376,   249,   280,   263,   340,   392,  -423,  1073,  1303,  1073,
     414,  1073,    -6,  -423,  1219,  1303,  -423,  -423,  -423,  1303,
    1303,  1303,   453,    -6,  1303,   262,   886,  -423,   420,   420,
     420,   420,   420,    19,    19,    19,    19,   402,   325,   348,
     352,   378,   397,  -423,   403,   390,  1073,    -6,   421,   423,
     274,  -423,  -423,  -423,   119,  -423,  -423,  -423,  -423,  -423,
    -423,   153,   408,  -423,   323,  -423,    92,  -423,  -423,  -423,
     417,  -423,   429,  -423,  -423,  1073,   441,  -423,  1303,   467,
     281,  -423,   495,  -423,  -423,   424,  -423,  -423,  -423,  -423,
     299,  -423,  1303,   425,   413,  1073,  1303,  1303,   447,  1073,
    -423,  1073,  1073,   451,   454,   301,  -423,  -423,  -423,  -423,
     161,  -423,   323,   106,   315,  -423,   457,  1073,  -423,  1073,
      37,  1303,    70,   495,  -423,   137,  1073,   304,  -423,  1303,
    -423,  -423,   446,  -423,   459,   466,  1073,  1073,   469,   452,
     455,   461,  -423,   106,  -423,   188,  -423,   470,  -423,  -423,
    -423,    75,   458,  -423,   495,  -423,   501,  -423,  1073,   464,
    1073,  -423,  -423,   473,   484,  1073,  -423,  -423,  -423,   474,
    -423,  -423,  1073,  1073,   489,   137,  -423,  1073,  -423,  -423,
    -423,   491,  -423,  1073,  1073,  -423,  -423,  -423,  -423
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -423,     0,  -334,  -423,  -423,  -423,  -423,  -423,  -423,  -423,
    -423,  -423,  -370,  -423,   189,  -423,  -423,    30,  -423,  -423,
     264,  -423,    42,  -423,    44,  -423,  -423,  -423,   -16,  -423,
     534,  -423,  -423,  -423,   133,    39,  -423,   -92,  -423,   -62,
    -423,   485,  -423,    34,   113,  -423,  -145,   213,  -423,  -139,
     208,  -423,  -132,   212,  -423,  -129,   217,  -423,  -140,   207,
    -423,  -423,  -423,  -423,  -423,  -423,  -423,  -138,  -321,  -423,
     -32,     6,  -423,  -423,    97,   -11,  -423,  -423,  -423,  -423,
    -423,   317,   -97,    62,  -423,  -423,  -423,  -423,  -422,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,     8,  -423,    40,  -423,
    -423,  -423,  -423,  -423,  -423,  -423,  -250,  -411,  -423,     2
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      39,   388,    82,   245,    85,   476,    83,    98,   100,   105,
     106,   139,   504,   204,   179,   300,    39,   128,   437,   140,
     202,   306,   293,   294,   103,   242,    39,   138,   145,   164,
      41,   430,   246,   452,   453,   468,   209,   249,   559,   149,
     472,   310,   340,    27,   283,    86,    41,   291,    95,   203,
      96,   130,   131,   329,   281,    27,    41,   135,   341,   342,
     330,   358,   333,   142,   143,   334,    27,   335,   331,   146,
     147,   148,   332,   150,   526,   155,   156,   180,   562,   239,
     258,   181,    39,   282,    27,   542,    27,   205,   228,   232,
     431,   314,   315,   252,   132,   255,    84,   182,   134,   243,
     544,   545,   384,   206,   520,   364,   247,   343,   344,   141,
     501,   460,    41,   133,   104,   244,   557,   569,   284,   353,
     391,   284,   248,   316,   317,   318,   210,   250,   560,   225,
     563,   226,   410,    48,   285,   573,   574,   292,   286,   411,
     288,   414,   289,   290,   415,    39,   416,   412,   354,    48,
     284,   413,   553,    27,   473,   582,    27,    39,   303,    48,
      27,   259,   308,   136,   591,   260,   307,    27,   473,   309,
     395,   396,   188,   189,   387,    41,   521,   522,   459,   208,
      27,   261,   549,   550,   551,   541,   183,    41,    27,   190,
     191,   522,   433,   262,   137,   366,    26,    27,   514,   184,
     267,   268,   397,   398,   399,   227,   263,   297,   434,   357,
     367,   336,   469,   151,   515,    48,   298,   269,   270,   311,
     312,   313,    27,   473,   461,   462,   463,   359,   325,   326,
     327,   328,   365,   516,   517,   518,   376,   382,   192,   193,
     284,   198,   208,   385,   383,   552,   522,   100,   284,   427,
     579,   359,   284,   389,   302,   157,   296,   428,   199,   284,
     390,   107,   580,     1,     2,     3,   271,   272,   483,   284,
      87,   284,   201,   417,    88,   200,    12,    13,    48,    15,
     487,   284,   207,   423,    18,   419,   497,   498,   484,   284,
      48,   499,   240,   420,   158,   162,   159,   159,   392,   393,
     394,   160,   163,    24,    25,    27,   473,   406,   407,   408,
     409,   237,   474,   159,   256,   257,   284,   251,   238,    89,
     421,    27,    28,    29,    30,    31,   253,    90,   159,   279,
     425,   457,    32,   254,   426,   458,   457,   284,    33,   277,
     486,   505,    34,    35,   435,    36,    37,    27,   278,   457,
     528,   108,   369,   513,   370,   371,   284,   185,   186,   187,
     530,   359,   372,   373,   374,   301,   338,   232,   467,   375,
       1,     2,     3,   470,   284,   475,   457,    87,   536,   284,
     548,    88,   280,   568,    27,   473,    15,   256,   257,   554,
     555,   287,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   194,   195,   196,   197,   229,    27,   230,
     231,   273,   274,   275,   276,   349,   350,    39,   351,    39,
     352,    39,   495,   356,   490,   178,    89,   361,    27,    28,
      29,   355,   368,   503,    90,   362,   500,   345,   346,   347,
     348,   465,    27,   230,   231,    33,   363,    41,   456,    41,
      35,    41,   477,   478,   479,   480,    39,   510,   443,   444,
     445,   446,   418,   422,   359,   264,   265,   266,   429,   464,
     471,   485,   488,   492,   475,   502,   475,   506,   284,   524,
     511,   507,   512,   165,   166,    39,    41,   519,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   525,
     527,   529,   531,   535,   539,    39,   431,   543,   537,    39,
     546,    39,    39,   547,   489,    41,   491,   556,   494,   571,
     475,   178,   475,   475,   566,   570,   572,    39,   575,    39,
     581,   576,   585,   589,   577,    41,    39,   561,   583,    41,
     578,    41,    41,   587,   590,   112,    39,    39,   592,   595,
      48,   598,    48,   475,    48,   112,   466,    41,   448,    41,
     451,   424,   447,   449,   386,   538,    41,   112,    39,   450,
      39,   213,   584,   565,   596,    39,    41,    41,     0,     0,
       0,     0,    39,    39,   593,   594,     0,    39,     0,    48,
       0,     0,     0,    39,    39,     0,     0,     0,    41,     0,
      41,     0,   540,     0,     0,    41,   208,     0,     0,     0,
       0,     0,    41,    41,     0,     0,     0,    41,    48,     0,
     212,     0,     0,    41,    41,     0,   558,     0,     0,     0,
       0,     0,     0,   567,     0,     0,     0,     0,    48,     0,
       0,     0,    48,     0,    48,    48,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      48,     0,    48,     0,     0,   586,   112,   588,   112,    48,
     112,   112,     0,   319,   320,   321,   322,   323,   324,    48,
      48,     0,     0,     0,   597,     0,     0,   112,     0,     0,
     208,   208,     0,   112,   112,     0,     0,   112,     0,     0,
       0,    48,     0,    48,     0,     0,     0,     0,    48,     0,
       0,     0,     0,   112,     0,    48,    48,     0,     0,     0,
      48,     0,     1,     2,     3,     0,    48,    48,     0,    87,
     211,     0,     0,    88,     0,    12,    13,   112,    15,   112,
       0,     0,     0,    18,     0,     0,     0,     0,     0,     0,
       0,     0,   400,   401,   402,   403,   404,   405,     0,     0,
       0,     0,    24,    25,     0,     0,     0,     0,     0,     0,
       0,     0,   112,     0,   112,     0,     0,     0,    89,     0,
      27,    28,    29,    30,    31,     0,    90,     0,   112,     0,
       0,    32,   112,     0,     0,     0,     0,    33,     0,     0,
       0,    34,    35,     0,    36,    37,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   112,     0,   112,     0,
       0,     0,     0,     0,   213,   438,   439,   440,   441,   442,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,     0,     0,     0,     0,     1,     2,     3,     4,     0,
       0,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,     0,     0,    18,    19,    20,     0,
      21,    22,   112,   436,    23,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    24,    25,     0,   436,   436,
     112,     0,     0,     0,     0,     0,   112,   112,   112,     0,
       0,    26,   144,    27,    28,    29,    30,    31,     0,     0,
       0,     0,     0,     0,    32,     0,   213,     0,     0,     0,
      33,     0,     0,     0,    34,    35,     0,    36,    37,   256,
     257,     0,     0,    38,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,     0,     0,     0,     0,     0,
       0,     0,   112,     0,     0,     0,     0,     0,   112,   112,
       0,     0,     0,   112,   112,   436,     0,   178,   112,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     2,     3,
       4,   213,     0,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,     0,     0,    18,    19,
      20,     0,    21,    22,     0,     0,    23,     0,     0,     0,
       0,     0,   112,     0,     0,     0,     0,    24,    25,     0,
       0,     0,     0,     0,     0,     0,   112,     0,     0,     0,
     436,   112,     0,    26,   295,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     0,     0,    32,     0,     0,     0,
       0,     0,    33,     0,     0,   112,    34,    35,     0,    36,
      37,     0,     0,   112,     0,    38,     1,     2,     3,     4,
       0,     0,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,     0,     0,    18,    19,    20,
       0,    21,    22,     0,     0,    23,     0,     1,     2,     3,
       0,     0,     0,     0,    87,     0,    24,    25,    88,     0,
      12,    13,     0,    15,     0,     0,     0,     0,    18,     0,
       0,     0,    26,     0,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     0,     0,    32,     0,    24,    25,     0,
       0,    33,     0,     0,     0,    34,    35,     0,    36,    37,
       0,     0,     0,    89,    38,    27,    28,    29,    30,    31,
       0,    90,     0,     0,     0,     0,    32,     0,     0,     0,
       0,     0,    33,     0,     0,     0,    34,    35,   299,    36,
      37,     1,     2,     3,     0,     0,     0,     0,    87,     0,
       0,     0,    88,     0,    12,    13,     0,    15,     0,     0,
       0,     0,    18,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     1,     2,     3,     0,     0,     0,     0,    87,
       0,    24,    25,    88,     0,    12,    13,     0,    15,     0,
       0,     0,     0,    18,     0,     0,     0,    89,     0,    27,
      28,    29,    30,    31,     0,    90,     0,     0,     0,     0,
      32,     0,    24,    25,     0,     0,    33,   304,     0,     0,
      34,    35,     0,    36,    37,     0,     0,     0,    89,     0,
      27,    28,    29,    30,    31,     0,    90,     0,     0,     0,
       0,    32,     0,     0,     0,     0,     0,    33,     0,     0,
       0,    34,    35,   496,    36,    37,     1,     2,     3,     0,
       0,     0,     0,    87,     0,     0,     0,    88,     0,    12,
      13,     0,    15,     0,     0,     0,     0,    18,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    24,    25,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    89,     0,    27,    28,    29,    30,    31,     0,
      90,     0,     0,     0,     0,    32,     0,     0,     0,     0,
       0,    33,     0,     0,     0,    34,    35,     0,    36,    37
};

static const yytype_int16 yycheck[] =
{
       0,   251,     0,   100,     4,   375,     1,     7,     8,     9,
      10,    22,   434,     1,    46,   153,    16,    11,   339,     1,
      41,   159,    30,    31,     1,     1,    26,    21,    26,    45,
       0,    22,     1,   354,   355,   369,     1,     1,     1,    33,
     374,   179,    23,    61,     1,    78,    16,     1,     6,    70,
       6,    12,    13,   198,    41,    61,    26,    18,    39,    40,
     199,    79,   202,    24,    25,   203,    61,   205,   200,    30,
      31,    32,   201,    34,   485,    36,    37,    67,     8,    95,
     112,    71,    82,    70,    61,   507,    61,    75,    88,    89,
      81,   183,   184,   109,    78,   111,    91,    87,    78,    75,
     511,   512,   240,    91,   474,    80,    75,    88,    89,    91,
     431,   361,    82,    16,    91,    91,   527,   539,    75,    41,
     258,    75,    91,   185,   186,   187,    91,    91,    91,    87,
      60,    87,   277,     0,    91,   546,   547,    91,   132,   278,
     134,   281,   136,   137,   282,   145,   284,   279,    70,    16,
      75,   280,   522,    61,    62,    80,    61,   157,   158,    26,
      61,    67,   162,    78,   575,    71,   160,    61,    62,   163,
     262,   263,    22,    23,    79,   145,    84,    85,    79,    82,
      61,    87,   516,   517,   518,   506,    72,   157,    61,    39,
      40,    85,    75,    72,    78,    60,    59,    61,    79,    85,
      22,    23,   264,   265,   266,    78,    85,    75,    91,   225,
      75,   211,    76,    75,   464,    82,    84,    39,    40,   180,
     181,   182,    61,    62,   362,   363,   364,   227,   194,   195,
     196,   197,   232,    80,    81,    82,   236,   237,    88,    89,
      75,    90,   145,   243,   238,    84,    85,   247,    75,    84,
      62,   251,    75,   253,   157,    80,    79,    84,    68,    75,
     254,     1,    74,     3,     4,     5,    88,    89,    84,    75,
      10,    75,    42,    79,    14,    66,    16,    17,   145,    19,
      84,    75,     0,   294,    24,    79,   424,   425,   385,    75,
     157,   429,    81,    79,    76,    76,    78,    78,   259,   260,
     261,    83,    83,    43,    44,    61,    62,   273,   274,   275,
     276,    76,    68,    78,    43,    44,    75,    78,    83,    59,
      79,    61,    62,    63,    64,    65,    76,    67,    78,    66,
      75,    75,    72,    83,    79,    79,    75,    75,    78,    90,
      79,    79,    82,    83,   338,    85,    86,    61,    68,    75,
     488,    91,    66,    79,    68,    69,    75,    45,    46,    47,
      79,   361,    76,    77,    78,    75,    22,   367,   368,    83,
       3,     4,     5,   373,    75,   375,    75,    10,    79,    75,
      79,    14,    42,    79,    61,    62,    19,    43,    44,    74,
      75,    21,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    35,    36,    37,    38,    60,    61,    62,
      63,    35,    36,    37,    38,    90,    68,   417,    66,   419,
      42,   421,   422,    91,   418,    81,    59,    78,    61,    62,
      63,    75,    67,   433,    67,    80,   430,    35,    36,    37,
      38,    60,    61,    62,    63,    78,    80,   417,    59,   419,
      83,   421,    70,    71,    72,    73,   456,   457,   345,   346,
     347,   348,    78,    78,   464,    45,    46,    47,    80,    78,
      70,    59,    80,    59,   474,    22,   476,    80,    75,    62,
      59,    91,    59,    43,    44,   485,   456,    79,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    70,
      59,    34,     7,    79,    91,   505,    81,    60,   502,   509,
      59,   511,   512,    59,   417,   485,   419,    60,   421,    60,
     520,    81,   522,   523,   535,    79,    60,   527,    59,   529,
      60,    79,    31,    60,    79,   505,   536,   531,    80,   509,
      79,   511,   512,    79,    60,    11,   546,   547,    74,    60,
     417,    60,   419,   553,   421,    21,   367,   527,   350,   529,
     353,   297,   349,   351,   247,   503,   536,    33,   568,   352,
     570,    86,   564,   533,   585,   575,   546,   547,    -1,    -1,
      -1,    -1,   582,   583,   582,   583,    -1,   587,    -1,   456,
      -1,    -1,    -1,   593,   594,    -1,    -1,    -1,   568,    -1,
     570,    -1,   505,    -1,    -1,   575,   509,    -1,    -1,    -1,
      -1,    -1,   582,   583,    -1,    -1,    -1,   587,   485,    -1,
      86,    -1,    -1,   593,   594,    -1,   529,    -1,    -1,    -1,
      -1,    -1,    -1,   536,    -1,    -1,    -1,    -1,   505,    -1,
      -1,    -1,   509,    -1,   511,   512,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     527,    -1,   529,    -1,    -1,   568,   132,   570,   134,   536,
     136,   137,    -1,   188,   189,   190,   191,   192,   193,   546,
     547,    -1,    -1,    -1,   587,    -1,    -1,   153,    -1,    -1,
     593,   594,    -1,   159,   160,    -1,    -1,   163,    -1,    -1,
      -1,   568,    -1,   570,    -1,    -1,    -1,    -1,   575,    -1,
      -1,    -1,    -1,   179,    -1,   582,   583,    -1,    -1,    -1,
     587,    -1,     3,     4,     5,    -1,   593,   594,    -1,    10,
      11,    -1,    -1,    14,    -1,    16,    17,   203,    19,   205,
      -1,    -1,    -1,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   267,   268,   269,   270,   271,   272,    -1,    -1,
      -1,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   238,    -1,   240,    -1,    -1,    -1,    59,    -1,
      61,    62,    63,    64,    65,    -1,    67,    -1,   254,    -1,
      -1,    72,   258,    -1,    -1,    -1,    -1,    78,    -1,    -1,
      -1,    82,    83,    -1,    85,    86,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   282,    -1,   284,    -1,
      -1,    -1,    -1,    -1,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,    -1,    -1,    -1,    -1,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    24,    25,    26,    -1,
      28,    29,   338,   339,    32,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    43,    44,    -1,   354,   355,
     356,    -1,    -1,    -1,    -1,    -1,   362,   363,   364,    -1,
      -1,    59,    60,    61,    62,    63,    64,    65,    -1,    -1,
      -1,    -1,    -1,    -1,    72,    -1,   431,    -1,    -1,    -1,
      78,    -1,    -1,    -1,    82,    83,    -1,    85,    86,    43,
      44,    -1,    -1,    91,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   418,    -1,    -1,    -1,    -1,    -1,   424,   425,
      -1,    -1,    -1,   429,   430,   431,    -1,    81,   434,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,   506,    -1,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    24,    25,
      26,    -1,    28,    29,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    -1,   488,    -1,    -1,    -1,    -1,    43,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   502,    -1,    -1,    -1,
     506,   507,    -1,    59,    60,    61,    62,    63,    64,    65,
      -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    -1,    -1,
      -1,    -1,    78,    -1,    -1,   531,    82,    83,    -1,    85,
      86,    -1,    -1,   539,    -1,    91,     3,     4,     5,     6,
      -1,    -1,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    24,    25,    26,
      -1,    28,    29,    -1,    -1,    32,    -1,     3,     4,     5,
      -1,    -1,    -1,    -1,    10,    -1,    43,    44,    14,    -1,
      16,    17,    -1,    19,    -1,    -1,    -1,    -1,    24,    -1,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    -1,
      -1,    -1,    -1,    -1,    -1,    72,    -1,    43,    44,    -1,
      -1,    78,    -1,    -1,    -1,    82,    83,    -1,    85,    86,
      -1,    -1,    -1,    59,    91,    61,    62,    63,    64,    65,
      -1,    67,    -1,    -1,    -1,    -1,    72,    -1,    -1,    -1,
      -1,    -1,    78,    -1,    -1,    -1,    82,    83,    84,    85,
      86,     3,     4,     5,    -1,    -1,    -1,    -1,    10,    -1,
      -1,    -1,    14,    -1,    16,    17,    -1,    19,    -1,    -1,
      -1,    -1,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,    -1,    -1,    -1,    -1,    10,
      -1,    43,    44,    14,    -1,    16,    17,    -1,    19,    -1,
      -1,    -1,    -1,    24,    -1,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    -1,    67,    -1,    -1,    -1,    -1,
      72,    -1,    43,    44,    -1,    -1,    78,    79,    -1,    -1,
      82,    83,    -1,    85,    86,    -1,    -1,    -1,    59,    -1,
      61,    62,    63,    64,    65,    -1,    67,    -1,    -1,    -1,
      -1,    72,    -1,    -1,    -1,    -1,    -1,    78,    -1,    -1,
      -1,    82,    83,    84,    85,    86,     3,     4,     5,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    -1,    14,    -1,    16,
      17,    -1,    19,    -1,    -1,    -1,    -1,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    -1,
      67,    -1,    -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,
      -1,    78,    -1,    -1,    -1,    82,    83,    -1,    85,    86
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    24,    25,
      26,    28,    29,    32,    43,    44,    59,    61,    62,    63,
      64,    65,    72,    78,    82,    83,    85,    86,    91,    93,
     105,   109,   110,   115,   117,   119,   123,   125,   126,   128,
     130,   132,   134,   137,   140,   143,   146,   149,   152,   155,
     158,   161,   165,   166,   167,   168,   171,   176,   177,   178,
     179,   182,   183,   184,   185,   186,   192,   193,   194,   195,
     196,   200,   201,     1,    91,    93,    78,    10,    14,    59,
      67,    93,    95,   108,   109,   114,   116,   197,    93,   169,
      93,   172,   173,     1,    91,    93,    93,     1,    91,   114,
     116,   118,   122,   124,   126,   127,   129,   131,   133,   135,
     138,   141,   144,   147,   150,   153,   156,   159,   163,   122,
     127,   127,    78,   166,    78,   127,    78,    78,   163,   167,
       1,    91,   127,   127,    60,   201,   127,   127,   127,   163,
     127,    75,   111,   112,   113,   127,   127,    80,    76,    78,
      83,   120,    76,    83,   120,    43,    44,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    81,   162,
      67,    71,    87,    72,    85,    45,    46,    47,    22,    23,
      39,    40,    88,    89,    35,    36,    37,    38,    90,    68,
      66,    42,    41,    70,     1,    75,    91,     0,   166,     1,
      91,    11,   122,   133,   136,   139,   142,   145,   148,   151,
     154,   157,   160,   164,   181,   114,   116,    78,    93,    60,
      62,    63,    93,   106,   107,    94,    96,    76,    83,   120,
      81,   174,     1,    75,    91,   174,     1,    75,    91,     1,
      91,    78,   120,    76,    83,   120,    43,    44,   162,    67,
      71,    87,    72,    85,    45,    46,    47,    22,    23,    39,
      40,    88,    89,    35,    36,    37,    38,    90,    68,    66,
      42,    41,    70,     1,    75,    91,   163,    21,   163,   163,
     163,     1,    91,    30,    31,    60,    79,    75,    84,    84,
     159,    75,   166,    93,    79,   121,   159,   163,    93,   163,
     159,   127,   127,   127,   129,   129,   131,   131,   131,   133,
     133,   133,   133,   133,   133,   135,   135,   135,   135,   138,
     141,   144,   147,   150,   159,   159,    93,   170,    22,   162,
      23,    39,    40,    88,    89,    35,    36,    37,    38,    90,
      68,    66,    42,    41,    70,    75,    91,   120,    79,    93,
     198,    78,    80,    80,    80,    93,    60,    75,    67,    66,
      68,    69,    76,    77,    78,    83,    93,    97,    98,   101,
     102,   103,    93,   163,   159,    93,   173,    79,   198,    93,
     163,   159,   127,   127,   127,   129,   129,   131,   131,   131,
     133,   133,   133,   133,   133,   133,   135,   135,   135,   135,
     138,   141,   144,   147,   150,   159,   159,    79,    78,    79,
      79,    79,    78,   167,   112,    75,    79,    84,    84,    80,
      22,    81,   175,    75,    91,   163,   122,   160,   133,   133,
     133,   133,   133,   136,   136,   136,   136,   139,   142,   145,
     148,   151,   160,   160,   163,   180,    59,    75,    79,    79,
     198,   159,   159,   159,    78,    60,   106,    93,    94,    76,
      93,    70,    94,    62,    68,    93,   104,    70,    71,    72,
      73,    99,   100,    84,   174,    59,    79,    84,    80,   166,
     163,   166,    59,   187,   166,    93,    84,   159,   159,   159,
     163,   160,    22,    93,   180,    79,    80,    91,   199,   201,
      93,    59,    59,    79,    79,   198,    80,    81,    82,    79,
     104,    84,    85,   104,    62,    70,   199,    59,   159,    34,
      79,     7,   188,   189,   190,    79,    79,   163,   175,    91,
     166,   160,   180,    60,   199,   199,    59,    59,    79,    94,
      94,    94,    84,   104,    74,    75,    60,   199,   166,     1,
      91,   163,     8,    60,   191,   190,   167,   166,    79,   180,
      79,    60,    60,   199,   199,    59,    79,    79,    79,    62,
      74,    60,    80,    80,   188,    31,   166,    79,   166,    60,
      60,   199,    74,   201,   201,    60,   167,   166,    60
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}






struct yypstate
  {
        /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

    /* Used to determine if this is the first time this instance has
       been used.  */
    int yynew;
  };

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
{
  return yypull_parse (0);
}

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yypull_parse (yypstate *yyps)
#else
int
yypull_parse (yyps)
    yypstate *yyps;
#endif
{
  int yystatus;
  yypstate *yyps_local;
  int yychar;
  YYSTYPE yylval;
  if (yyps == 0)
    {
      yyps_local = yypstate_new ();
      if (!yyps_local)
        {
          yyerror (YY_("memory exhausted"));
          return 2;
        }
    }
  else
    yyps_local = yyps;
  do {
    yychar = YYLEX;
    yystatus =
      yypush_parse (yyps_local, yychar, &yylval);
  } while (yystatus == YYPUSH_MORE);
  if (yyps == 0)
    yypstate_delete (yyps_local);
  return yystatus;
}

/* Initialize the parser data structure.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
yypstate *
yypstate_new (void)
#else
yypstate *
yypstate_new ()

#endif
{
  yypstate *yyps;
  yyps = (yypstate *) malloc (sizeof *yyps);
  if (!yyps)
    return 0;
  yyps->yynew = 1;
  return yyps;
}

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void
yypstate_delete (yypstate *yyps)
#else
void
yypstate_delete (yyps)
    yypstate *yyps;
#endif
{
#ifndef yyoverflow
  /* If the stack was reallocated but the parse did not complete, then the
     stack still needs to be freed.  */
  if (!yyps->yynew && yyps->yyss != yyps->yyssa)
    YYSTACK_FREE (yyps->yyss);
#endif
  free (yyps);
}

#define yynerrs yyps->yynerrs
#define yystate yyps->yystate
#define yyerrstatus yyps->yyerrstatus
#define yyssa yyps->yyssa
#define yyss yyps->yyss
#define yyssp yyps->yyssp
#define yyvsa yyps->yyvsa
#define yyvs yyps->yyvs
#define yyvsp yyps->yyvsp
#define yystacksize yyps->yystacksize

/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yypush_parse (yypstate *yyps, int yypushed_char, YYSTYPE const *yypushed_val)
#else
int
yypush_parse (yyps, yypushed_char, yypushed_val)
    yypstate *yyps;
    int yypushed_char;
    YYSTYPE const *yypushed_val;
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;


  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  if (!yyps->yynew)
    {
      yyn = yypact[yystate];
      goto yyread_pushed_token;
    }

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      if (!yyps->yynew)
        {
          YYDPRINTF ((stderr, "Return for a new token:\n"));
          yyresult = YYPUSH_MORE;
          goto yypushreturn;
        }
      yyps->yynew = 0;
yyread_pushed_token:
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yypushed_char;
      if (yypushed_val)
        yylval = *yypushed_val;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 215 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = node_new (TOK_NAME, PN_NAME); (yyval.node)->pn_u.name.name = (yyvsp[(1) - (1)].name).iname; (yyval.node)->pn_pos = (yyvsp[(1) - (1)].name).pos; (yyval.node)->pn_u.name.isconst = 0;;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 219 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = NULL;;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 220 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = NULL;;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 224 "./parser/Grammar.y"
    { PRINT_LINE_TODO; (yyval.node) = NULL;;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 225 "./parser/Grammar.y"
    { PRINT_LINE_TODO; (yyval.node) = NULL;;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 229 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 230 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 234 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 235 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 236 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 240 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 241 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 251 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 252 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 253 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 254 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 255 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 256 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 260 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 261 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 262 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;/* TODO: FIX AtomEscape = IDENT*/ ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 263 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;/* TODO: FIX AtomEscape = IDENT*/ ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 264 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 265 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 266 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 267 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 268 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 272 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 276 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 277 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 281 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 282 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 283 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 284 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 288 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = node_new (TOK_PRIMARY, PN_NULLARY); (yyval.node)->pn_op = JSOP_NULL;;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 289 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = node_new (TOK_PRIMARY, PN_NULLARY); (yyval.node)->pn_op = JSOP_TRUE;;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 290 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = node_new (TOK_PRIMARY, PN_NULLARY); (yyval.node)->pn_op = JSOP_FALSE;;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 291 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = node_new (TOK_NUMBER, PN_NULLARY);;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 292 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = node_new (TOK_STRING, PN_NULLARY);;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 296 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_COLON, PN_BINARY);
							(yyval.node)->pn_u.binary.left = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.binary.right = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 303 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_COLON, PN_BINARY);
							(yyval.node)->pn_u.binary.left = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.binary.right = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 309 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_COLON, PN_BINARY);
							(yyval.node)->pn_u.binary.left = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.binary.right = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));							
;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 315 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 317 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 321 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_RC, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (1)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (1)].node));
;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 326 "./parser/Grammar.y"
    { PRINT_LINE; 
							(yyval.node) = (yyvsp[(1) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
							g_assert ((yyvsp[(1) - (3)].node)->pn_arity == PN_LIST);
							if ((yyvsp[(1) - (3)].node)->pn_u.list.head) {
								JSNode *i;
								for (i = (yyvsp[(1) - (3)].node)->pn_u.list.head; i->pn_next != NULL; i = i->pn_next)
									;
								i->pn_next = (yyvsp[(3) - (3)].node);
							} else (yyvsp[(1) - (3)].node)->pn_u.list.head = (yyvsp[(3) - (3)].node);
;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 341 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 342 "./parser/Grammar.y"
    { PRINT_LINE;
									(yyval.node) = (yyvsp[(2) - (3)].node);
									node_correct_position_end ((yyval.node), (yyvsp[(3) - (3)].intValue));
;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 347 "./parser/Grammar.y"
    { PRINT_LINE;
									(yyval.node) = (yyvsp[(2) - (4)].node);
									node_correct_position_end ((yyval.node), (yyvsp[(4) - (4)].intValue));
;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 354 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = node_new (TOK_PRIMARY, PN_NULLARY); (yyval.node)->pn_op = JSOP_THIS;;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 357 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (1)].node);;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 358 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (3)].node);;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 362 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 363 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 364 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 368 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 370 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 374 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 379 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 380 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 386 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (1)].node);;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 387 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 388 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_DOT, PN_NAME);
							(yyval.node)->pn_u.name.expr = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.name.name = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 395 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_NEW, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(2) - (3)].node);
							(yyvsp[(2) - (3)].node)->pn_next = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(2) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 406 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 407 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_DOT, PN_NAME);
							(yyval.node)->pn_u.name.expr = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.name.name = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 414 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 419 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 424 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 428 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_LP, PN_LIST);
							(yyvsp[(1) - (2)].node)->pn_next = (yyvsp[(2) - (2)].node);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (2)].node));
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 435 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_LP, PN_LIST);
							(yyvsp[(1) - (2)].node)->pn_next = (yyvsp[(2) - (2)].node);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (2)].node));
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 442 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 443 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_DOT, PN_NAME);
							(yyval.node)->pn_u.name.expr = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.name.name = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 453 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_LP, PN_LIST);
							(yyvsp[(1) - (2)].node)->pn_next = (yyvsp[(2) - (2)].node);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (2)].node));
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 460 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_LP, PN_LIST);
							(yyvsp[(1) - (2)].node)->pn_next = (yyvsp[(2) - (2)].node);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (2)].node));
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 467 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 468 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new ( TOK_DOT, PN_NAME);
							(yyval.node)->pn_u.name.expr = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.name.name = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 478 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 479 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (3)].node);;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 483 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (1)].node);;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 484 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = (yyvsp[(1) - (3)].node);
							if ((yyvsp[(1) - (3)].node)) (yyvsp[(1) - (3)].node)->pn_next = (yyvsp[(3) - (3)].node);
							else (yyval.node) = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 504 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (2)].node);;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 505 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (2)].node);;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 510 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 511 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 515 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 516 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 517 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 518 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 519 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 520 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 521 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 522 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 523 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 524 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 525 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 539 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 540 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 541 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 547 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 549 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 551 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 556 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 557 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 563 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 565 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 570 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 571 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 572 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 577 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 578 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 579 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 584 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 585 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 586 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 587 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 588 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 589 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 594 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 595 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 596 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 597 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 599 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 604 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 605 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 606 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 607 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 609 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 611 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 616 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 617 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 618 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 619 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 625 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 627 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 629 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 631 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 637 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 638 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 640 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 642 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 647 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 653 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 658 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 663 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 669 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 675 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 680 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 686 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 692 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 697 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 703 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 709 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 714 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 720 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 725 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 731 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 737 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 743 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 749 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_ASSIGN, PN_BINARY);
							(yyval.node)->pn_u.binary.left = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.binary.right = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 761 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_ASSIGN, PN_BINARY);
							(yyval.node)->pn_u.binary.left = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.binary.right = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 773 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_ASSIGN, PN_BINARY);
							(yyval.node)->pn_u.binary.left = (yyvsp[(1) - (3)].node);
							(yyval.node)->pn_u.binary.right = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 783 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 784 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 785 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 786 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 787 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 788 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 789 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 790 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 791 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 792 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 793 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 794 "./parser/Grammar.y"
    { PRINT_LINE_NOTNEED;  (yyval.node) = NULL;;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 799 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 804 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 809 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = (yyvsp[(3) - (3)].node);;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 833 "./parser/Grammar.y"
    { PRINT_LINE;
									(yyval.node) = node_new (TOK_LC, PN_LIST);
									(yyval.node)->pn_u.list.head = NULL;
									node_correct_position_end ((yyval.node), (yyvsp[(2) - (2)].intValue));
;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 838 "./parser/Grammar.y"
    { PRINT_LINE;
									(yyval.node) = node_new (TOK_LC, PN_LIST);
									(yyval.node)->pn_u.list.head = (yyvsp[(2) - (3)].node);
									node_correct_position ((yyval.node), (yyvsp[(2) - (3)].node));
									node_correct_position_end ((yyval.node), (yyvsp[(3) - (3)].intValue));
;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 847 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (3)].node);;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 848 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (3)].node); AUTO_SEMICOLON (node_get_line ((yyvsp[(2) - (3)].node))); ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 852 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_VAR, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (1)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (1)].node));
;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 857 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_VAR, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (2)].node);
							(yyvsp[(1) - (2)].node)->pn_u.name.expr = (yyvsp[(2) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (2)].node));
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 865 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (3)].node);
							if ((yyvsp[(3) - (3)].node)) {
								g_assert ((yyvsp[(1) - (3)].node)->pn_arity == PN_LIST);
								(yyvsp[(3) - (3)].node)->pn_next = (yyvsp[(1) - (3)].node)->pn_u.list.head;
								(yyvsp[(1) - (3)].node)->pn_u.list.head = (yyvsp[(3) - (3)].node);
							}
							node_correct_position ((yyval.node), (yyvsp[(1) - (3)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 875 "./parser/Grammar.y"
    { PRINT_LINE;
							if ((yyvsp[(3) - (4)].node)) {
								g_assert ((yyvsp[(1) - (4)].node)->pn_arity == PN_LIST);
								(yyvsp[(3) - (4)].node)->pn_next = (yyvsp[(1) - (4)].node)->pn_u.list.head;
								(yyvsp[(1) - (4)].node)->pn_u.list.head = (yyvsp[(3) - (4)].node);
								(yyvsp[(3) - (4)].node)->pn_u.name.expr = (yyvsp[(4) - (4)].node);
							}
							node_correct_position ((yyval.node), (yyvsp[(1) - (4)].node));
							node_correct_position ((yyval.node), (yyvsp[(3) - (4)].node));
							node_correct_position ((yyval.node), (yyvsp[(4) - (4)].node));
;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 889 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_VAR, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (1)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (1)].node));
;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 894 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_VAR, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (2)].node);
							(yyvsp[(1) - (2)].node)->pn_u.name.expr = (yyvsp[(2) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (2)].node));
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 902 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 904 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 908 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (3)].node);;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 910 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (3)].node); AUTO_SEMICOLON (node_get_line ((yyvsp[(2) - (3)].node)));; ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 914 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_VAR, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (1)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (1)].node));
;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 920 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = (yyvsp[(1) - (3)].node);
							if ((yyvsp[(3) - (3)].node)) {
								g_assert ((yyvsp[(1) - (3)].node)->pn_arity == PN_LIST);
								(yyvsp[(3) - (3)].node)->pn_next = (yyvsp[(1) - (3)].node)->pn_u.list.head;
								(yyvsp[(1) - (3)].node)->pn_u.list.head = (yyvsp[(3) - (3)].node);
							}
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 932 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node)->pn_u.name.isconst = 1;;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 933 "./parser/Grammar.y"
    { PRINT_LINE_TODO;
							(yyval.node) = (yyvsp[(1) - (2)].node);
							(yyval.node)->pn_u.name.expr = (yyvsp[(2) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 941 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (2)].node);;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 945 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(2) - (2)].node);;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 949 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = node_new (TOK_SEMI, PN_UNARY); (yyval.node)->pn_u.unary.kid = NULL;;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 953 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (2)].node);;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 954 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(1) - (2)].node); AUTO_SEMICOLON (node_get_line ((yyvsp[(1) - (2)].node)));; ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 959 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(5) - (5)].node);;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 961 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(5) - (7)].node);;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 965 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 966 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = NULL; AUTO_SEMICOLON (node_get_line ((yyvsp[(5) - (7)].node)));;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 967 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(5) - (5)].node);;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 969 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(9) - (9)].node);;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 971 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 973 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(7) - (7)].node);;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 975 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(8) - (8)].node);;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 977 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = (yyvsp[(9) - (9)].node);;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 981 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 986 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 991 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 992 "./parser/Grammar.y"
    { PRINT_LINE;  AUTO_SEMICOLON (0); ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 993 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 994 "./parser/Grammar.y"
    { PRINT_LINE;  AUTO_SEMICOLON (node_get_line ((yyvsp[(2) - (3)].node))); ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 998 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 999 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL; AUTO_SEMICOLON (0);;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 1000 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1001 "./parser/Grammar.y"
    { PRINT_LINE; AUTO_SEMICOLON (node_get_line ((yyvsp[(2) - (3)].node))); ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1005 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = node_new (TOK_RETURN, PN_UNARY); (yyval.node)->pn_u.unary.kid = NULL;;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1006 "./parser/Grammar.y"
    { PRINT_LINE;  (yyval.node) = node_new (TOK_RETURN, PN_UNARY); (yyval.node)->pn_u.unary.kid = NULL; AUTO_SEMICOLON (0); ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1007 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_RETURN, PN_UNARY);
							(yyval.node)->pn_u.unary.kid = (yyvsp[(2) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(2) - (3)].node));
;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1012 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_RETURN, PN_UNARY);
							(yyval.node)->pn_u.unary.kid = (yyvsp[(2) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(2) - (3)].node));
							AUTO_SEMICOLON (node_get_line ((yyvsp[(2) - (3)].node)));
;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 1021 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1025 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1029 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 1031 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1035 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 1040 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 1041 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 1045 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 1046 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1050 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1051 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 1055 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1059 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 1060 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1064 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 1065 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1067 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1071 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1072 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1076 "./parser/Grammar.y"
    { PRINT_LINE;
											(yyval.node) = node_new ( TOK_FUNCTION, PN_FUNC);
											(yyval.node)->pn_u.func.name = (yyvsp[(2) - (7)].node);
											(yyval.node)->pn_u.func.body = (yyvsp[(6) - (7)].node);
											(yyval.node)->pn_u.func.args = NULL;
											node_correct_position ((yyval.node), (yyvsp[(2) - (7)].node));
											node_correct_position ((yyval.node), (yyvsp[(6) - (7)].node));
											node_correct_position_end ((yyval.node), (yyvsp[(7) - (7)].intValue));
;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1086 "./parser/Grammar.y"
    { PRINT_LINE;
											(yyval.node) = node_new ( TOK_FUNCTION, PN_FUNC);
											(yyval.node)->pn_u.func.name = (yyvsp[(2) - (8)].node);
											(yyval.node)->pn_u.func.body = (yyvsp[(7) - (8)].node);
											(yyval.node)->pn_u.func.args = (yyvsp[(4) - (8)].node);
											node_correct_position ((yyval.node), (yyvsp[(2) - (8)].node));
											node_correct_position ((yyval.node), (yyvsp[(7) - (8)].node));
											node_correct_position ((yyval.node), (yyvsp[(4) - (8)].node));
											node_correct_position_end ((yyval.node), (yyvsp[(8) - (8)].intValue));
;}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1099 "./parser/Grammar.y"
    { PRINT_LINE;
										(yyval.node) = node_new ( TOK_FUNCTION, PN_FUNC);
										(yyval.node)->pn_u.func.name = NULL;
										(yyval.node)->pn_u.func.body = (yyvsp[(5) - (6)].node);
										(yyval.node)->pn_u.func.args = NULL;
										node_correct_position ((yyval.node), (yyvsp[(5) - (6)].node));
										node_correct_position_end ((yyval.node), (yyvsp[(6) - (6)].intValue));
;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1108 "./parser/Grammar.y"
    { PRINT_LINE;
										(yyval.node) = node_new ( TOK_FUNCTION, PN_FUNC);
										(yyval.node)->pn_u.func.name = NULL;
										(yyval.node)->pn_u.func.body = (yyvsp[(6) - (7)].node);
										(yyval.node)->pn_u.func.args = (yyvsp[(3) - (7)].node);
										node_correct_position ((yyval.node), (yyvsp[(3) - (7)].node));
										node_correct_position ((yyval.node), (yyvsp[(6) - (7)].node));
										node_correct_position_end ((yyval.node), (yyvsp[(7) - (7)].intValue));
;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1117 "./parser/Grammar.y"
    { PRINT_LINE;
										(yyval.node) = node_new ( TOK_FUNCTION, PN_FUNC);
										(yyval.node)->pn_u.func.name = (yyvsp[(2) - (7)].node);
										(yyval.node)->pn_u.func.body = (yyvsp[(6) - (7)].node);
										(yyval.node)->pn_u.func.args = NULL;
										node_correct_position ((yyval.node), (yyvsp[(2) - (7)].node));
										node_correct_position ((yyval.node), (yyvsp[(6) - (7)].node));
										node_correct_position_end ((yyval.node), (yyvsp[(7) - (7)].intValue));
;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1127 "./parser/Grammar.y"
    { PRINT_LINE;
										(yyval.node) = node_new ( TOK_FUNCTION, PN_FUNC);
										(yyval.node)->pn_u.func.name = (yyvsp[(2) - (8)].node);
										(yyval.node)->pn_u.func.body = (yyvsp[(7) - (8)].node);
										(yyval.node)->pn_u.func.args = (yyvsp[(4) - (8)].node);
										node_correct_position ((yyval.node), (yyvsp[(2) - (8)].node));
										node_correct_position ((yyval.node), (yyvsp[(4) - (8)].node));
										node_correct_position ((yyval.node), (yyvsp[(7) - (8)].node));
										node_correct_position_end ((yyval.node), (yyvsp[(8) - (8)].intValue));
;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1140 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_LC, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (1)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (1)].node));
;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1145 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = (yyvsp[(1) - (3)].node);
							g_assert ((yyvsp[(1) - (3)].node)->pn_arity == PN_LIST);
							if ((yyvsp[(1) - (3)].node)->pn_u.list.head) {
								JSNode *i;
								for (i = (yyvsp[(1) - (3)].node)->pn_u.list.head; i->pn_next != NULL; i = i->pn_next)
									;
								i->pn_next = (yyvsp[(3) - (3)].node);
							} else (yyvsp[(1) - (3)].node)->pn_u.list.head = (yyvsp[(3) - (3)].node);
							node_correct_position ((yyval.node), (yyvsp[(3) - (3)].node));
;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1159 "./parser/Grammar.y"
    { PRINT_LINE_TODO;  (yyval.node) = NULL;;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1160 "./parser/Grammar.y"
    { PRINT_LINE; (yyval.node) = (yyvsp[(1) - (1)].node);;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1164 "./parser/Grammar.y"
    { PRINT_LINE; global = NULL;;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1165 "./parser/Grammar.y"
    { PRINT_LINE;  global = (yyvsp[(1) - (1)].node);;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1169 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = node_new (TOK_LC, PN_LIST);
							(yyval.node)->pn_u.list.head = (yyvsp[(1) - (1)].node);
							node_correct_position ((yyval.node), (yyvsp[(1) - (1)].node));
;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1174 "./parser/Grammar.y"
    { PRINT_LINE;
							(yyval.node) = (yyvsp[(1) - (2)].node);
							g_assert ((yyvsp[(1) - (2)].node)->pn_arity == PN_LIST);
							if ((yyvsp[(1) - (2)].node)->pn_u.list.head) {
								JSNode *i;
								for (i = (yyvsp[(1) - (2)].node)->pn_u.list.head; i->pn_next != NULL; i = i->pn_next) {
								}
								i->pn_next = (yyvsp[(2) - (2)].node);
							} else (yyvsp[(1) - (2)].node)->pn_u.list.head = (yyvsp[(2) - (2)].node);
							node_correct_position ((yyval.node), (yyvsp[(2) - (2)].node));
;}
    break;



/* Line 1455 of yacc.c  */
#line 4421 "./parser/js-parser-y-tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  yyps->yynew = 1;

yypushreturn:
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1187 "./parser/Grammar.y"

#undef GLOBAL_DATA

void yyerror (char* msg)
{
//	puts (msg);
}


%pure_parser
%define api.push_pull "both"
%{

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

%}

%union {
    int intValue;
    JSNode* node;
    struct {
	char    *iname;
	JSTokenPos pos;
    } name;
}

%start Program

/* literals */
%token NULLTOKEN TRUETOKEN FALSETOKEN

/* keywords */
%token BREAK CASE DEFAULT FOR NEW VAR CONSTTOKEN CONTINUE
%token FUNCTION RETURN VOIDTOKEN DELETETOKEN
%token IF THISTOKEN DO WHILE INTOKEN INSTANCEOF TYPEOF
%token SWITCH WITH RESERVED
%token THROW TRY CATCH FINALLY
%token DEBUGGER

/* give an if without an else higher precedence than an else to resolve the ambiguity */
%nonassoc IF_WITHOUT_ELSE
%nonassoc ELSE

/* punctuators */
%token EQEQ NE                     /* == and != */
%token STREQ STRNEQ                /* === and !== */
%token LE GE                       /* < and > */
%token OR AND                      /* || and && */
%token PLUSPLUS MINUSMINUS         /* ++ and --  */
%token LSHIFT                      /* << */
%token RSHIFT URSHIFT              /* >> and >>> */
%token PLUSEQUAL MINUSEQUAL        /* += and -= */
%token MULTEQUAL DIVEQUAL          /* *= and /= */
%token LSHIFTEQUAL                 /* <<= */
%token RSHIFTEQUAL URSHIFTEQUAL    /* >>= and >>>= */
%token ANDEQUAL MODEQUAL           /* &= and %= */
%token XOREQUAL OREQUAL            /* ^= and |= */
%token <intValue> OPENBRACE        /* { (with char offset) */
%token <intValue> CLOSEBRACE       /* } (line number) */

/* terminal types */
%token <name> IDENT_IN
%token <node> NUMBER
%token <node> STRING

/* automatically inserted semicolon */
%token AUTOPLUSPLUS AUTOMINUSMINUS

/* non-terminal types */
%type <node>  Literal ArrayLiteral IDENT AssignmentOperator

%type <node>  PrimaryExpr PrimaryExprNoBrace
%type <node>  MemberExpr MemberExprNoBF /* BF => brace or function */
%type <node>  NewExpr NewExprNoBF
%type <node>  CallExpr CallExprNoBF
%type <node>  LeftHandSideExpr LeftHandSideExprNoBF
%type <node>  PostfixExpr PostfixExprNoBF
%type <node>  UnaryExpr UnaryExprNoBF UnaryExprCommon
%type <node>  MultiplicativeExpr MultiplicativeExprNoBF
%type <node>  AdditiveExpr AdditiveExprNoBF
%type <node>  ShiftExpr ShiftExprNoBF
%type <node>  RelationalExpr RelationalExprNoIn RelationalExprNoBF
%type <node>  EqualityExpr EqualityExprNoIn EqualityExprNoBF
%type <node>  BitwiseANDExpr BitwiseANDExprNoIn BitwiseANDExprNoBF
%type <node>  BitwiseXORExpr BitwiseXORExprNoIn BitwiseXORExprNoBF
%type <node>  BitwiseORExpr BitwiseORExprNoIn BitwiseORExprNoBF
%type <node>  LogicalANDExpr LogicalANDExprNoIn LogicalANDExprNoBF
%type <node>  LogicalORExpr LogicalORExprNoIn LogicalORExprNoBF
%type <node>  ConditionalExpr ConditionalExprNoIn ConditionalExprNoBF
%type <node>  AssignmentExpr AssignmentExprNoIn AssignmentExprNoBF
%type <node>  Expr ExprNoIn ExprNoBF

%type <node>  ExprOpt ExprNoInOpt

%type <node>   Statement Block
%type <node>   VariableStatement ConstStatement EmptyStatement ExprStatement
%type <node>   IfStatement IterationStatement ContinueStatement
%type <node>   BreakStatement ReturnStatement WithStatement
%type <node>   SwitchStatement LabelledStatement
%type <node>   ThrowStatement TryStatement
%type <node>   DebuggerStatement

%type <node>  Initializer InitializerNoIn
%type <node>   FunctionDeclaration
%type <node>    FunctionExpr
%type <node> FunctionBody
%type <node>  SourceElements
%type <node>   FormalParameterList
%type <node>   Arguments
%type <node>    ArgumentList
%type <node>     VariableDeclarationList VariableDeclarationListNoIn
%type <node>   ConstDeclarationList
%type <node>   ConstDeclaration
%type <node>   CaseBlock
%type <node>  CaseClause DefaultClause
%type <node>      CaseClauses CaseClausesOpt
%type <node>        Elision ElisionOpt
%type <node>     ElementList
%type <node>    Property
%type <node>    PropertyList RegExp Disjunction Alternative Term Assertion QuantifierPrefix Quantifier PatternCharacter ClassRanges CharacterClass RAtom
%%

IDENT:
    IDENT_IN                           { PRINT_LINE; $$ = node_new (TOK_NAME, PN_NAME); $$->pn_u.name.name = $1.iname; $$->pn_pos = $1.pos; $$->pn_u.name.isconst = 0;}
;

Disjunction:
    Alternative                        { PRINT_LINE; $$ = NULL;}
  | Alternative '|' Disjunction        { PRINT_LINE; $$ = NULL;}
;

RegExp:
    '/' Disjunction '/'                { PRINT_LINE_TODO; $$ = NULL;}
  | '/' Disjunction '/' IDENT          { PRINT_LINE_TODO; $$ = NULL;}
;

Alternative:
    /* nothing */                      { PRINT_LINE_TODO;  $$ = NULL;}
  | Alternative Term                   { PRINT_LINE_TODO;  $$ = NULL;}
;

Term:
    Assertion                          { PRINT_LINE_TODO;  $$ = NULL;}
  | RAtom                              { PRINT_LINE_TODO;  $$ = NULL;}
  | RAtom Quantifier                   { PRINT_LINE_TODO;  $$ = NULL;}
;

Assertion:
    '^'                                { PRINT_LINE_TODO;  $$ = NULL;}
  | '$'                                { PRINT_LINE_TODO;  $$ = NULL;}
/*|  \ b |  \ B*/
;

Quantifier:
    QuantifierPrefix
  | QuantifierPrefix '?'
;

QuantifierPrefix:
    '*'                                 { PRINT_LINE_TODO;  $$ = NULL;}
  | '+'                                 { PRINT_LINE_TODO;  $$ = NULL;}
  | '?'                                 { PRINT_LINE_TODO;  $$ = NULL;}
  | '{' NUMBER '}'                      { PRINT_LINE_TODO;  $$ = NULL;}
  | '{' NUMBER ',' '}'                  { PRINT_LINE_TODO;  $$ = NULL;}
  | '{' NUMBER ',' NUMBER '}'           { PRINT_LINE_TODO;  $$ = NULL;}
;

RAtom:
    PatternCharacter                    { PRINT_LINE_TODO;  $$ = NULL;}
  | '.'                                 { PRINT_LINE_TODO;  $$ = NULL;}
  | '\\' IDENT                          { PRINT_LINE_TODO;  $$ = NULL;/* TODO: FIX AtomEscape = IDENT*/ }   
  | '\\' '.'                            { PRINT_LINE_TODO;  $$ = NULL;/* TODO: FIX AtomEscape = IDENT*/ }   
  | CharacterClass                      { PRINT_LINE_TODO;  $$ = NULL;}
  | '(' Disjunction ')'                 { PRINT_LINE_TODO;  $$ = NULL;}
  | '(' '?' ':' Disjunction ')'         { PRINT_LINE_TODO;  $$ = NULL;}
  | '(' '?' '=' Disjunction ')'         { PRINT_LINE_TODO;  $$ = NULL;}
  | '(' '?' '!' Disjunction ')'         { PRINT_LINE_TODO;  $$ = NULL;}
;

PatternCharacter:
    IDENT                               { PRINT_LINE_TODO;  $$ = NULL;}
;

CharacterClass:
    '[' ClassRanges ']'                 { PRINT_LINE_TODO;  $$ = NULL;}
  | '[' '^' ClassRanges ']'             { PRINT_LINE_TODO;  $$ = NULL;}
;

ClassRanges:
    NUMBER                              { PRINT_LINE_TODO;  $$ = NULL;}
  | IDENT                               { PRINT_LINE_TODO;  $$ = NULL;}
  | ClassRanges '-' ClassRanges         { PRINT_LINE_TODO;  $$ = NULL;}
  | ClassRanges ClassRanges             { PRINT_LINE_TODO;  $$ = NULL;}
;

Literal:
    NULLTOKEN                           { PRINT_LINE; $$ = node_new (TOK_PRIMARY, PN_NULLARY); $$->pn_op = JSOP_NULL;}
  | TRUETOKEN                           { PRINT_LINE; $$ = node_new (TOK_PRIMARY, PN_NULLARY); $$->pn_op = JSOP_TRUE;}
  | FALSETOKEN                          { PRINT_LINE; $$ = node_new (TOK_PRIMARY, PN_NULLARY); $$->pn_op = JSOP_FALSE;}
  | NUMBER                              { PRINT_LINE; $$ = node_new (TOK_NUMBER, PN_NULLARY);}
  | STRING                              { PRINT_LINE; $$ = node_new (TOK_STRING, PN_NULLARY);}
;

Property:
    IDENT ':' AssignmentExpr            { PRINT_LINE;
							$$ = node_new (TOK_COLON, PN_BINARY);
							$$->pn_u.binary.left = $1;
							$$->pn_u.binary.right = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
  | STRING ':' AssignmentExpr           { PRINT_LINE;
							$$ = node_new (TOK_COLON, PN_BINARY);
							$$->pn_u.binary.left = $1;
							$$->pn_u.binary.right = $3;
							node_correct_position ($$, $3);
}
  | NUMBER ':' AssignmentExpr           { PRINT_LINE;
							$$ = node_new (TOK_COLON, PN_BINARY);
							$$->pn_u.binary.left = $1;
							$$->pn_u.binary.right = $3;
							node_correct_position ($$, $3);							
}
  | IDENT IDENT '(' ')' OPENBRACE FunctionBody CLOSEBRACE    { PRINT_LINE_TODO;  $$ = NULL;}
  | IDENT IDENT '(' FormalParameterList ')' OPENBRACE FunctionBody CLOSEBRACE
                                                             { PRINT_LINE_TODO;  $$ = NULL;}
;

PropertyList:
    Property                            { PRINT_LINE;
							$$ = node_new (TOK_RC, PN_LIST);
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
}
  | PropertyList ',' Property           { PRINT_LINE; 
							$$ = $1;
							node_correct_position ($$, $3);
							g_assert ($1->pn_arity == PN_LIST);
							if ($1->pn_u.list.head) {
								JSNode *i;
								for (i = $1->pn_u.list.head; i->pn_next != NULL; i = i->pn_next)
									;
								i->pn_next = $3;
							} else $1->pn_u.list.head = $3;
}
;

PrimaryExpr:
    PrimaryExprNoBrace
  | OPENBRACE CLOSEBRACE                            { PRINT_LINE_TODO;  $$ = NULL;}
  | OPENBRACE PropertyList CLOSEBRACE               { PRINT_LINE;
									$$ = $2;
									node_correct_position_end ($$, $3);
}
  /* allow extra comma, see http://bugs.webkit.org/show_bug.cgi?id=5939 */
  | OPENBRACE PropertyList ',' CLOSEBRACE           { PRINT_LINE;
									$$ = $2;
									node_correct_position_end ($$, $4);
}
;

PrimaryExprNoBrace:
    THISTOKEN                           { PRINT_LINE;  $$ = node_new (TOK_PRIMARY, PN_NULLARY); $$->pn_op = JSOP_THIS;}
  | Literal
  | ArrayLiteral
  | IDENT                               { PRINT_LINE;  $$ = $1;}
  | '(' Expr ')'                        { PRINT_LINE;  $$ = $2;}
;

ArrayLiteral:
    '[' ElisionOpt ']'                  { PRINT_LINE_TODO;  $$ = NULL;}
  | '[' ElementList ']'                 { PRINT_LINE_TODO;  $$ = NULL;}
  | '[' ElementList ',' ElisionOpt ']'  { PRINT_LINE_TODO;  $$ = NULL;}
;

ElementList:
    ElisionOpt AssignmentExpr           { PRINT_LINE_TODO;  $$ = NULL;}
  | ElementList ',' ElisionOpt AssignmentExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

ElisionOpt:
    /* nothing */                       { PRINT_LINE_TODO;  $$ = NULL;}
  | Elision
;

Elision:
    ','                                 { PRINT_LINE_TODO;  $$ = NULL;}
  | Elision ','                         { PRINT_LINE_TODO;  $$ = NULL;}
;

MemberExpr:
    PrimaryExpr
  | RegExp
  | FunctionExpr                        { PRINT_LINE;  $$ = $1;}
  | MemberExpr '[' Expr ']'             { PRINT_LINE_TODO;  $$ = NULL;}
  | MemberExpr '.' IDENT                { PRINT_LINE;
							$$ = node_new ( TOK_DOT, PN_NAME);
							$$->pn_u.name.expr = $1;
							$$->pn_u.name.name = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
  | NEW MemberExpr Arguments            { PRINT_LINE;
							$$ = node_new ( TOK_NEW, PN_LIST);
							$$->pn_u.list.head = $2;
							$2->pn_next = $3;
							node_correct_position ($$, $2);
							node_correct_position ($$, $3);
}
;

MemberExprNoBF:
    PrimaryExprNoBrace
  | MemberExprNoBF '[' Expr ']'         { PRINT_LINE_TODO;  $$ = NULL;}
  | MemberExprNoBF '.' IDENT            { PRINT_LINE;
							$$ = node_new ( TOK_DOT, PN_NAME);
							$$->pn_u.name.expr = $1;
							$$->pn_u.name.name = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
  | NEW MemberExpr Arguments            { PRINT_LINE_TODO;  $$ = NULL;}
;

NewExpr:
    MemberExpr
  | NEW NewExpr                         { PRINT_LINE_TODO;  $$ = NULL;}
;

NewExprNoBF:
    MemberExprNoBF
  | NEW NewExpr                         { PRINT_LINE_TODO;  $$ = NULL;}
;

CallExpr:
    MemberExpr Arguments                { PRINT_LINE;
							$$ = node_new ( TOK_LP, PN_LIST);
							$1->pn_next = $2;
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
							node_correct_position ($$, $2);
}
  | CallExpr Arguments                  { PRINT_LINE;
							$$ = node_new ( TOK_LP, PN_LIST);
							$1->pn_next = $2;
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
							node_correct_position ($$, $2);
}
  | CallExpr '[' Expr ']'               { PRINT_LINE_TODO;  $$ = NULL;}
  | CallExpr '.' IDENT                  { PRINT_LINE;
							$$ = node_new ( TOK_DOT, PN_NAME);
							$$->pn_u.name.expr = $1;
							$$->pn_u.name.name = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
;

CallExprNoBF:
    MemberExprNoBF Arguments            { PRINT_LINE;
							$$ = node_new ( TOK_LP, PN_LIST);
							$1->pn_next = $2;
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
							node_correct_position ($$, $2);
}
  | CallExprNoBF Arguments              { PRINT_LINE;
							$$ = node_new ( TOK_LP, PN_LIST);
							$1->pn_next = $2;
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
							node_correct_position ($$, $2);
}
  | CallExprNoBF '[' Expr ']'           { PRINT_LINE_TODO;  $$ = NULL;}
  | CallExprNoBF '.' IDENT              { PRINT_LINE;
							$$ = node_new ( TOK_DOT, PN_NAME);
							$$->pn_u.name.expr = $1;
							$$->pn_u.name.name = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
;

Arguments:
    '(' ')'                             { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | '(' ArgumentList ')'                { PRINT_LINE;  $$ = $2;}
;

ArgumentList:
    AssignmentExpr                      { PRINT_LINE;  $$ = $1;}
  | ArgumentList ',' AssignmentExpr     { PRINT_LINE;
							$$ = $1;
							if ($1) $1->pn_next = $3;
							else $$ = $3;
							node_correct_position ($$, $3);
}
;

LeftHandSideExpr:
    NewExpr
  | CallExpr
;

LeftHandSideExprNoBF:
    NewExprNoBF
  | CallExprNoBF
;

PostfixExpr:
    LeftHandSideExpr
  | LeftHandSideExpr PLUSPLUS           { PRINT_LINE;  $$ = $1;}
  | LeftHandSideExpr MINUSMINUS         { PRINT_LINE;  $$ = $1;}
;

PostfixExprNoBF:
    LeftHandSideExprNoBF
  | LeftHandSideExprNoBF PLUSPLUS       { PRINT_LINE_TODO;  $$ = NULL;}
  | LeftHandSideExprNoBF MINUSMINUS     { PRINT_LINE_TODO;  $$ = NULL;}
;

UnaryExprCommon:
    DELETETOKEN UnaryExpr               { PRINT_LINE_TODO;  $$ = NULL;}
  | VOIDTOKEN UnaryExpr                 { PRINT_LINE_TODO;  $$ = NULL;}
  | TYPEOF UnaryExpr                    { PRINT_LINE_TODO;  $$ = NULL;}
  | PLUSPLUS UnaryExpr                  { PRINT_LINE_TODO;  $$ = NULL;}
  | AUTOPLUSPLUS UnaryExpr              { PRINT_LINE_TODO;  $$ = NULL;}
  | MINUSMINUS UnaryExpr                { PRINT_LINE_TODO;  $$ = NULL;}
  | AUTOMINUSMINUS UnaryExpr            { PRINT_LINE_TODO;  $$ = NULL;}
  | '+' UnaryExpr                       { PRINT_LINE_TODO;  $$ = NULL;}
  | '-' UnaryExpr                       { PRINT_LINE_TODO;  $$ = NULL;}
  | '~' UnaryExpr                       { PRINT_LINE_TODO;  $$ = NULL;}
  | '!' UnaryExpr                       { PRINT_LINE_TODO;  $$ = NULL;}

UnaryExpr:
    PostfixExpr
  | UnaryExprCommon
;

UnaryExprNoBF:
    PostfixExprNoBF
  | UnaryExprCommon
;

MultiplicativeExpr:
    UnaryExpr
  | MultiplicativeExpr '*' UnaryExpr    { PRINT_LINE;  $$ = $3;}
  | MultiplicativeExpr '/' UnaryExpr    { PRINT_LINE;  $$ = $3;}
  | MultiplicativeExpr '%' UnaryExpr    { PRINT_LINE;  $$ = $3;}
;

MultiplicativeExprNoBF:
    UnaryExprNoBF
  | MultiplicativeExprNoBF '*' UnaryExpr
                                        { PRINT_LINE_TODO;  $$ = $3;}
  | MultiplicativeExprNoBF '/' UnaryExpr
                                        { PRINT_LINE_TODO;  $$ = $3;}
  | MultiplicativeExprNoBF '%' UnaryExpr
                                        { PRINT_LINE_TODO;  $$ = $3;}
;

AdditiveExpr:
    MultiplicativeExpr
  | AdditiveExpr '+' MultiplicativeExpr { PRINT_LINE;  $$ = $3;}
  | AdditiveExpr '-' MultiplicativeExpr { PRINT_LINE;  $$ = $3;}
;

AdditiveExprNoBF:
    MultiplicativeExprNoBF
  | AdditiveExprNoBF '+' MultiplicativeExpr
                                        { PRINT_LINE;  $$ = $3;}
  | AdditiveExprNoBF '-' MultiplicativeExpr
                                        { PRINT_LINE;  $$ = $3;}
;

ShiftExpr:
    AdditiveExpr
  | ShiftExpr LSHIFT AdditiveExpr       { PRINT_LINE_TODO;  $$ = NULL;}
  | ShiftExpr RSHIFT AdditiveExpr       { PRINT_LINE_TODO;  $$ = NULL;}
  | ShiftExpr URSHIFT AdditiveExpr      { PRINT_LINE_TODO;  $$ = NULL;}
;

ShiftExprNoBF:
    AdditiveExprNoBF
  | ShiftExprNoBF LSHIFT AdditiveExpr   { PRINT_LINE_TODO;  $$ = NULL;}
  | ShiftExprNoBF RSHIFT AdditiveExpr   { PRINT_LINE_TODO;  $$ = NULL;}
  | ShiftExprNoBF URSHIFT AdditiveExpr  { PRINT_LINE_TODO;  $$ = NULL;}
;

RelationalExpr:
    ShiftExpr
  | RelationalExpr '<' ShiftExpr        { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExpr '>' ShiftExpr        { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExpr LE ShiftExpr         { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExpr GE ShiftExpr         { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExpr INSTANCEOF ShiftExpr { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExpr INTOKEN ShiftExpr    { PRINT_LINE_TODO;  $$ = NULL;}
;

RelationalExprNoIn:
    ShiftExpr
  | RelationalExprNoIn '<' ShiftExpr    { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoIn '>' ShiftExpr    { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoIn LE ShiftExpr     { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoIn GE ShiftExpr     { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoIn INSTANCEOF ShiftExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

RelationalExprNoBF:
    ShiftExprNoBF
  | RelationalExprNoBF '<' ShiftExpr    { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoBF '>' ShiftExpr    { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoBF LE ShiftExpr     { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoBF GE ShiftExpr     { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoBF INSTANCEOF ShiftExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | RelationalExprNoBF INTOKEN ShiftExpr 
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

EqualityExpr:
    RelationalExpr
  | EqualityExpr EQEQ RelationalExpr    { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExpr NE RelationalExpr      { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExpr STREQ RelationalExpr   { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExpr STRNEQ RelationalExpr  { PRINT_LINE_TODO;  $$ = NULL;}
;

EqualityExprNoIn:
    RelationalExprNoIn
  | EqualityExprNoIn EQEQ RelationalExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExprNoIn NE RelationalExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExprNoIn STREQ RelationalExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExprNoIn STRNEQ RelationalExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

EqualityExprNoBF:
    RelationalExprNoBF
  | EqualityExprNoBF EQEQ RelationalExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExprNoBF NE RelationalExpr  { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExprNoBF STREQ RelationalExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | EqualityExprNoBF STRNEQ RelationalExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseANDExpr:
    EqualityExpr
  | BitwiseANDExpr '&' EqualityExpr     { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseANDExprNoIn:
    EqualityExprNoIn
  | BitwiseANDExprNoIn '&' EqualityExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseANDExprNoBF:
    EqualityExprNoBF
  | BitwiseANDExprNoBF '&' EqualityExpr { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseXORExpr:
    BitwiseANDExpr
  | BitwiseXORExpr '^' BitwiseANDExpr   { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseXORExprNoIn:
    BitwiseANDExprNoIn
  | BitwiseXORExprNoIn '^' BitwiseANDExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseXORExprNoBF:
    BitwiseANDExprNoBF
  | BitwiseXORExprNoBF '^' BitwiseANDExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseORExpr:
    BitwiseXORExpr
  | BitwiseORExpr '|' BitwiseXORExpr    { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseORExprNoIn:
    BitwiseXORExprNoIn
  | BitwiseORExprNoIn '|' BitwiseXORExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

BitwiseORExprNoBF:
    BitwiseXORExprNoBF
  | BitwiseORExprNoBF '|' BitwiseXORExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

LogicalANDExpr:
    BitwiseORExpr
  | LogicalANDExpr AND BitwiseORExpr    { PRINT_LINE_TODO;  $$ = NULL;}
;

LogicalANDExprNoIn:
    BitwiseORExprNoIn
  | LogicalANDExprNoIn AND BitwiseORExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

LogicalANDExprNoBF:
    BitwiseORExprNoBF
  | LogicalANDExprNoBF AND BitwiseORExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

LogicalORExpr:
    LogicalANDExpr
  | LogicalORExpr OR LogicalANDExpr     { PRINT_LINE_TODO;  $$ = NULL;}
;

LogicalORExprNoIn:
    LogicalANDExprNoIn
  | LogicalORExprNoIn OR LogicalANDExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

LogicalORExprNoBF:
    LogicalANDExprNoBF
  | LogicalORExprNoBF OR LogicalANDExpr { PRINT_LINE_TODO;  $$ = NULL;}
;

ConditionalExpr:
    LogicalORExpr
  | LogicalORExpr '?' AssignmentExpr ':' AssignmentExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

ConditionalExprNoIn:
    LogicalORExprNoIn
  | LogicalORExprNoIn '?' AssignmentExprNoIn ':' AssignmentExprNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

ConditionalExprNoBF:
    LogicalORExprNoBF
  | LogicalORExprNoBF '?' AssignmentExpr ':' AssignmentExpr
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

AssignmentExpr:
    ConditionalExpr
  | LeftHandSideExpr AssignmentOperator AssignmentExpr
                                        { PRINT_LINE;
							$$ = node_new (TOK_ASSIGN, PN_BINARY);
							$$->pn_u.binary.left = $1;
							$$->pn_u.binary.right = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
;

AssignmentExprNoIn:
    ConditionalExprNoIn
  | LeftHandSideExpr AssignmentOperator AssignmentExprNoIn
                                        { PRINT_LINE;
							$$ = node_new (TOK_ASSIGN, PN_BINARY);
							$$->pn_u.binary.left = $1;
							$$->pn_u.binary.right = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
;

AssignmentExprNoBF:
    ConditionalExprNoBF
  | LeftHandSideExprNoBF AssignmentOperator AssignmentExpr
                                        { PRINT_LINE;
							$$ = node_new (TOK_ASSIGN, PN_BINARY);
							$$->pn_u.binary.left = $1;
							$$->pn_u.binary.right = $3;
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
;

AssignmentOperator:
    '='          { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | PLUSEQUAL    { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | MINUSEQUAL   { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | MULTEQUAL    { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | DIVEQUAL     { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | LSHIFTEQUAL  { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | RSHIFTEQUAL  { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | URSHIFTEQUAL { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | ANDEQUAL { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | XOREQUAL { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | OREQUAL  { PRINT_LINE_NOTNEED;  $$ = NULL;}
  | MODEQUAL { PRINT_LINE_NOTNEED;  $$ = NULL;}
;

Expr:
    AssignmentExpr
  | Expr ',' AssignmentExpr             { PRINT_LINE_TODO;  $$ = $3;}
;

ExprNoIn:
    AssignmentExprNoIn
  | ExprNoIn ',' AssignmentExprNoIn     { PRINT_LINE_TODO;  $$ = $3;}
;

ExprNoBF:
    AssignmentExprNoBF
  | ExprNoBF ',' AssignmentExpr         { PRINT_LINE_TODO;  $$ = $3;}
;

Statement:
    Block
  | VariableStatement
  | ConstStatement
  | FunctionDeclaration
  | EmptyStatement
  | ExprStatement
  | IfStatement
  | IterationStatement
  | ContinueStatement
  | BreakStatement
  | ReturnStatement
  | WithStatement
  | SwitchStatement
  | LabelledStatement
  | ThrowStatement
  | TryStatement
  | DebuggerStatement
;

Block:
    OPENBRACE CLOSEBRACE                            { PRINT_LINE;
									$$ = node_new (TOK_LC, PN_LIST);
									$$->pn_u.list.head = NULL;
									node_correct_position_end ($$, $2);
}
  | OPENBRACE SourceElements CLOSEBRACE             { PRINT_LINE;
									$$ = node_new (TOK_LC, PN_LIST);
									$$->pn_u.list.head = $2;
									node_correct_position ($$, $2);
									node_correct_position_end ($$, $3);
}
;

VariableStatement:
    VAR VariableDeclarationList ';'     { PRINT_LINE;  $$ = $2;}
  | VAR VariableDeclarationList error   { PRINT_LINE;  $$ = $2; AUTO_SEMICOLON (node_get_line ($2)); }
;

VariableDeclarationList:
    IDENT                               { PRINT_LINE;
							$$ = node_new (TOK_VAR, PN_LIST);
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
}
  | IDENT Initializer                   { PRINT_LINE;
							$$ = node_new (TOK_VAR, PN_LIST);
							$$->pn_u.list.head = $1;
							$1->pn_u.name.expr = $2;
							node_correct_position ($$, $1);
							node_correct_position ($$, $2);
}
  | VariableDeclarationList ',' IDENT
                                        { PRINT_LINE;  $$ = $1;
							if ($3) {
								g_assert ($1->pn_arity == PN_LIST);
								$3->pn_next = $1->pn_u.list.head;
								$1->pn_u.list.head = $3;
							}
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
}
  | VariableDeclarationList ',' IDENT Initializer
                                        { PRINT_LINE;
							if ($3) {
								g_assert ($1->pn_arity == PN_LIST);
								$3->pn_next = $1->pn_u.list.head;
								$1->pn_u.list.head = $3;
								$3->pn_u.name.expr = $4;
							}
							node_correct_position ($$, $1);
							node_correct_position ($$, $3);
							node_correct_position ($$, $4);
}
;

VariableDeclarationListNoIn:
    IDENT                               { PRINT_LINE;
							$$ = node_new (TOK_VAR, PN_LIST);
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
}
  | IDENT InitializerNoIn               { PRINT_LINE;
							$$ = node_new (TOK_VAR, PN_LIST);
							$$->pn_u.list.head = $1;
							$1->pn_u.name.expr = $2;
							node_correct_position ($$, $1);
							node_correct_position ($$, $2);
}
  | VariableDeclarationListNoIn ',' IDENT
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | VariableDeclarationListNoIn ',' IDENT InitializerNoIn
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

ConstStatement:
    CONSTTOKEN ConstDeclarationList ';' { PRINT_LINE;  $$ = $2;}
  | CONSTTOKEN ConstDeclarationList error
                                        { PRINT_LINE;  $$ = $2; AUTO_SEMICOLON (node_get_line ($2));; }
;

ConstDeclarationList:
    ConstDeclaration                    { PRINT_LINE;
							$$ = node_new (TOK_VAR, PN_LIST);
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
}
  | ConstDeclarationList ',' ConstDeclaration
                                        { PRINT_LINE;
							$$ = $1;
							if ($3) {
								g_assert ($1->pn_arity == PN_LIST);
								$3->pn_next = $1->pn_u.list.head;
								$1->pn_u.list.head = $3;
							}
							node_correct_position ($$, $3);
}
;

ConstDeclaration:
    IDENT                               { PRINT_LINE;  $$ = $1; $1->pn_u.name.isconst = 1;}
  | IDENT Initializer                   { PRINT_LINE_TODO;
							$$ = $1;
							$$->pn_u.name.expr = $2;
							node_correct_position ($$, $2);
}
;

Initializer:
    '=' AssignmentExpr                  { PRINT_LINE;  $$ = $2;}
;

InitializerNoIn:
    '=' AssignmentExprNoIn              { PRINT_LINE;  $$ = $2;}
;

EmptyStatement:
    ';'                                 { PRINT_LINE;  $$ = node_new (TOK_SEMI, PN_UNARY); $$->pn_u.unary.kid = NULL;}
;

ExprStatement:
    ExprNoBF ';'                        { PRINT_LINE;  $$ = $1;}
  | ExprNoBF error                      { PRINT_LINE;  $$ = $1; AUTO_SEMICOLON (node_get_line ($1));; }
;

IfStatement:
    IF '(' Expr ')' Statement %prec IF_WITHOUT_ELSE
                                        { PRINT_LINE;  $$ = $5;}
  | IF '(' Expr ')' Statement ELSE Statement
                                        { PRINT_LINE;  $$ = $5;}
;

IterationStatement:
    DO Statement WHILE '(' Expr ')' ';'    { PRINT_LINE_TODO;  $$ = NULL;}
  | DO Statement WHILE '(' Expr ')' error  { PRINT_LINE;  $$ = NULL; AUTO_SEMICOLON (node_get_line ($5));}
  | WHILE '(' Expr ')' Statement        { PRINT_LINE;  $$ = $5;}
  | FOR '(' ExprNoInOpt ';' ExprOpt ';' ExprOpt ')' Statement
                                        { PRINT_LINE;  $$ = $9;}
  | FOR '(' VAR VariableDeclarationListNoIn ';' ExprOpt ';' ExprOpt ')' Statement
                                        { PRINT_LINE_TODO;  $$ = NULL;}
  | FOR '(' LeftHandSideExpr INTOKEN Expr ')' Statement
                                        { PRINT_LINE;  $$ = $7;}
  | FOR '(' VAR IDENT INTOKEN Expr ')' Statement
                                        { PRINT_LINE;  $$ = $8;}
  | FOR '(' VAR IDENT InitializerNoIn INTOKEN Expr ')' Statement
                                        { PRINT_LINE;  $$ = $9;}
;

ExprOpt:
    /* nothing */                       { PRINT_LINE_TODO;  $$ = NULL;}
  | Expr
;

ExprNoInOpt:
    /* nothing */                       { PRINT_LINE_TODO;  $$ = NULL;}
  | ExprNoIn
;

ContinueStatement:
    CONTINUE ';'                        { PRINT_LINE_TODO;  $$ = NULL;}
  | CONTINUE error                      { PRINT_LINE;  AUTO_SEMICOLON (0); }
  | CONTINUE IDENT ';'                  { PRINT_LINE_TODO;  $$ = NULL;}
  | CONTINUE IDENT error                { PRINT_LINE;  AUTO_SEMICOLON (node_get_line ($2)); }
;

BreakStatement:
    BREAK ';'                           { PRINT_LINE_TODO;  $$ = NULL;}
  | BREAK error                         { PRINT_LINE_TODO;  $$ = NULL; AUTO_SEMICOLON (0);}
  | BREAK IDENT ';'                     { PRINT_LINE_TODO;  $$ = NULL;}
  | BREAK IDENT error                   { PRINT_LINE; AUTO_SEMICOLON (node_get_line ($2)); }
;

ReturnStatement:
    RETURN ';'                          { PRINT_LINE;  $$ = node_new (TOK_RETURN, PN_UNARY); $$->pn_u.unary.kid = NULL;}
  | RETURN error                        { PRINT_LINE;  $$ = node_new (TOK_RETURN, PN_UNARY); $$->pn_u.unary.kid = NULL; AUTO_SEMICOLON (0); }
  | RETURN Expr ';'                     { PRINT_LINE;
							$$ = node_new (TOK_RETURN, PN_UNARY);
							$$->pn_u.unary.kid = $2;
							node_correct_position ($$, $2);
}
  | RETURN Expr error                   { PRINT_LINE;
							$$ = node_new (TOK_RETURN, PN_UNARY);
							$$->pn_u.unary.kid = $2;
							node_correct_position ($$, $2);
							AUTO_SEMICOLON (node_get_line ($2));
}
;

WithStatement:
    WITH '(' Expr ')' Statement         { PRINT_LINE_TODO;  $$ = NULL;}
;

SwitchStatement:
    SWITCH '(' Expr ')' CaseBlock       { PRINT_LINE_TODO;  $$ = NULL;}
;

CaseBlock:
    OPENBRACE CaseClausesOpt CLOSEBRACE              { PRINT_LINE_TODO;  $$ = NULL;}
  | OPENBRACE CaseClausesOpt DefaultClause CaseClausesOpt CLOSEBRACE
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

CaseClausesOpt:
/* nothing */                       { PRINT_LINE_TODO;  $$ = NULL;}
  | CaseClauses
;

CaseClauses:
    CaseClause                          { PRINT_LINE_TODO;  $$ = NULL;}
  | CaseClauses CaseClause              { PRINT_LINE_TODO;  $$ = NULL;}
;

CaseClause:
    CASE Expr ':'                       { PRINT_LINE_TODO;  $$ = NULL;}
  | CASE Expr ':' SourceElements        { PRINT_LINE_TODO;  $$ = NULL;}
;

DefaultClause:
    DEFAULT ':'                         { PRINT_LINE_TODO;  $$ = NULL;}
  | DEFAULT ':' SourceElements          { PRINT_LINE_TODO;  $$ = NULL;}
;

LabelledStatement:
    IDENT ':' Statement                 { PRINT_LINE_TODO;  $$ = NULL;}
;

ThrowStatement:
    THROW Expr ';'                      { PRINT_LINE_TODO;  $$ = NULL;}
  | THROW Expr error                    { PRINT_LINE_TODO;  $$ = NULL;}
;

TryStatement:
    TRY Block FINALLY Block             { PRINT_LINE_TODO;  $$ = NULL;}
  | TRY Block CATCH '(' IDENT ')' Block { PRINT_LINE_TODO;  $$ = NULL;}
  | TRY Block CATCH '(' IDENT ')' Block FINALLY Block
                                        { PRINT_LINE_TODO;  $$ = NULL;}
;

DebuggerStatement:
    DEBUGGER ';'                        { PRINT_LINE_TODO;  $$ = NULL;}
  | DEBUGGER error                      { PRINT_LINE_TODO;  $$ = NULL;}
;

FunctionDeclaration:
    FUNCTION IDENT '(' ')' OPENBRACE FunctionBody CLOSEBRACE { PRINT_LINE;
											$$ = node_new ( TOK_FUNCTION, PN_FUNC);
											$$->pn_u.func.name = $2;
											$$->pn_u.func.body = $6;
											$$->pn_u.func.args = NULL;
											node_correct_position ($$, $2);
											node_correct_position ($$, $6);
											node_correct_position_end ($$, $7);
}
  | FUNCTION IDENT '(' FormalParameterList ')' OPENBRACE FunctionBody CLOSEBRACE
      { PRINT_LINE;
											$$ = node_new ( TOK_FUNCTION, PN_FUNC);
											$$->pn_u.func.name = $2;
											$$->pn_u.func.body = $7;
											$$->pn_u.func.args = $4;
											node_correct_position ($$, $2);
											node_correct_position ($$, $7);
											node_correct_position ($$, $4);
											node_correct_position_end ($$, $8);
}
;

FunctionExpr:
    FUNCTION '(' ')' OPENBRACE FunctionBody CLOSEBRACE { PRINT_LINE;
										$$ = node_new ( TOK_FUNCTION, PN_FUNC);
										$$->pn_u.func.name = NULL;
										$$->pn_u.func.body = $5;
										$$->pn_u.func.args = NULL;
										node_correct_position ($$, $5);
										node_correct_position_end ($$, $6);
}
    | FUNCTION '(' FormalParameterList ')' OPENBRACE FunctionBody CLOSEBRACE 
      { PRINT_LINE;
										$$ = node_new ( TOK_FUNCTION, PN_FUNC);
										$$->pn_u.func.name = NULL;
										$$->pn_u.func.body = $6;
										$$->pn_u.func.args = $3;
										node_correct_position ($$, $3);
										node_correct_position ($$, $6);
										node_correct_position_end ($$, $7);
}
  | FUNCTION IDENT '(' ')' OPENBRACE FunctionBody CLOSEBRACE { PRINT_LINE;
										$$ = node_new ( TOK_FUNCTION, PN_FUNC);
										$$->pn_u.func.name = $2;
										$$->pn_u.func.body = $6;
										$$->pn_u.func.args = NULL;
										node_correct_position ($$, $2);
										node_correct_position ($$, $6);
										node_correct_position_end ($$, $7);
}
  | FUNCTION IDENT '(' FormalParameterList ')' OPENBRACE FunctionBody CLOSEBRACE 
      { PRINT_LINE;
										$$ = node_new ( TOK_FUNCTION, PN_FUNC);
										$$->pn_u.func.name = $2;
										$$->pn_u.func.body = $7;
										$$->pn_u.func.args = $4;
										node_correct_position ($$, $2);
										node_correct_position ($$, $4);
										node_correct_position ($$, $7);
										node_correct_position_end ($$, $8);
}
;

FormalParameterList:
    IDENT                               { PRINT_LINE;
							$$ = node_new (TOK_LC, PN_LIST);
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
}
  | FormalParameterList ',' IDENT       { PRINT_LINE;
							$$ = $1;
							g_assert ($1->pn_arity == PN_LIST);
							if ($1->pn_u.list.head) {
								JSNode *i;
								for (i = $1->pn_u.list.head; i->pn_next != NULL; i = i->pn_next)
									;
								i->pn_next = $3;
							} else $1->pn_u.list.head = $3;
							node_correct_position ($$, $3);
}
;

FunctionBody:
    /* not in spec */                   { PRINT_LINE_TODO;  $$ = NULL;}
  | SourceElements               { PRINT_LINE; $$ = $1;}
;

Program:
    /* not in spec */                   { PRINT_LINE; global = NULL;}
    | SourceElements                    { PRINT_LINE;  global = $1;}
;

SourceElements:
    Statement                           { PRINT_LINE;
							$$ = node_new (TOK_LC, PN_LIST);
							$$->pn_u.list.head = $1;
							node_correct_position ($$, $1);
}
  | SourceElements Statement            { PRINT_LINE;
							$$ = $1;
							g_assert ($1->pn_arity == PN_LIST);
							if ($1->pn_u.list.head) {
								JSNode *i;
								for (i = $1->pn_u.list.head; i->pn_next != NULL; i = i->pn_next) {
								}
								i->pn_next = $2;
							} else $1->pn_u.list.head = $2;
							node_correct_position ($$, $2);
}
;
 
%%
#undef GLOBAL_DATA

void yyerror (char* msg)
{
//	puts (msg);
}

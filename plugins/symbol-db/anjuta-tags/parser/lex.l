%{
#include <stdio.h>
#include "jstypes.h"
#include "js-node.h"
#include "y.tab.h"
%}

%option nounput
%option noinput
%option yylineno
COMMENT_BEGIN "/*"
COMMENT_END  "*/"
%START  COMMENT
%%
{COMMENT_BEGIN} {BEGIN COMMENT;};
<COMMENT>[^*]* ;
<COMMENT>[^/] ;
<COMMENT>{COMMENT_END} {BEGIN 0; };
null	return NULLTOKEN;
true	return TRUETOKEN;
false	return FALSETOKEN;

break	return BREAK;
case	return CASE;
catch	return CATCH;
const	return CONSTTOKEN;
default return DEFAULT;
finally return FINALLY;
for return FOR;
instanceof return INSTANCEOF;
new return NEW;
var return VAR;
continue return CONTINUE;
function return FUNCTION;
return return RETURN;
void return VOIDTOKEN;
delete return DELETETOKEN;
if return IF;
this return THISTOKEN;
do return DO;
while return WHILE;
else return ELSE;
in return INTOKEN;
switch return SWITCH;
throw return THROW;
try return TRY;
typeof return TYPEOF;
with return WITH;
debugger return DEBUGGER;
"==="  return STREQ;
"!=="  return STRNEQ;
"let" ;
"++" return PLUSPLUS;
"--" return MINUSMINUS;
"+=" return PLUSEQUAL;
"-=" return MINUSEQUAL;
"*=" return MULTEQUAL;
"/=" return DIVEQUAL;
"{" return OPENBRACE;
"}" (*yylval).intValue = yylineno; return CLOSEBRACE;
"==" return EQEQ;
"!=" return NE;
"<=" return LE;
">=" return GE;
"||" return OR;
"&&" return AND;
"&=" return ANDEQUAL;
"%=" return MODEQUAL;
"^=" return XOREQUAL;
"|=" return OREQUAL;
"0x"[A-Fa-f0-9]+ return NUMBER;
[0-9]+"."[0-9]* return NUMBER;
[0-9]*"."[0-9]+ return NUMBER;
[0-9]+ return NUMBER;
[a-zA-Z_][a-zA-Z0-9_]* (*yylval).name.iname=strdup (yytext); (*yylval).name.pos.end = yylineno; (*yylval).name.pos.begin = yylineno; return IDENT_IN;
"\""([^\"]*"\\\""?[^\"]*)*"\"" return STRING;
"\'"([^\']*"\\\'"?[^\']*)*"\'" return STRING;
"<<" return LSHIFT;
">>" return RSHIFT;
">>>" return URSHIFT;
">>=" return RSHIFTEQUAL;
">>>=" return URSHIFTEQUAL;
"(" return '(';
")" return ')';
"/" return '/';
"\\" return '\\';
":" return ':';
"," return ',';
"[" return '[';
"]" return ']';
"." return '.';
"+" return '+';
"-" return '-';
"~" return '~';
"!" return '!';
"*" return '*';
"%" return '%';
"<" return '<';
">" return '>';
"&" return '&';
"^" return '^';
"|" return '|';
"?" return '?';
";" return ';';
"=" return '=';
"$" return '$';

"//".*"\n" ;
" " ;
"\t" ;
"\n" ;
%%
//"<<=" return LSHIFTEQUA;
//     RESERVED = 282,
//     IF_WITHOUT_ELSE = 288,
//     IDENT = 317,

//     AUTOPLUSPLUS = 319,
//     AUTOMINUSMINUS = 320


int yywrap ()
{
	return 1;
}


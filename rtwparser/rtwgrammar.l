%{
typedef void String;
typedef void TypeNameDef;
typedef void ObjectList;
typedef void ObjectMap;
typedef void NamedStruct;
typedef void BaseObject;
#include <stdlib.h>
#include <string.h>

#define MAX_STR_CONST 1024
static char string_buf[MAX_STR_CONST];
static char *string_buf_ptr;
static char *string_buf_end = &string_buf[MAX_STR_CONST-1];
static int structlevel = 0;
static char* newcopy(const char* src)
{
  char* r = (char*)(calloc(1, strlen(src)+1));
  strcpy(r, src);
  return r;
}

const char* YYText()
{
  return yytext;
}
#include "rtwsyntax.hxx"
%}

ws      [ \t\n]+
wsx     [ \t\n]*
wss     [ \t]*
alpha   [A-Za-z]
dig     [0-9]
name    ({alpha}|_)({alpha}|{dig}|[_<>,])*
num0         [-+]?{dig}+
num1    [-+]?{dig}+\.?([eE][-+]?{dig}+)?
num2    [-+]?{dig}*\.{dig}+([eE][-+]?{dig}+)?
number  {num1}|{num2}

%x comment param precomp precomp_misc define_a define_b a_struct inside_struct

%%
  // Lex state memory
  int comment_previous=INITIAL, struct_previous=INITIAL, precomp_previous=INITIAL;

  /** ==== PRECOMPILER STATEMENT RULES =============================================================== */
\#                          {precomp_previous = YY_START; BEGIN(precomp);}
<precomp>{
if|ifdef|ifndef|elif|endif  { BEGIN(precomp_misc); }
include|error|warning       { BEGIN(precomp_misc); }
define                      { yylval.name = strdup(yytext); BEGIN(define_a); return T_Precompiler;}
}

<precomp,precomp_misc>{
[^(\\\n)]                   /* eat precompiler statements */
[\(\){ws}]                  /* eat precompiler statements */
\\{ws}\n                    /* backslash indicates precompiler statement continues on next line */
\n                          { BEGIN(precomp_previous);}
}

  /** ==== DEFINE RULES ============================================================================== */
<define_a>{
MODEL_NAME                  { string_buf_ptr = string_buf; BEGIN(precomp_previous); return T_ModelName;}
{name}(\(({name}|{ws})+\))? { string_buf_ptr = string_buf; yylval.name = strdup(yytext);
                              BEGIN(define_b); return T_MiscDefine;}
}
<define_b>{
\\{ws}\n                    *string_buf_ptr++ = ' ';
[^(\n|"/*")]                { char *yptr = yytext;
                              while ( *yptr && string_buf_ptr < string_buf_end)
                                *string_buf_ptr++ = *yptr++; }
\n                          { *string_buf_ptr = '\0'; yylval.name = newcopy(string_buf);
                              BEGIN(precomp_previous); return T_String;}
"/*"                        { *string_buf_ptr = '\0'; yylval.name = newcopy(string_buf);
                              string_buf_ptr = string_buf; comment_previous = precomp_previous;
                              BEGIN(comment); return T_String;}
{ws}                        { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = ' '; }
[(]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = '('; }
[)]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = ')'; }
[/]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = '/'; }
["]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = '"'; }
}

  /** ==== COMMENT RULES ============================================================================= */
<*>"/*"                     {string_buf_ptr = string_buf; comment_previous = YY_START; BEGIN(comment);}
<comment>{
"*/"                        { /* saw closing quote - all done */
                              *string_buf_ptr = '\0';
                              yylval.name = newcopy(string_buf); BEGIN(comment_previous); return T_Comment;}
({number}|{name}|")")({wss}"*"{wss}({number}|{name}|"("))+ {
                              /* Mathematical expressions with * in comments should be copied */
                              char *yptr = yytext;
                              while ( *yptr && string_buf_ptr < string_buf_end)
                                *string_buf_ptr++ = *yptr++;}
{ws}"*"{ws}                 /* skip * in comment */
{ws}                        { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = ' '; }
[(]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = '('; }
[)]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = ')'; }
[/]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = '/'; }
["]                         { if (string_buf_ptr < string_buf_end) *string_buf_ptr++ = '"'; }
[^("*/")]                   { char *yptr = yytext;
                              while ( *yptr && string_buf_ptr < string_buf_end)
                                *string_buf_ptr++ = *yptr++; }
}

  /** ==== STRUCT AND VARIABLE RULES ================================================================= */
<INITIAL,inside_struct>{
typedef|const|extern        { yylval.name = strdup(yytext); return T_Keyword;}
struct                      { yylval.name = strdup(yytext);
                              if (structlevel++ == 0) struct_previous = YY_START;
                              BEGIN(a_struct); return T_Keyword;}
}

<a_struct>{name}            { yylval.name = strdup(yytext);
                              if (--structlevel == 0) BEGIN(struct_previous);
                              else BEGIN(inside_struct);
                              return T_StructName;}

<a_struct>\{                { BEGIN(inside_struct); return '{';}
<inside_struct>\}           { BEGIN(a_struct); return '}';}
\{                          return '{';
\}({ws}{name})?             return '}';

<INITIAL,inside_struct>{
\[                          return '[';
\]                          return ']';
;{wss}"/*"                  { string_buf_ptr = string_buf; comment_previous = YY_START; BEGIN(comment);}
;                           { string_buf[0] = '\0'; yylval.name = newcopy(string_buf);
                              /* Fake a trailing comment */ return T_Comment;}
;{wss}\n                    { string_buf[0] = '\0'; yylval.name = newcopy(string_buf);
                              /* Fake a trailing comment */ return T_Comment;}
{num0}                      { yylval.number = atoi(yytext); return T_Int;}
{name}                      { yylval.name = strdup(yytext); return T_String;}
}

  /** ==== UNMACHTED TEXT AND WHITESPACE RULES ======================================================= */
<*>{
.                           /* eat unmatched text */
{ws}                        /* eat whitespace */
}
%%

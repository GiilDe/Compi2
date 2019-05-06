%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokens.h"

#define TAB   0x09
#define LF    0x0A
#define CR    0x0D

int comment_lines = 1;
char * curr_str = NULL;

static inline int is_printable_char(int hex) {
  return (hex >= 0x20 && hex <= 0x7E) || hex == TAB | hex == CR || hex == LF;
}

static inline void show_comment_token() {
  printf("%d COMMENT %d\n", yylineno, comment_lines);
  // Reset the count for the next comment
  comment_lines = 1;
}

void show_token(char * name) {
  printf("%d %s %s\n", yylineno, name, yytext);
}

void append_curr_str(char * suffix) {
  if (!curr_str) {
    curr_str = (char *) malloc(sizeof(char) * (strlen(suffix) + 1));
    strcpy(curr_str, suffix);
    curr_str[strlen(suffix)] = '\0';
    return;
  }
  char * prefix = curr_str;
  curr_str = (char *) malloc(sizeof(char) * (strlen(suffix) + strlen(prefix) + 1));
  curr_str[0] = '\0';
  strcat(curr_str, prefix);
  strcat(curr_str, suffix);
  free (prefix);
}

static inline void append_escape_seq() {
  int hex = strtol(++yytext, NULL, 16);
  if (!is_printable_char(hex)) {
    return;
  }
  if (!curr_str) {
    curr_str = (char *) malloc(sizeof(char));
    curr_str[0] = hex;
    return;
  }
  char * prefix = curr_str;
  curr_str = (char *) malloc(sizeof(char) * (strlen(prefix) + 2));
  int len = strlen(prefix);
  strcpy(curr_str, prefix);
  curr_str[len] = (char) hex;
  curr_str[len + 1] = '\0';
}

static int show_string() {
  return STRING;
}

static void error(char * c_name) {
  printf("Error %s\n", c_name);
  exit(0);
}

static inline void nested_comment() {
  printf("Warning nested comment\n");
  exit(0);
}

static inline void illegal_escape_sequence(){
    printf("Error undefined escape sequence %s\n", ++yytext);
    exit(0);
}

%}

%option yylineno
%option noyywrap

%x COMMENT
%x STRING_ONE
%x STRING_TWO

ws                    ([\r\n\t ])
digit                 ([0-9])
letter                ([a-zA-Z])
hex_digit             ([0-9a-fA-F])
hex_num               ([\+\-]?0x{hex_digit}+)
printable_char        ([\x20-\x7E\x09\x0A\x0D])
identifier_char       ({digit}|{letter}|[\-_])
ascii_escape_seq      (\\({hex_digit}){1,6})
num                   ({digit}+)
s_num                 ([\+\-]?{num})
rgb_num               ({ws}*{s_num}{ws}*)
printable_in_comment  ([\x20-\x29\x2B-\x2E\x30-\x7E\t\r])
printable_str_c       ([\x20-\x21\x23-\x5B\x5D-\x7E\x09])
printable_str_c_f     ([\x20-\x26\x28-\x5B\x5D-\x7E\x09])
esc_seq_no_lf         ((\\r)|(\\t)|(\\\\)|{ascii_escape_seq})
esc_seq               ((\\n)|{esc_seq_no_lf})

%%
\/\*                                          BEGIN(COMMENT);
<COMMENT>\/\*                                 nested_comment();
<COMMENT>{printable_in_comment}*              ;
<COMMENT>\n                                   comment_lines++;
<COMMENT>\*\/                                 BEGIN(INITIAL); show_comment_token();
<COMMENT>\*                                   ;
<COMMENT>\/                                   ;
<COMMENT>.                                    error("/");
<COMMENT><<EOF>>                              error("unclosed comment");


\"                                            BEGIN(STRING_ONE);
<STRING_ONE>\"                                BEGIN(INITIAL); return STRING;
<STRING_ONE>({printable_str_c}|{esc_seq})*    append_curr_str(yytext);



\'                                            BEGIN(STRING_TWO);
<STRING_TWO>\'                                BEGIN(INITIAL); return STRING;
<STRING_TWO>\\n                               append_curr_str("\n");
<STRING_TWO>\\t                               append_curr_str("\t");
<STRING_TWO>\\r                               append_curr_str("\r");
<STRING_TWO>\\\\                              append_curr_str("\\");
<STRING_TWO>{ascii_escape_seq}                append_escape_seq();
<STRING_ONE,STRING_TWO>\\{printable_str_c}    illegal_escape_sequence();
<STRING_TWO>{printable_str_c_f}*              append_curr_str(yytext);
<STRING_ONE,STRING_TWO>{ws}*\n                error("unclosed string");
<STRING_ONE,STRING_TWO><<EOF>>                error("unclosed string");
<STRING_TWO>.                                 error("'");
<STRING_ONE>.                                 error("\"");


#((-?{letter})|{num}){identifier_char}*       return HASHID;
@import                                       return IMPORT;
!{ws}*[iI][mM][pP][oO][rR][tT][aA][nN][tT]    return IMPORTANT;
[>\+~]                                        return COMB;
:                                             return COLON;
;                                             return SEMICOLON;
\{                                            return LBRACE;
\}                                            return RBRACE;
\[                                            return LBRACKET;
\]                                            return RBRACKET;
=                                             return EQUAL;
\*                                            return ASTERISK;
\.                                            return DOT;
({s_num}|{hex_num})                           return NUMBER;
(({digit}+)|({digit}*\.{digit}+))([a-z]+|%)   return UNIT;
rgb\({rgb_num},{rgb_num},{rgb_num}\)          show_token("RGB");
rgb                                           error("in rgb parameters");
(\-)?[a-zA-Z]{identifier_char}*               return NAME;
{ws}                                          ;
<<EOF>> {return EF;}

.                                             error(yytext);

%%

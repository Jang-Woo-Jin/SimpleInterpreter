%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>
#include "myGrammar.h"
#include "myGrammar.tab.h"
%}

%%
"mainprog"			{ return MAINPROG; }
"var"				{ return VAR; }
"array"				{ return ARRAY; }
"of"				{ return OF; }
"function"			{ return FUNCTION; }
"procedure"			{ return PROCEDURE; }
"begin"				{ return BEG; }
"end"				{ return END; }
"if"				{ return IF; }
"then"				{ return THEN; }
"else"				{ return ELSE; }
"nop"				{ return NOP; }
"while"				{ return WHILE; }
"return"			{ return RETURN; }
"print"				{ return PRINT; }
"int"				{ return INTEGER; }
"float"				{ return FLOAT; }

[a-zA-Z][a-zA-Z0-9]*	{
				   			yylval.c_value = *yytext - 'a';
							return ID;		// ID
						}
[0-9]+				{ 
						yylval.i_value = atoi(yytext);
						return I_VALUE;		// Integer
					}	
[0-9]+"."[0-9]+		{	
						float temp;
						sscanf(yytext,"%f",&temp);
						yylval.f_value = temp;
						return F_VALUE;		// Float
					}

[-+*/<>!~]			{ 
						yylval.c_value = *yytext;
						return *yytext; 
					}

">="        		{ return GE; }
"<="        		{ return LE; }
"=="        		{ return EQ; }
"!="        		{ return NE; }

[;.,=():]|\[|\]		{ return *yytext; }
					
[\n]*|\t|[ ][\t ]*			;// WhiteSpace
"/*"([^*]|\*+[^*/])*\*+"/"|"//".*|"#".*		;// WhiteSpace - Comment
%%

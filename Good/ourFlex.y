%{
#include<stdio.h>
#include<string.h>
#include "ourFlex.tab.h"

void yyerror(const char *str){
	fprintf(stderr,"err : %s\n",str);
}

int yywrap(){
	return 1;
}
main(){
	yyparse();
}
%}




%token ReservedWord
%token ID
%token Int
%token Float
%token Operator
%token Delimiter


%left '+' '-'
%left '*' '/'

%%

Identifies: /*empty*/
	| Identifies Identify
	;

Identify:
	ReservedWord
	{
		if(!strcmp($1, "then")){
			printf("then");
		}
		else if($0 == "then"){
			printf("then");
		}
		printf("\tFlex returned ReservedWord : %s\n",$1);
	}
	|
	ID
	{
		printf("\tFlex returned ID : %s\n",$1);
	}
	|
	Int
	{
		printf("%d",yyval*yyval);
		printf("\tFlex returned Int : %d\n",yylval);
	}
	|
	Float
	{
		printf("\tFlex returned Float : %s\n",$1);
	}
	|
	Operator
	{
		printf("\tFlex returned Operator : %s\n",$1);
	}
	|
	Delimiter
	{
		printf("\tFlex returned Delimiter : %s\n",$1);
	}
	;
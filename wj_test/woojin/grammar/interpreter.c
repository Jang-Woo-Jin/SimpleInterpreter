# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <string.h>
# include <math.h>
# include "test.h"
/*
#define ADDOTHERS 5000
#define ADDFLOAT 2000
#define ADDINTEGER 1000
*/
/* symbol table */
/* hash a symbol */
static unsigned symhash(char *sym) {
	unsigned int hash = 0;
	unsigned c;
	while(c = *sym++) hash = hash*9 ^ c;
	return hash;
}

struct symbol *lookup(char* sym) {
	struct symbol *sp = &symtab[symhash(sym)%NHASH];
	int scount = NHASH; /* how many have we looked at */
	while(--scount >= 0) {
		if(sp->name && !strcmp(sp->name, sym)) { 
			return sp; 
		}
		if(!sp->name) { /* new entry */
			sp->name = strdup(sym);
			sp->value = 0;
			sp->func = NULL;
			sp->syms = NULL;
			return sp;
		}	
		if(++sp >= symtab+NHASH){
			sp = symtab; /* try the next entry */
		} 
	}
	yyerror("symbol table overflow\n");
	abort(); /* tried them all, table is full */
}

struct ast *newast(int nodetype, struct ast *l, struct ast *r) {
	struct ast *a = malloc(sizeof(struct ast));

	if(!a) {
		yyerror("out of space in newast");
		exit(0);
	}

	a->nodetype = nodetype;
	a->l = l;
	a->r = r;
	
	return a;
}

struct ast *newnum(float d, int valuetype) {
	struct numval *a = malloc(sizeof(struct numval));

	if(!a) {
		yyerror("out of space in newnum");
		exit(0);
	}

	if(valuetype == 'I') a->nodetype = 'K' + 1000;
	else if(valuetype == 'F') a->nodetype = 'K' + 2000;
	else {
		yyerror("Unvalid number type");
		exit(0);
	}

	a->number = d;
	return (struct ast *)a;
}

struct ast *newcmp(int cmptype, struct ast *l, struct ast *r) {
	struct ast *a = malloc(sizeof(struct ast));

	if(!a) {
		yyerror("out of space in newcmp");
		exit(0);
	}
	
	a->nodetype = '0' + cmptype;
	a->l = l;
	a->r = r;
	return a;
}

struct ast *newfunc(int functype, struct ast *l) {
	struct fncall *a = malloc(sizeof(struct fncall));

	if(!a) {
		yyerror("out of space in new func");
		exit(0);
	}

	a->nodetype = 'F';
	a->l = l;
	a->functype = functype;
	return (struct ast *)a;
}

struct ast *newcall(struct symbol *s, struct ast *l) {
	struct ufncall *a = malloc(sizeof(struct ufncall));

	if(!a) {
		yyerror("out of space in newcall");
		exit(0);
	}
	
	a->nodetype = 'C';
	a->l = l;
	a->s = s;
	return (struct ast *)a;
}

struct ast *newref(struct symbol *s) {
	struct symref *a = malloc(sizeof(struct symref));

	if(!a) {
		yyerror("out of space in newref");
		exit(0);
	}

	a->nodetype = 'N';
	a->s = s;
	return (struct ast *)a;
}

struct ast *newasgn(struct symref *l, struct ast *v) {
	struct symasgn *a = malloc(sizeof(struct symasgn));
	
	if(!a) {
		yyerror("out of space in newasgn");
		exit(0);
	}
	
	int valuetype = ((struct numval*)v)->nodetype;
	if(valuetype > 2000) valuetype = 'F';
	else if(valuetype > 1000) valuetype = 'I';
	
	// printf("asign l->s : %d\n",l->s);
	// printf("asign l->s type : %d\n",(struct type*)(l->s->type));
	// printf("asign l->s valuetype : %c\n",(struct type*)(l->s->type)->valuetype);
	// printf("asigned value type : %c\n",valuetype);
	if(((struct typedivide *)(l->s->type))->valuetype != valuetype){
		printf("type miss match!\n");
	}
	else {
		printf("type match!\n");
	}
	a->nodetype = '=';
	a->s = l->s;
	a->v = v;
	return (struct ast *)a;
}

struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el) {
	struct flow *a = malloc(sizeof(struct flow));

	if(!a) {
		yyerror("out of space in newflow");
		exit(0);
	}

	a->nodetype = nodetype;
	a->cond = cond;
	a->tl = tl;
	a->el = el;
	return (struct ast *)a;
}

/* free a tree of ASTs */
void treefree(struct ast *a) {
	int separater = a->nodetype;
	if(separater > 2000) separater -= 2000;
	else if(separater > 1000) separater -= 1000;
	
	switch(separater) {
	/* two subtrees */
		case '+':
		case '-':
		case '*':
		case '/':
		case '1': case '2': case '3': case '4': case '5': case '6':
		case 'L':
			treefree(a->r);
		/* one subtree */
		case '|':
		case 'M': case 'C': case 'F': case 'U': 
			treefree(a->l);
		/* no subtree */
		case 'K': case 'N': case 'X': case 'T': case 'E': case 'Z':
			break;
		
		case '=':
			free( ((struct symasgn *)a)->v);
			break;
		/* up to three subtrees */
		case 'I': case 'W':
			free( ((struct flow *)a)->cond);
			if( ((struct flow *)a)->tl){
				treefree( ((struct flow *)a)->tl);
			}
			if( ((struct flow *)a)->el) {
				treefree( ((struct flow *)a)->el);
			}
			break;
		default: 
			printf("internal error: free bad node %c\n", a->nodetype);
	}	
	free(a); /* always free the node itself */
}

struct symlist *newsymlist(struct symbol *sym, struct symlist *next) {
	struct symlist *sl = malloc(sizeof(struct symlist));

	if(!sl) {
		yyerror("out of space");
		exit(0);
	}
	sl->sym = sym;
	sl->next = next;
	return sl;
}
/* free a list of symbols */
void symlistfree(struct symlist *sl) {
	struct symlist *nsl;
	
	while(sl) {
		nsl = sl->next;
		free(sl);
		sl = nsl;
	}
}

static float callbuiltin(struct fncall *f) {
	enum bifs functype = f->functype;
	float v = eval(f->l);
	
	int valuetype = f->l->valuetype;
	
	switch(functype) {
		case B_print:
			// if(valuetype == 'F') printf("float = %4.4f\n",v);
			// else if(valuetype == 'I') printf("int = %d\n",v);
			// else printf("Error unknown num type");
			printf("print = %4.4g\n",v);
			return v;
		default:
			yyerror("Unknown built-in function %d", functype);
			return 0.0;
	}
}

float eval(struct ast *a) {
	float v;
	if(!a) {
		yyerror("internal error, null eval");
		return 0.0;
	}
	
	int separater = a->nodetype;
	if(separater > 2000) separater -= 2000;
	else if(separater > 1000) separater -= 1000;
		
	switch(separater) {
		/* constant */
		case 'K': 
			v = ((struct numval *)a)->number; 
			break;
		/* name reference */
		case 'N': 
			v = ((struct symref *)a)->s->value; 
			break;
		/* assignment */
		case '=':
			v = ((struct symasgn *)a)->s->value = eval(((struct symasgn *)a)->v);
			break;
		/* no operation*/
		case 'X': 
			printf("nop detected!\n");
			v = 0.0;
			//do nothing
			break;
		case 'E': 
			printf("Epsilon detected!\n");
			v = 0.0;
			//do nothing
			break;	
		case 'T':
			printf("type divided!\n");
			break;
		/* expressions */
		case '+': 
			v = eval(a->l) + eval(a->r);
			break;
		case '-':
			v = eval(a->l) - eval(a->r); 
			break;
		case '*': 
			v = eval(a->l) * eval(a->r); 
			break;
		case '/': 
			v = eval(a->l) / eval(a->r);
			break;
		case '|': 
			v = fabs(eval(a->l)); 
			break;
		case 'M': 
			v = -eval(a->l); 
			break;
		case '!':
			v = !eval(a->l);
			break;
		/* comparisons */
		case '1': 
			v = (eval(a->l) > eval(a->r))? 1 : 0; 
			break;
		case '2': 
			v = (eval(a->l) < eval(a->r))? 1 : 0; 
			break;
		case '3': 
			v = (eval(a->l) != eval(a->r))? 1 : 0; 
			break;
		case '4': 
			v = (eval(a->l) == eval(a->r))? 1 : 0; 
			break;
		case '5': 
			v = (eval(a->l) >= eval(a->r))? 1 : 0; 
			break;
		case '6': 
			v = (eval(a->l) <= eval(a->r))? 1 : 0;
			break;
		/* control flow */
		/* null expressions allowed in the grammar, so check for them */
		/* if/then/else */
		case 'I':
			if( eval( ((struct flow *)a)->cond) != 0) { 
				if( ((struct flow *)a)->tl) { 
					v = eval( ((struct flow *)a)->tl);
				}
				else {
					v = 0.0; /* a default value */
				}
			} 
			else {
				if( ((struct flow *)a)->el) { 
					v = eval(((struct flow *)a)->el);
				} 
				else {
					v = 0.0; /* a default value */
				}
			}
			break;
		
		/* while/do */
		case 'W':
			v = 0.0; /* a default value */
			if( ((struct flow *)a)->tl) {
				while( eval(((struct flow *)a)->cond) != 0) {
					v = eval(((struct flow *)a)->tl); 
				} 
			}
			break; /* value of last statement is value of while/do */
		/* list of statements */
		case 'L':
			eval(a->l);
			v = eval(a->r); 
			break;
		case 'U': case 'Z':
			v = eval(a->l);
			break;
		case 'F':
			v = callbuiltin((struct fncall *)a); 
			break;
		case 'C':
			v = calluser((struct ufncall *)a); 
			break;
		default:
			printf("point value : %d\n",a);
			printf("point seperator : %d\n",separater);
			printf("internal error: bad node %c\n", a->nodetype);
	}
	return v;
}

void dodef(struct symbol *name, struct symlist *syms, struct ast *func) {
	if(name->syms) { 
		symlistfree(name->syms);
	}
	if(name->func) {
		treefree(name->func);
	}
	name->syms = syms;
	name->func = func;
}

static float calluser(struct ufncall *f) {
	struct symbol *fn = f->s; /* function name */
	struct symlist *sl; /* dummy arguments */
	struct ast *args = f->l; /* actual arguments */
	float *oldval, *newval; /* saved arg values */
	float v;
	int nargs;
	int i;
	
	if(!fn->func) {
		yyerror("call to undefined function", fn->name);
		return 0;
	}
	
	/* count the arguments */
	sl = fn->syms;
	for(nargs = 0; sl; sl = sl->next) {
		nargs++;
	}
	
	/* prepare to save them */
	oldval = (float *)malloc(nargs * sizeof(float));
	newval = (float *)malloc(nargs * sizeof(float));
	if(!oldval || !newval) {
		yyerror("Out of space in %s", fn->name); return 0.0;
	}

	/* evaluate the arguments */
	for(i = 0; i < nargs; i++) {
		if(!args) {
			yyerror("too few args in call to %s", fn->name);
			free(oldval); free(newval);
			return 0.0;
		}
		if(args->nodetype == 'L') { /* if this is a list node */
			newval[i] = eval(args->l);
			args = args->r;
		} 
		else { /* if it's the end of the list */
			newval[i] = eval(args);
			args = NULL;
		}
	}

	/* save old values of dummies, assign new ones */
	sl = fn->syms;
	for(i = 0; i < nargs; i++) {
		struct symbol *s = sl->sym;
		oldval[i] = s->value;
		s->value = newval[i];
		sl = sl->next;
	}
	free(newval);
	
	/* evaluate the function */
	v = eval(fn->func);
	
	/* put the real values of the dummies back */
	sl = fn->syms;
	for(i = 0; i < nargs; i++) {
		struct symbol *s = sl->sym;
		s->value = oldval[i];
		sl = sl->next;
	}
	free(oldval);
	return v;
}

///////////////////////
struct ast *typedivide(int isarray, float number, int type){
	struct typedivide *a = malloc(sizeof(struct typedivide));
	
	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = 'T';
	a->isarray = isarray;
	a->number = number;
	a->valuetype = type;

	return (struct ast*)a;
}

struct ast *newEpsilon(){
	struct ast *a = malloc(sizeof(struct ast));

	a->nodetype = 'E';

	return a;
}

struct ast *newidentifier(struct fixsymlist *idls, struct ast *type, struct ast *r){
	struct ast *a = malloc(sizeof(struct ast));
	struct fixsymlist *temp = idls;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	temp->symref->s->type = type;
	
	while(temp->next){
		temp = temp->next;
		temp->symref->s->type = type;
	}
	if(r->nodetype == 'E') a->nodetype = 'U';
	else a->nodetype = 'L';

	a->l = (struct ast*)idls;
	a->r = r;
	// printf("first sym : %d\n",temp->symref->s);
	// printf("type : %d\n",type);
	// printf("type->type : %c\n",type->valuetype);
	// printf("right : %d\n",a->r);
	// printf("left : %d\n",a->l);
	return a;

}

struct fixsymlist *newfixsymlist(struct symref *sym, struct fixsymlist *next) {
	struct fixsymlist *fsl = malloc(sizeof(struct fixsymlist));

	if(!fsl) {
		yyerror("out of space");
		exit(0);
	}
	fsl->nodetype = 'Z';
	fsl->symref = sym;
	fsl->next = next;
	// printf("make fixsymlist symref: %d\n",sym);
	// printf("make fixsymlist sym: %d\n",sym->s);
	return fsl;
}

/*
int makeNodeType(int addednodetype){
	int makednodetype;
	if(addednodetype > 5000) makednodetype = addednodetype - 5000;
	else if(addednodetype > 2000) makednodetype = addednodetype - 2000;
	else if(addednodetype > 1000) makednodetype = addednodetype - 1000;
	else makednodetype = 0;
	return makednodetype;
}
int findNumValueType(int nodetype){
	int valuetype = nodetype;
	if(valuetype > 2000) valuetype = 'F';
	else if(valuetype > 1000) valuetype = 'I';
	return valuetype;
}
*/
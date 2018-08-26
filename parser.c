#include <stdio.h>
#include <stdlib.h>
//angle brackets searches "system locations" first, quotes searches current directory first
#include "mpc.h"

//macros
#define LASSERT(args, cond, err) \
	if(!(cond)) {lval_del(args); return lval_err(err);}

//if we are running on windows. I believe this works for x64 too
#ifdef _WIN32
#include <string.h>
//buffer for user input of size 2048 (bytes, since each char is a byte)
//note: a static global variable means lexical scope is confined to this "compilation unit" (can probs be understood as file).
//static inside of a function means that it keeps its value between invocations
static char buffer[2048];

char* readline(char* prompt)
{
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	//malloc one extra byte since a string ends with \0
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	//terminate string
	cpy[strlen(cpy)-1]='\0';
	return cpy;
}
//windows doesn't need the add_history fn
void add_history(char* unused){}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

//data structures
//lisp value - return type of many funcs
typedef struct lval
{
	int type;
	double num;
	//error is now a string
	char* err;
	char* sym;
	int count;
	struct lval** cell;
} lval;
//possible lval types
enum{LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};

//prototypes
lval eval(mpc_ast_t* t);
lval eval_op(lval x, char* op, lval y);
lval* lval_num(double x);
lval* lval_err(char* m);
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_sexpr(void);
lval* lval_sym(char* s);
void lval_del(lval* v);
lval* lval_add(lval* v, lval* x);
lval* lval_read(mpc_ast_t* t);
void lval_expr_print(lval* v, char open, char close);
lval* lval_eval_sexpr(lval* v);
lval* lval_pop(lval* v, int i);
lval* builtin_op(lval* a, char* op);
lval* lval_take(lval* v, int i);
lval* lval_eval(lval* v);
lval* lval_qexpr(void);
lval* builtin_head(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
lval* lval_join(lval* x, lval* y);
lval* builtin(lval* a, char* func);
lval* builtin_cons(lval* a);
lval* builtin_len(lval* a);
lval* builtin_init(lval* a);

//argc is argument count, argv is argument vector
//char** is a list of strings (approximately)
int main(int argc, char** argv)
{

	//define polish notation parsers
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Qexpr = mpc_new("qexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT, 
	"\
	 number : /-?[0-9]+(\\.[0-9]+)?/ ; \
	 symbol : \"init\" | \"cons\" | \"len\" | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ; \
	 sexpr: '(' <expr>* ')' ;\
	 qexpr: '{' <expr>* '}' ;\
	 expr : <number> | <symbol> | <sexpr> | <qexpr> ; \
	 lispy : /^/ <expr>* /$/; \
	",
	Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

	
	puts("Lispy Version 0.0.0.0.1");
	//note extra new line
	puts("Press Ctrl+C to exit\n");
	while(1)
	{
		char* input = readline("lispy> ");
		add_history(input);
		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lispy, &r))
		{
			//mpc_ast_print(r.output);
			//lval result=eval(r.output);
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
			mpc_ast_delete(r.output);	
		}
		else
		{
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	return 0;
}

lval* lval_eval_sexpr(lval* v)
{
	//eval children
	for(int i=0;i<v->count;i++)
		v->cell[i]=lval_eval(v->cell[i]);
	//return first error found if there is one
	for(int i=0;i<v->count;i++)
		if(v->cell[i]->type==LVAL_ERR)
			return lval_take(v, i);
	//empty
	if(v->count==0)
		return v;
	//single contained in parens
	if(v->count==1)
		return lval_take(v,0);
	//make sure first elem is symbol
	lval* f=lval_pop(v,0);
	if(f->type!=LVAL_SYM)
	{
		lval_del(f);lval_del(v);
		return lval_err("S expression must start with a symbol");
	}
	//builtin op evals based on operator
	lval* result=builtin(v, f->sym);
	lval_del(f);
	return result;
}

lval* lval_eval(lval* v)
{
	if(v->type==LVAL_SEXPR)
		return lval_eval_sexpr(v);
	//otherwise we just return it verbatim
	return v;
}

lval* lval_pop(lval* v, int i)
{
	//get ith elem
	lval* x=v->cell[i];
	//shift rest of mem over. "copies 3rd chars from 2nd to 1st"
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
	v->count--;
	//realloc mem
	v->cell=realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i)
{
	lval* x=lval_pop(v,i);
	lval_del(v);
	return x;
}


lval* builtin_op(lval* a, char* op)
{
	//make sure they're all nums
	for(int i=0;i<a->count;i++)
		if(a->cell[i]->type!=LVAL_NUM)
		{
			lval_del(a);
			return lval_err("All args must be numbers");
		}
	//get first elem
	lval* x=lval_pop(a,0);
	//if no args and sub then perform unary negation
	if((strcmp(op, "-")==0)&&a->count==0)
		x->num=-x->num;
	//while still elems
	while(a->count>0)
	{
		lval* y=lval_pop(a,0);
		if(strcmp(op, "max")==0)
			x->num=x->num>y->num?x->num:y->num;
		if(strcmp(op, "min")==0)
			x->num=x->num<y->num?x->num:y->num;
		if(strcmp(op, "+")==0) x->num+=y->num;
		if(strcmp(op, "-")==0) x->num-=y->num;
		if(strcmp(op, "*")==0) x->num*=y->num;
		if(strcmp(op, "/")==0)
		{
			if(y->num==0)
			{
				lval_del(x);lval_del(y);
				x=lval_err("Division by zero");break;
			}
			x->num/=y->num;
		}
		if(strcmp(op, "%")==0)
		{
			if(y->num==0)
			{
				lval_del(x);lval_del(y);
				x=lval_err("Division by zero");break;
			}
			x->num=x->num-y->num*(int)(x->num/y->num);
		}
		lval_del(y);
	}
	lval_del(a);
	return x;
}

lval* lval_num(double x)
{
	lval* v=malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_err(char* m)
{
	lval* v=malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m)+1);
	strcpy(v->err, m);
	return v;
}

lval* lval_sexpr(void)
{
	lval* v=malloc(sizeof(lval));
	v->type=LVAL_SEXPR;
	v->count=0;
	v->cell=NULL;
	return v;
}

lval* lval_read_num(mpc_ast_t* t)
{
	errno=0;
	double x=strtod(t->contents, NULL);
	return errno!=ERANGE?lval_num(x):lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t)
{
	if(strstr(t->tag, "number")) return lval_read_num(t);
	if(strstr(t->tag, "symbol")) return lval_sym(t->contents);
	//if root (>) or sexpr
	lval* x=NULL;
	if(strcmp(t->tag, ">")==0 || strstr(t->tag, "sexpr")) x=lval_sexpr();
	if(strstr(t->tag, "qexpr")) x=lval_qexpr();
	//add all the important things
	for(int i=0; i<t->children_num; i++)
	{
		if(strcmp(t->children[i]->contents, "(")==0 ||
		   strcmp(t->children[i]->contents, ")")==0 ||
		   strcmp(t->children[i]->contents, "{")==0 ||
		   strcmp(t->children[i]->contents, "}")==0 ||
		   strcmp(t->children[i]->tag, "regex")==0)
			continue;
		x=lval_add(x, lval_read(t->children[i]));
	}
	return x;
}

lval* lval_add(lval* v, lval* x)
{
	v->count++;
	//second star is times, not pointer
	v->cell=realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1]=x;
	return v;
}

void lval_del(lval* v)
{
	switch(v->type)
	{
		case LVAL_NUM: break;
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			for(int i=0; i<v->count; i++)
				lval_del(v->cell[i]);
			free(v->cell);
			break;
	}
	free(v);
}

lval* lval_sym(char* s)
{
	lval* v=malloc(sizeof(lval));
	v->type=LVAL_SYM;
	v->sym=malloc(sizeof(s)+1);
	strcpy(v->sym, s);
	return v;
}

void lval_print(lval* v)
{
	switch(v->type)
	{
		case LVAL_NUM: printf("%f", v->num); break;
		case LVAL_ERR: printf("Error: %s", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
	}
}

void lval_expr_print(lval* v, char open, char close)
{
	putchar(open);
	for(int i=0; i<v->count; i++)
	{
		lval_print(v->cell[i]);
		if(i!=(v->count-1))
			putchar(' ');
	}
	putchar(close);
}

void lval_println(lval* v) {lval_print(v); putchar('\n');}

lval* lval_qexpr(void)
{
	lval* v=malloc(sizeof(lval));
	v->type=LVAL_QEXPR;
	v->count=0;
	v->cell=NULL;
	return v;
}

lval* builtin_head(lval* a)
{
	LASSERT(a, a->count==1,
		"Function 'head' passed too many arguments");
	LASSERT(a, a->cell[0]->type==LVAL_QEXPR,
		"Function 'head' passed incorrect type");
	LASSERT(a, a->cell[0]->count!=0,
		"Function 'head' passed {}");
	
	lval* v=lval_take(a,0);
	while(v->count>1)
		lval_del(lval_pop(v, 1));
	return v;
}

lval* builtin_tail(lval* a)
{
	LASSERT(a, a->count==1,
		"Function 'tail' passed too many arguments");
	LASSERT(a, a->cell[0]->type==LVAL_QEXPR,
		"Function 'tail' passed incorrect type");
	LASSERT(a, a->cell[0]->count!=0,
		"Function 'tail' passed {}");

	lval* v=lval_take(a,0);
	lval_del(lval_pop(v,0));
	return v;
}

lval* builtin_list(lval* a)
{
	a->type=LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lval* a)
{
	LASSERT(a, a->count==1,
		"Function 'eval' passed too many arguments");
	LASSERT(a, a->cell[0]->type==LVAL_QEXPR,
		"Function 'eval' passed incorrect type");

	lval* x=lval_take(a,0);
	x->type=LVAL_SEXPR;
	return lval_eval(x);
}

lval* builtin_join(lval* a)
{
	for(int i=0;i<a->count;i++)
		LASSERT(a, a->cell[i]->type==LVAL_QEXPR,
			"Function 'join' passed incorrect type");
	lval* x=lval_pop(a,0);
	while(a->count)
		x=lval_join(x, lval_pop(a,0));
	lval_del(a);
	return x;
}

lval* lval_join(lval* x, lval* y)
{
	while(y->count)
		x=lval_add(x, lval_pop(y,0));
	lval_del(y);
	return x;
}

lval* builtin(lval* a, char* func)
{
	if(strcmp("list", func)==0) return builtin_list(a);
	if(strcmp("head", func)==0) return builtin_head(a);
	if(strcmp("tail", func)==0) return builtin_tail(a);
	if(strcmp("join", func)==0) return builtin_join(a);
	if(strcmp("eval", func)==0) return builtin_eval(a);
	if(strcmp("cons", func)==0) return builtin_cons(a);
	if(strcmp("len",  func)==0) return builtin_len(a);
	if(strcmp("init", func)==0) return builtin_init(a);
	if(strstr("+-/*^minmax%", func)) return builtin_op(a, func);
	lval_del(a);
	return lval_err("Unknown function");
}

//!!!
//i feel like i'm leaking one byte of mem every time i call this...valgrind later
lval* builtin_cons(lval* a)
{
	LASSERT(a, a->cell[1]->type==LVAL_QEXPR,
		"Function 'cons' passed incorrect type");
	a->cell[1]->count++;
	a->cell[1]->cell=realloc(a->cell[1]->cell, sizeof(lval*) * a->cell[1]->count+1);
	memmove(&a->cell[1]->cell[1], &a->cell[1]->cell[0], sizeof(lval*) * a->cell[1]->count-1);
	a->cell[1]->cell[0]=a->cell[0];
	return a->cell[1];
}

lval* builtin_len(lval* a)
{
	LASSERT(a, a->cell[1]->type==LVAL_QEXPR,
		"Function 'len' should be passed Q expression");
	return lval_num(a->cell[0]->count);
}

lval* builtin_init(lval* a)
{
	LASSERT(a, a->cell[1]->type==LVAL_QEXPR,
		"Function 'init' should be passed Q expression");
	LASSERT(a, a->count==1, 
		"Function 'init' passed incorrect number of arguments");
	free(a->cell[1]->cell[a->cell[1]->count-1]);
	a->cell[1]->count--;
	a->cell[1]->cell=realloc(a->cell[1]->cell, sizeof(lval*) * a->cell[1]->count);
	return a->cell[1];
}

	




















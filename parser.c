#include <stdio.h>
#include <stdlib.h>
//angle brackets searches "system locations" first, quotes searches current directory first
#include "mpc.h"
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
typedef struct 
{
	int type;
	long num;
	int err;
} lval;
//possible lval types
enum{LVAL_NUM, LVAL_ERR};
//possible error types
enum{LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};

//prototypes
lval eval(mpc_ast_t* t);
lval eval_op(lval x, char* op, lval y);
lval lval_num(long x);
lval lval_err(int x);
void lval_print(lval v);
void lval_println(lval v);

//argc is argument count, argv is argument vector
//char** is a list of strings (approximately)
int main(int argc, char** argv)
{

	//define polish notation parsers
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT, 
	"\
	 number : /-?[0-9]+(\\.[0-9]+)?/ ; \
	 operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ; \
	 expr : <number> | '(' <operator> <expr>+ ')'; \
	 lispy : /^/ <operator> <expr>+ /$/; \
	",
	Number, Operator, Expr, Lispy);

	
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
			lval result=eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);	
		}
		else
		{
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;
}

lval eval(mpc_ast_t* t)
{
	//if tag has number in it return it directly (base case)
	if(strstr(t->tag, "number"))
	{
		errno=0;
		long x=strtol(t->contents, NULL, 10);
		return errno != ERANGE?lval_num(x):lval_err(LERR_BAD_NUM);
	}
	//first child is (
	char* op=t->children[1]->contents;
	lval x=eval(t->children[2]);
	//if there is only one argument to the operator
	if(t->children_num == 4)
	{
		if(strcmp(op, "-")==0)
			return lval_num(-1*x.num);
		else
			return lval_err(LERR_BAD_OP);
	}
	for(int i=3; i<t->children_num && strstr(t->children[i]->tag, "expr"); i++)
		x = eval_op(x, op, eval(t->children[i]));
	return x;
}

lval eval_op(lval x, char* op, lval y)
{
	if(x.type == LVAL_ERR) return x;
	if(y.type == LVAL_ERR) return y;
	if(strcmp(op, "*")==0)
		return lval_num(x.num*y.num);
	if(strcmp(op, "+")==0)
		return lval_num(x.num+y.num);
	if(strcmp(op, "-")==0)
		return lval_num(x.num-y.num);
	if(strcmp(op, "/")==0)
		return y.num==0?lval_err(LERR_DIV_ZERO):lval_num(x.num/y.num);
	if(strcmp(op, "%")==0)
		return y.num==0?lval_err(LERR_DIV_ZERO):lval_num(x.num-y.num*(x.num/y.num));
	if(strcmp(op, "^")==0)
	{
		long r=1;
		for(int i=0; i<y.num; i++)
			r*=x.num;
		return lval_num(r);
	}
	if(strcmp(op, "min")==0)
		return x.num<y.num?x:y;
	if(strcmp(op, "max")==0)
		return x.num>y.num?x:y;
	return lval_err(LERR_BAD_OP);
}

lval lval_num(long x)
{
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

lval lval_err(int x)
{
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

void lval_print(lval v)
{
	switch(v.type)
	{
		case LVAL_NUM: printf("%li", v.num); break;
		case LVAL_ERR:
			switch(v.err)
			{
				case LERR_DIV_ZERO: printf("Error: Division by zero"); break;
				case LERR_BAD_OP  : printf("Error: Invalid operator"); break;
				case LERR_BAD_NUM : printf("Error: Invalid number"); break;
			}break;
	}
}

void lval_println(lval v) {lval_print(v); putchar('\n');}



























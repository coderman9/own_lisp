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

long eval(mpc_ast_t* t);
long eval_op(long x, char* op, long y);

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
	 operator : '+' | '-' | '*' | '/' ; \
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
			long result=eval(r.output);
			printf("%li\n", result);
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

long eval(mpc_ast_t* t)
{
	//if tag has number in it return it directly (base case)
	if(strstr(t->tag, "number"))
		return atoi(t->contents);
	//first child is (
	char* op=t->children[1]->contents;
	long x=eval(t->children[2]);
	for(int i=3; i<t->children_num && strstr(t->children[i]->tag, "expr"); i++)
		x = eval_op(x, op, eval(t->children[i]));
	return x;
}

long eval_op(long x, char* op, long y)
{
	/*if(strcmp(op, "*")==0)
		return x*y;
	if(strcmp(op, "+")==0)
		return x+y;
	if(strcmp(op, "-")==0)
		return x-y;
	if(strcmp(op, "/")==0)
		return x/y;*/
	//not sure if this way is cleaner or not, but i kinda like it
	switch(op[0])
	{
		case 42: return x*y;
		case 43: return x+y;
		case 45: return x-y;
		case 47: return x/y;
	}
	return 0;
}






























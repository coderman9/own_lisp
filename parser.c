#include <stdio.h>
#include <stdlib.h>

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

//argc is argument count, argv is argument vector
//char** is a list of strings (approximately)
int main(int argc, char** argv)
{
	puts("Lispy Version 0.0.0.0.1");
	//note extra new line
	puts("Press Ctrl+C to exit\n");
	while(1)
	{
		//note doesn't read in newline
		char* input = readline("lispy> ");
		add_history(input);
		printf("echo: %s\n", input);
		free(input);
	}
	return 0;
}

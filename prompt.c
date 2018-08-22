#include <stdio.h>

//buffer for user input of size 2048 (bytes, since each char is a byte)
//note: a static global variable means lexical scope is confined to this "compilation unit" (can probs be understood as file).
//static inside of a function means that it keeps its value between invocations
static char input[2048];

//argc is argument count, argv is argument vector
//char** is a list of strings (approximately)
int main(int argc, char** argv)
{
	puts("Lispy Version 0.0.0.0.1");
	//note extra new line
	puts("Press Ctrl+C to exit\n");
	while(1)
	{
		fputs("lisp> ", stdout);
		//reads stdin of max size 2048 to new var input
		fgets(input, 2048, stdin);
		printf("echo: %s", input);
	}
	return 0;
}

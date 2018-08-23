#include "mpc.h"
#include <editline/readline.h>
#include <editline/history.h>
//grammer spec cribbed from wikipedia article on chomsky hierarchies

int main(int argc, char** argv)
{
	mpc_parser_t* noun = mpc_new("noun");
	mpc_parser_t* verb = mpc_new("verb");
	mpc_parser_t* adjective = mpc_new("adj");
	mpc_parser_t* nounphrase = mpc_new("nounp");
	mpc_parser_t* verbphrase = mpc_new("verbp");
	mpc_parser_t* sentence = mpc_new("sent");
	
	mpca_lang(MPCA_LANG_DEFAULT,
	"\
	 noun: \"ideas\" | \"linguists\"; \
	 verb: \"generate\" | \"hate\"; \
	 adj: \"great\" | \"green\"; \
	 nounp: <noun> | <adj> <nounp> ; \
	 verbp: <verb> <nounp> | <verb>; \
	 sent: /^/ <nounp> <verbp>/$/; \
	",
	noun, verb, adjective, nounphrase, verbphrase, sentence);
	
	puts("Chomsky Hierarchy / mpc Example");

	while(1)
	{
		char* input = readline("parse> ");
		add_history(input);
		mpc_result_t r;
		if(mpc_parse("<stdin>", input, sentence, &r))
		{
			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);	
		}
		else
		{
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}
	mpc_cleanup(6, noun, verb, adjective, nounphrase, verbphrase, sentence);
	return 0;
}

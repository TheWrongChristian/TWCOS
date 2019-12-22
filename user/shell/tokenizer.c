#include <string.h>

#include "tokenizer.h"

#if INTERFACE

#include <stddef.h>

struct tokenizer_t {
	char cclass[1<<8];
	int options;
	char * token;
	int line;
	int column;
};

typedef void (*tokenizer_consumer_t)(void * arg, int ttype, char * token, size_t tokenlen);

#define TOKENIZER_NEWLINES	1<<0
#define TOKENIZER_SLASHSLASH	1<<1
#define TOKENIZER_SLASHSTAR	1<<2
#define TOKENIZER_HASH		1<<3
#define TOKENIZER_DASHDASH	1<<4

#define TOKENIZER_WORD		1<<8
#define TOKENIZER_STRING	1<<9

#endif

#define WORD_CHAR 1<<0
#define SPACE_CHAR 1<<1
#define QUOTE_CHAR 1<<2

void tokenizer_init(tokenizer_t * tokenizer)
{
	memset(tokenizer->cclass, 0, sizeof(tokenizer->cclass));
	tokenizer_wordchars(tokenizer, 'a', 'z');
	tokenizer_wordchars(tokenizer, 'A', 'Z');
	tokenizer_wordchars(tokenizer, '_', '_');
	tokenizer_wordchars(tokenizer, '0', '9');
	tokenizer_spacechars(tokenizer, 1, ' ');
	tokenizer_quotechars(tokenizer, '\'', '\'');
	tokenizer_quotechars(tokenizer, '\"', '\"');
	tokenizer_options(tokenizer, 0);
	tokenizer->line=1;
	tokenizer->column=1;
}

void tokenizer_wordchars(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] |= WORD_CHAR;
	}
}

void tokenizer_spacechars(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] |= SPACE_CHAR;
	}
}

void tokenizer_quotechars(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] |= QUOTE_CHAR;
	}
}

void tokenizer_options(tokenizer_t * tokenizer, int options)
{
}

void tokenizer_tokenize(tokenizer_t * tokenizer, char * buf, size_t buflen, tokenizer_consumer_t consumer, void * arg)
{
	int inword=0;
	int inspace=0;
	int inquote=0;
	int inescape=0;
	char * token=buf;
	for(int i=0; i<buflen && buf[i]; i++) {
		char cclass=tokenizer->cclass[buf[i]];

		if (inescape) {
			inescape=0;
			continue;
		}

		if (inquote && '\\'==buf[i]) {
			inescape=1;
			continue;
		}

		if (inquote==buf[i]) {
			char * tokenend=buf+i;
			consumer(arg, TOKENIZER_STRING, token, 1+tokenend-token);
			inquote=0;
			continue;
		}

		if (inword && !(WORD_CHAR & cclass)) {
			/* We have a word token */
			char * tokenend=buf+i;
			consumer(arg, TOKENIZER_WORD, token, tokenend-token);
			inword=0;
		}

		if (inspace && !(SPACE_CHAR & cclass)) {
			inspace=0;
		}

		if (!inword && !inspace && !inquote) {
			if (WORD_CHAR & cclass) {
				inword=1;
				token=buf+i;
			} else if (SPACE_CHAR & cclass) {
				inspace=1;
			} else if (QUOTE_CHAR & cclass) {
				inquote=buf[i];
				token=buf+i;
			} else {
				consumer(arg, buf[i], buf+i, 1);
			}
		}
		tokenizer->column++;
		if ('\n' == buf[i]) {
			tokenizer->line++;
			tokenizer->column=1;
		}
	}

	if (inword) {
		/* We have a word token */
		char * tokenend=buf+buflen;
		consumer(arg, TOKENIZER_WORD, token, tokenend-token);
	}
}

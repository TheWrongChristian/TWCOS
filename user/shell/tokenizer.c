#include <string.h>

#include "tokenizer.h"

#if INTERFACE

#include <stddef.h>

struct tokenizer_t {
	char cclass[2<<8];
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

#endif

#define WORD_CHAR 1<<0
#define SPACE_CHAR 1<<1

void tokenizer_init(tokenizer_t * tokenizer)
{
	memset(tokenizer->cclass, 0, sizeof(tokenizer->cclass));
	tokenizer_wordchars(tokenizer, 'a', 'z');
	tokenizer_wordchars(tokenizer, 'A', 'Z');
	tokenizer_spacechars(tokenizer, 1, ' ');
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

void tokenizer_options(tokenizer_t * tokenizer, int options)
{
}

void tokenizer_tokenize(tokenizer_t * tokenizer, char * buf, size_t buflen, tokenizer_consumer_t consumer, void * arg)
{
	int inword=0;
	int inspace=0;
	char * token=buf;
	for(int i=0; i<buflen && buf[i]; i++) {
		char cclass=tokenizer->cclass[buf[i]];
		tokenizer->column++;

		if (inword && !(WORD_CHAR & cclass)) {
			/* We have a word token */
			char * tokenend=buf+i;
			consumer(arg, TOKENIZER_WORD, token, tokenend-token);
			inword=0;
		}

		if (inspace && !(SPACE_CHAR & cclass)) {
			inspace=0;
		}
		if (!inword && !inspace) {
			if (WORD_CHAR & cclass) {
				inword=1;
				token=buf+i;
			} else if (SPACE_CHAR & cclass) {
				inspace=1;
			} else {
				consumer(arg, buf[i], buf+i, 1);
			}
		}
		if ('\n' == buf[i]) {
			tokenizer->line++;
			tokenizer->column=0;
		}
	}

	if (inword) {
		/* We have a word token */
		char * tokenend=buf+buflen;
		consumer(arg, TOKENIZER_WORD, token, tokenend-token);
	}
}

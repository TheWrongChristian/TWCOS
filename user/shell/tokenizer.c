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

enum tokenizer_mode { tokenizer_C_mode, tokenizer_shell_mode, tokenizer_filename_mode };

#define TOKENIZER_NEWLINES	1<<0
#define TOKENIZER_SLASHSLASH	1<<1
#define TOKENIZER_SLASHSTAR	1<<2
#define TOKENIZER_HASH		1<<3
#define TOKENIZER_DASHDASH	1<<4

#define TOKENIZER_WORD		1<<8
#define TOKENIZER_STRING	1<<9

#define TOKENIZER_WORD_CHAR 1<<0
#define TOKENIZER_SPACE_CHAR 1<<1
#define TOKENIZER_QUOTE_CHAR 1<<2

#endif

void tokenizer_set_mode(tokenizer_t * tokenizer, enum tokenizer_mode mode)
{
	switch(mode)
	{
	case tokenizer_C_mode:
		tokenizer_wordchars(tokenizer, '_', '_');
		tokenizer_default(tokenizer, '-', '-');
		tokenizer_default(tokenizer, '/', '/');
		break;
	case tokenizer_shell_mode:
		tokenizer_wordchars(tokenizer, '_', '_');
		tokenizer_wordchars(tokenizer, '-', '-');
		tokenizer_default(tokenizer, '/', '/');
		break;
	case tokenizer_filename_mode:
		tokenizer_wordchars(tokenizer, '_', '_');
		tokenizer_wordchars(tokenizer, '-', '-');
		tokenizer_wordchars(tokenizer, '/', '/');
		break;
	}
}

void tokenizer_default(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] = 0;
	}
}

void tokenizer_escapechars(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] |= TOKENIZER_WORD_CHAR;
	}
}

void tokenizer_wordchars(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] |= TOKENIZER_WORD_CHAR;
	}
}

void tokenizer_spacechars(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] |= TOKENIZER_SPACE_CHAR;
	}
}

void tokenizer_quotechars(tokenizer_t * tokenizer, int low, int high)
{
	for(int i=low; i<=high && i<sizeof(tokenizer->cclass); i++) {
		tokenizer->cclass[i] |= TOKENIZER_QUOTE_CHAR;
	}
}

void tokenizer_options(tokenizer_t * tokenizer, int options)
{
}

void tokenizer_init(tokenizer_t * tokenizer)
{
	memset(tokenizer->cclass, 0, sizeof(tokenizer->cclass));
	tokenizer_wordchars(tokenizer, 'a', 'z');
	tokenizer_wordchars(tokenizer, 'A', 'Z');
	tokenizer_wordchars(tokenizer, '0', '9');
	tokenizer_spacechars(tokenizer, 1, ' ');
	tokenizer_quotechars(tokenizer, '\'', '\'');
	tokenizer_quotechars(tokenizer, '\"', '\"');
	tokenizer_set_mode(tokenizer, tokenizer_C_mode);
	tokenizer_options(tokenizer, 0);
	tokenizer->line=1;
	tokenizer->column=1;
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

		if (inword && !(TOKENIZER_WORD_CHAR & cclass)) {
			/* We have a word token */
			char * tokenend=buf+i;
			consumer(arg, TOKENIZER_WORD, token, tokenend-token);
			inword=0;
		}

		if (inspace && !(TOKENIZER_SPACE_CHAR & cclass)) {
			inspace=0;
		}

		if (!inword && !inspace && !inquote) {
			if (TOKENIZER_WORD_CHAR & cclass) {
				inword=1;
				token=buf+i;
			} else if (TOKENIZER_SPACE_CHAR & cclass) {
				inspace=1;
			} else if (TOKENIZER_QUOTE_CHAR & cclass) {
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

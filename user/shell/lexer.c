#include "lexer.h"

#if INTERFACE

struct token_t {
	char * input;
	int line;
	int column;
	cbuffer_t token[1];
};

struct lexer_ops_t {
	void (*addc)(lexer_t * lexer, int c);
};

struct lexer_t {
	lexer_ops_t * ops;
};

#endif

typedef struct {
	lexer_t lexer;
	token_t token;
	cbuffer_t * buf;
	void (*consumer)(void * arg, token_t * token);
	void * arg; 

	int state;
} clexer_t;

void lexer_addc(lexer_t * lexer, int c)
{
	lexer->ops->addc(lexer, c);
}

void lexer_adds(lexer_t * lexer, char * s)
{
	while(*s) {
		int c = *s++;

		if (c<0x80) {
			lexer_addc(lexer, c);
		} else if (*s) {
			int extra = 0;

			if ((c&0xe0) == 0xc0) {
				c <<= 5;
				c += 0xe0 & *s;
				extra = 1;
			} else if ((c&0xf0) == 0xe0) {
				c <<= 4;
				c += 0xf0 & *s;
				extra = 2;
			} else if ((c&0xf8) == 0xf0) {
				c <<= 3;
				c += 0xf8 & *s;
				extra = 3;
			}

			for(int i=0; i<extra && *s; i++, s++) {
				c <<= 6;
				c += 0xc0 & *s;
			}

			lexer_addc(lexer, c);
		} else {
			/* Invalid encoding - end of string */
			return;
		}
	}
}


#define CLEXER_BEGIN(clexer) while(1) switch(clexer->state)
#define CLEXER_START() case 0: do {} while(0)
#define CLEXER_NEXT(clexer) clexer->state = __LINE__ ; return ; case __LINE__: do {} while(0)
#define CLEXER_RESET(clexer) clexer->state = 0; continue;


void clexer_addc(lexer_t * lexer, int c)
{
	clexer_t * clexer = container_of(lexer, clexer_t, lexer);

	CLEXER_BEGIN(clexer) {
		CLEXER_START();
		if (cbuffer_len(clexer->buf)) {
			clexer->consumer(clexer->arg, &clexer->token);
			cbuffer_trunc(clexer->buf, 0);
		}

		if (isalpha(c) || '_' == c) {
			while(isalnum(c) || '_' == c) {
				cbuffer_addc(clexer->buf, c);
				CLEXER_NEXT(clexer);
			}
			CLEXER_RESET(clexer);
		} else if (isdigit(c)) {
			if ('0' == c) {
				cbuffer_addc(clexer->buf, c);
				CLEXER_NEXT(clexer);
				if ('x' == c) {
					cbuffer_addc(clexer->buf, c);
					CLEXER_NEXT(clexer);

					while(isxdigit(c)) {
						cbuffer_addc(clexer->buf, c);
						CLEXER_NEXT(clexer);
					}
					CLEXER_RESET(clexer);
				}
			}
			while(isxdigit(c)) {
				cbuffer_addc(clexer->buf, c);
				CLEXER_NEXT(clexer);
			}
			CLEXER_RESET(clexer);
		} else if (isspace(c)) {
			if ('\n' == c) {
				clexer->token.line++;
				clexer->token.column = 0;
			}
			CLEXER_NEXT(clexer);
			CLEXER_RESET(clexer);
		} else {
			cbuffer_addc(clexer->buf, c);
			CLEXER_NEXT(clexer);
			CLEXER_RESET(clexer);
		}
	}
}

lexer_t * clexer_new(void (*consumer)(void * arg, token_t * token), void * arg)
{
	static lexer_ops_t ops = { clexer_addc };
	clexer_t * clexer = calloc(1, sizeof(*clexer));

	clexer->lexer.ops = &ops;
	clexer->state = 0;
	clexer->buf = &clexer->token.token;
	clexer->consumer = consumer;
	clexer->arg = arg;

	return &clexer->lexer;
}

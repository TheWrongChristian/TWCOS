#if INTERFACE

#define isalpha(c) ((c>='a' && c<='z') || (c>='A' && c<='Z'))
#define isdigit(c) (c>='0' && c<='9')
#define isxdigit(c) (isdigit(c) || (c>='a' && c<='f') || (c>='A' && c<='F'))
#define isspace(c) (c == '\n' || c == ' ' || c == '\t' || c == '\r')
#define isalnum(c) (isalpha(c) || isdigit(c))

#endif

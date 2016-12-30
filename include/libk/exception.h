#ifndef EXCEPTION_DEF

#define EXCEPTION_DEF(type,parent) static struct exception_def exception_def_ ## type = { #type, &exception_def_ ## parent }

typedef struct exception_def {
        const char * name;
        struct exception_def * parent;
} exception_def;

extern struct exception_def exception_def_Throwable;
EXCEPTION_DEF(Exception, Throwable);
EXCEPTION_DEF(Error, Throwable);
#endif


#if INTERFACE

extern int _call_kernel(...);

#endif

void exit(int code)
{
	_call_kernel(code);
}

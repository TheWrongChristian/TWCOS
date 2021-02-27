char ** environ;

int cstart(int argc, char * argv0)
{
	char ** argv=&argv0;
	environ=argv+argc+1;
	return main(argc, argv, environ);
}

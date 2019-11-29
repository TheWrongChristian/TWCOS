int cstart(int argc, char * argv0)
{
	char ** argv=&argv0;
	char ** envp=argv+argc+1;
	return main(argc, argv, envp);
}

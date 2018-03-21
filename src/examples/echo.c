#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
  int i;

  char* buffer="Hello word!\n";
  int res=write (1, buffer, 12);
  printf("Value is '%i'\n",res);

  buffer="Goodbye world!\n";
  write (1, buffer, 15);

  /*
  for (i = 0; i < argc; i++)
     printf ("%s ", argv[i]);
   printf ("\n");
  */
  return EXIT_SUCCESS;
}

#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
  //int i;

  char* buffer="Hello word!\n";
  write (1, buffer, 12);

  /*for (i = 0; i < argc; i++)
     printf ("%s ", argv[i]);
   printf ("\n");
 */
  return EXIT_SUCCESS;
}

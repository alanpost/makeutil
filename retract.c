/*
 * retract - create a text archive from input.
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int forarg();

static
retract(filename, file)
  char *filename;
  FILE *file;
{
  char p[BUFSIZ];
  size_t rsize, wsize;

  fprintf(stdout, "<++> %s\n", filename);
  while(rsize=fread(&p[0], sizeof(char), sizeof p/sizeof p[0], file)) {
    if(rsize!=(wsize=fwrite(p, sizeof(char), rsize, stdout))) {
      perror("error: write");
      exit(EXIT_FAILURE);
    }
  }
  if(ferror(file)) {
    perror("error: read");
    return EXIT_FAILURE;
  }

  if('\n'!=p[wsize-1]) fputc('\n', stdout);
  fputs("<-->\n\f\n", stdout);

  if(ferror(stdout)) {
    perror("error: write");
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}

main(argc, argv)
  char *argv[];
{
  exit(forarg(argc, argv, retract, "r"));
}

/*                                                              ..__
 *                                                              `' "
 */

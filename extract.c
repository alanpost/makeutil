/*
 * extract - extract files from text archive
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: extract.c,v 1.1 2004/11/28 22:57:53 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern int forarg();

static
extract(filename, file)
  char *filename;
  FILE *file;
{
  int retval=EXIT_SUCCESS;
  char line[256];
  FILE *outfile=NULL;
  size_t header, footer;

  header=strlen("<++>");
  footer=strlen("<-->");

  while(NULL!=fgets(line, sizeof line/sizeof line[0], file)) {
    if((NULL!=outfile) && (0==strncmp("<-->", line, footer))) {
      if((outfile!=stdout) && (EOF==fclose(outfile))) {
        perror("error: close");
        retval|=EXIT_FAILURE;
      }
      outfile=NULL;
    }

    if((NULL!=outfile) && (EOF==fputs(line, outfile))) {
      perror("error: write");
      retval|=EXIT_FAILURE;
      fclose(outfile);
      outfile=NULL;
    }

    if((NULL==outfile) && (0==strncmp("<++>", line, header))) {
      size_t n, c=header;

      /* remove newline */
      n=strlen(line);
      if(n && '\n'==line[n-1]) line[--n]='\0';

      while(isspace(line[c])) ++c;

      if(0==strcmp("", line+c)) {
        fputs("warning: missing filename after '<++>'\n", stderr);
        retval|=EXIT_FAILURE;
      } else if(0==strcmp("-", line+c)) {
        outfile=stdout;
      } else if(NULL==(outfile=fopen(line+c, "w"))) {
        perror("error: open");
        retval|=EXIT_FAILURE;
      }
    }
  }
  if(ferror(file)) {
    perror("error: read");
    retval|=EXIT_FAILURE;
  }

  if((NULL!=outfile) && (stdout!=outfile) && (EOF==fclose(outfile))) {
    perror("error: close");
    retval|=EXIT_FAILURE;
  }
  outfile=NULL;

  return retval;
}

main(argc, argv)
  char *argv[];
{
  exit(forarg(argc, argv, extract, "r"));
}

/*                                                              ..__
 *                                                              `' "
 */

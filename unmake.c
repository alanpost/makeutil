/*
 * unmake - process include directive in Makefile
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: unmake.c,v 1.1 2004/11/28 22:57:53 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char line[1024]; /*save stack space in unmake*/

/*
 * recursively unroll filename.
 *
 * on error, we can't report the filename because it has been clobbered by
 * fgets.
 */
static void
unmake(filename)
  char *filename;
{
  FILE *file;
  unsigned number;
  unsigned len;

  if(!filename) return;

  /*
   * open the input file
   */
  if(0==(file=fopen(filename, "r"))) {
    fprintf(stderr, "error: open: %s", filename);
    perror("");
    exit(1);
  }

  number=0;
  while(fgets(line, sizeof(line)/sizeof(line[0]), file)) {
    ++number;

    /*
     * if we find an include, extract the filename and inline the file.
     */
    if(0==strncmp("include ", line, len=strlen("include "))) {
      unsigned size;
      size=strlen(line); if(size) --size;
      if('\n'==line[size]) line[size]='\0';
      unmake(line+len);
    } else {
      if(EOF==fputs(line, stdout)) {
        perror("error: write: -");
        exit(1);
      }
    }
  }
  if(ferror(file)) {
    perror("error: read");
    exit(1);
  }

  if(EOF==fclose(file)) {
    perror("error: close");
    exit(1);
  }
}

main(argc, argv)
  char *argv[];
{
  char *filename;

  --argc;
  ++argv;

  filename=argc?*argv:"Makefile";
  unmake(filename);
  exit(0);
}

/*                                                              ..__
 *                                                              `' "
 */

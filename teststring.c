/*
 * teststring - test/sample program for mkstring.
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: teststring.c,v 1.1 2004/12/02 02:34:12 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>

extern char string[];

main(argc, argv)
  char *argv;
{
  fputs(&string[0], stdout);
  fputs("\n", stdout);
  exit(EXIT_SUCCESS);
}

/*                                                              ..__
 *                                                              `' "
 */

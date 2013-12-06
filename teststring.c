/*
 * teststring - test/sample program for mkstring.
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

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

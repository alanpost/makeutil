/*
 * testarray - test/sample program for mkarray.
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: testarray.c,v 1.3 2004/12/02 02:33:29 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>

extern char array[];
extern size_t _array_size, *array_size;

main(argc, argv)
  char *argv;
{
  fwrite(&array[0], _array_size, sizeof(char), stdout);
  exit(EXIT_SUCCESS);
}

/*                                                              ..__
 *                                                              `' "
 */

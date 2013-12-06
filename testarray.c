/*
 * testarray - test/sample program for mkarray.
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

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

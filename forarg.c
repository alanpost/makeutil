/*
 * forarg - open each file and call closure
 *
 * this file is part of the makeutil package:
 *   http://sourceforge.net/projects/makeutil/
 *   http://www.cybermesa.com/~aisa/makeutil/
 *
 * this file is hereby placed in the public domain.
 * aisa0@users.sourceforge.net, aisa@cybermesa.com
 */

static char rcsid[]="$Id: forarg.c,v 1.2 2006/06/19 03:44:30 aisa0 Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

forarg(argc, argv, func, mode)
  char *argv[];
  int (*func)();
  char *mode;
{
  int status=EXIT_SUCCESS;

  --argc;
  ++argv;

  if(argc) {
    int i;
    for(i=0;i<argc;++i) {
      if(0==strcmp("-", argv[i])) {
        status|=(*func)("-", stdin);
      } else {
        FILE *file;

        if(NULL!=(file=fopen(argv[i], mode))) {
          status|=(*func)(argv[i], file);
          if(EOF==fclose(file)) {
            perror("error: close");
            status|=EXIT_FAILURE;
          }
        } else {
          perror("error: open");
          status|=EXIT_FAILURE;
        }
      }
    }
  } else {
    status|=(*func)("-", stdin);
  }

  return status;
}

/*                                                              ..__
 *                                                              `' "
 */

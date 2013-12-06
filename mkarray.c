/*
 * mkarray - make a c array containing the input file.
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

static
mkarray(filename, file, varname)
  char *filename;
  FILE *file;
  char *varname;
{
  char buf[BUFSIZ];
  size_t i, rsize, tsize=0;
  int done=0, line=8, status=EXIT_SUCCESS;

  fprintf(stdout, "/*\n");
  fprintf(stdout, " * this file is automatically generated\n");
  fprintf(stdout, " */\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "#include <string.h>\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "char %s[]=\n{\n", varname);

  while(!done) {
    int i, ch;

    rsize=fread(&buf[0], sizeof(char), sizeof buf/sizeof buf[0], file);
    tsize+=rsize;

    switch(rsize) {
    case 0:
      rsize=1;
      buf[0]='\0';
      done=1;

      /* down seems more likely */

    default:
      for(i=0;i<rsize;++i) {
        if(8==line) fputc('\t', stdout);

        line+=(ch=fprintf(stdout, "0x%02x,", buf[i]&0xff));

        if(line+ch>=78) {
          line=8;
          fputc('\n', stdout);
        }
      }
    }
  }
  if(ferror(file)) {
    perror("error: read");
    status|=EXIT_FAILURE;
  }

  if(8!=line) fputc('\n', stdout);
  fputs("};\n\n", stdout);

  fprintf(stdout, "size_t _%s_size=%d;\n", varname, tsize);
  fprintf(stdout, "size_t *%s_size=&_%s_size;\n", varname, varname);

  if(ferror(stdout)) {
    perror("error: write: -: ");
    status|=EXIT_FAILURE;
  }
  if(EOF==fclose(stdout)) {
    perror("error: close: -: ");
    status|=EXIT_FAILURE;
  }

  return status;
}

main(argc, argv)
  char *argv[];
{
  char *filename, *varname;
  FILE *file;
  int status=EXIT_SUCCESS;

  --argc;
  ++argv;

  if(argc!=2) {
      fputs("usage: mkarray <file> <name>\n", stderr);
      exit(status|EXIT_FAILURE);
  }

  filename=argv[0];
  varname=argv[1];

  if(0==strcmp("-", filename)) {
    status|=mkarray(filename, stdin, varname);
  } else {
    FILE *file;

    if(NULL!=(file=fopen(filename, "rb"))) {
      status|=mkarray(filename, file, varname);
      if(EOF==fclose(file)) {
        perror("error: close");
        status|=EXIT_FAILURE;
      }
    } else {
      perror("error: open");
      status|=EXIT_FAILURE;
    }
  }

  exit(status);
}

/*                                                              ..__
 *                                                              `' "
 */

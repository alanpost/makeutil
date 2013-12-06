/*
 * any #include directive in a comment is ignored:
 *
 * #include "rm/comment.h"
 */

/*
 * this isn't even valid syntax:
#include \"rm/escape.h\"
 */

/*
 * any #include directive in a string is ignored:
"#include \"rm/insidequote0.h\""
"#include <rm/insidequote1.h>"
 */

/*
 * here is a tricky string masquerading as an
 * #include directive:
"\"#include "FOO""
 */

/*
 * this is valid syntax, but unsupported mkdep:
#define DEFINCLUDE "defined.h"
#include DEFINCLUDE
 */

/*
 * here are several valid cases:
 */
#include"h1.h"
#include "h2.h"
#include	"h3.h"
# include"h4.h"
# include "h5.h"
# include	"h6.h"
#	include"h7.h"
#	include "h8.h"
#	include	"h9.h"
#    include     "ha.h"

#include<s1.h>
#include <s2.h>
#include	<s3.h>
# include<s4.h>
# include <s5.h>
# include	<s6.h>
#	include<s7.h>
#	include <s8.h>
#	include	<s9.h>
#    include     <sa.h>

/*
 * and round out with some system headers:
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

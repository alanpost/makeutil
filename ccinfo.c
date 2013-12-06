/*
 * ccinfo - determine which c compiler we are being compiled by
 *
 * this file is part of the makeutil package:
 *   https://github.com/alanpost/makeutil
 *
 * this file is hereby placed in the public domain.
 * alyn.post@lodockikumazvati.org
 */

#include <stdio.h>
#include <stdlib.h>

borland()
{
#ifdef __BORLANDC__
	char *cc;
	unsigned major, minor;

	cc="bcc";
	major=__BORLANDC__/0x100U;
	minor=__BORLANDC__%0x100U;
	while(minor && 0x0U==minor%0x10U) minor/=0x10U;
	printf("%s-%x.%x\n", cc, major, minor);
	exit(0);
#endif
}

gcc()
{
#ifdef __GNUC__
	char *cc;
	unsigned major, minor, patch;

	cc="gcc";
	major=__GNUC__;
	minor=__GNUC_MINOR__;
#ifdef __GNUC_PATCHLEVEL__
	patch=__GNUC_PATCHLEVEL__;
	printf("%s-%u.%u.%u\n", cc, major, minor, patch);
#else
	patch=0U;
	printf("%s-%u.%u\n", cc, major, minor);
#endif
	exit(0);
#endif
}

msc()
{
#ifdef _MSC_VER
	char *cc;
	unsigned major, minor;

	cc="msc";
	major=_MSC_VER/100U;
	minor=_MSC_VER%100U;
	while(minor && 0U==minor%10U) minor/=10U;
	printf("%s-%u.%u\n", cc, major, minor);
	exit(0);
#endif
}

sunpro()
{
#ifdef __SUNPRO_C
	char *cc;
	unsigned major, minor;

	cc="sunpro";
	major=__SUNPRO_C/0x100U;
	minor=__SUNPRO_C%0x100U;
	while(minor && 0x0U==minor%0x10U) minor/=0x10U;
	printf("%s-%x.%x\n", cc, major, minor);
	exit(0);
#endif
}

unknown()
{
	char *cc;

	cc="unknown";
	printf("%s-\n", cc);
	exit(0);
}

main()
{
	borland();
	gcc();
	msc();
	sunpro();
	unknown();
	exit(0);
}

/*                                                              ..__
 *                                                              `' "
 */

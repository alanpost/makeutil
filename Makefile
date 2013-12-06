#
# makeutil - build system support tools
#
# this file is part of the makeutil package:
#   https://github.com/alanpost/makeutil
#
# this file is hereby placed in the public domain.
# alyn.post@lodockikumazvati.org
#
# default   - build all targets.
# test      - run the test suite.
# dist      - create a source distribution.  will need tweeking
#             if your platform does not match mine.
# install   - install all targets to $prefix.  please note that
#             this package is not intended to be installed, see
#             README.
# uninstall - remove files from $prefix.
# clean     - remove all derived files.
#
# if you do not wish to use make, this software can be build with
# the included bootstrap file:
#
#  $ sh bootstrap
#
# it is not intended that you use this Makefile in your own project.
# instead, pull the tools you will be using and incorporate them in
# your own build system.  The rules here are simple enough that you
# shouldn't hassle with trying to get this makefile to work with
# whatever system you are using to build.
#

SHELL=/bin/sh
prefix=/usr/local

default: \
	addcr \
	armor \
	b00t \
	ccinfo \
	config \
	dearmor \
	delcr \
	extract \
	mkarray \
	mkdep \
	mkstring \
	retract \
	systype \
	testarray \
	teststring \
	textpack \
	textpand \
	unmake \
	makeutil.text

# to recreate the bootstrap file:
#bootstrap: Makefile
#	gmake -nrWMakefile > bootstrap || (rm -f bootstrap && exit 1)

addcr: Makefile addcr.o forarg.o
	cc -o addcr addcr.o forarg.o

armor: Makefile armor.o forarg.o
	cc -o armor armor.o forarg.o

b00t: Makefile b00t.o
	cc -o b00t b00t.o

ccinfo: Makefile ccinfo.o
	cc -o ccinfo ccinfo.o

ckey: Makefile ckey.o forarg.o
	cc -o ckey ckey.o forarg.o

config: Makefile config.o
	cc -o config config.o

dearmor: Makefile dearmor.o forarg.o
	cc -o dearmor dearmor.o forarg.o

delcr: Makefile delcr.o forarg.o
	cc -o delcr delcr.o forarg.o

extract: Makefile extract.o forarg.o
	cc -o extract extract.o forarg.o

mkarray: Makefile mkarray.o
	cc -o mkarray mkarray.o

mkdep: Makefile mkdep.o
	cc -o mkdep mkdep.o

mkstring: Makefile mkstring.o
	cc -o mkstring mkstring.o

retract: Makefile retract.o forarg.o
	cc -o retract retract.o forarg.o

systype: Makefile mksystype conf-pdesc ccinfo
	sh mksystype > systype || (rm -f systype && exit 1)

testarray: Makefile testarray.o test0.o
	cc -o testarray testarray.o test0.o

teststring: Makefile teststring.o test1.o
	cc -o teststring teststring.o test1.o

textpack: Makefile textpack.o tree.o freq.o forarg.o
	cc -o textpack textpack.o tree.o freq.o forarg.o

textpand: Makefile textpand.o tree.o freq.o forarg.o
	cc -o textpand textpand.o tree.o freq.o forarg.o

unmake: Makefile unmake.o
	cc -o unmake unmake.o

makeutil.text: \
	retract \
	README \
	README.b00t \
	AUTHORS \
	Changes \
	EXTERN \
	LICENSE \
	Makefile \
	TODO \
	VERSION \
	addcr.c \
	armor.c \
	b00t.c \
	bootstrap \
	ccinfo.c \
	ckey.c \
	conf-pdesc \
	config.c \
	config.db \
	dearmor.c \
	delcr.c \
	extract.c \
	forarg.c \
	init.l1 \
	mkarray.c \
	mkdep.c \
	mkstring.c \
	mksystype \
	retract.c \
	test.db \
	testarray.c \
	teststring.c \
	textpack.c \
	textpand.c \
	test.bl \
	test.db \
	test.l1 \
	textpack.c \
	textpand.c \
	tree.c \
	tree.h \
	unmake.c \
	vpath.l1
	./retract \
	README \
	README.b00t \
	AUTHORS \
	Changes \
	EXTERN \
	LICENSE \
	Makefile \
	TODO \
	VERSION \
	addcr.c \
	armor.c \
	b00t.c \
	bootstrap \
	ccinfo.c \
	ckey.c \
	conf-pdesc \
	config.c \
	config.db \
	dearmor.c \
	delcr.c \
	extract.c \
	forarg.c \
	init.l1 \
	mkarray.c \
	mkdep.c \
	mkstring.c \
	mksystype \
	retract.c \
	test.db \
	testarray.c \
	teststring.c \
	textpack.c \
	textpand.c \
	test.bl \
	test.db \
	test.l1 \
	textpack.c \
	textpand.c \
	tree.c \
	tree.h \
	unmake.c \
	vpath.l1 \
	> makeutil.text || (rm -f makeutil.text && exit 1)

addcr.o: Makefile addcr.c
	cc -c -o addcr.o addcr.c

armor.o: Makefile armor.c
	cc -c -o armor.o armor.c

b00t.o: Makefile b00t.c
	cc -c -o b00t.o b00t.c

ccinfo.o: Makefile ccinfo.c
	cc -c -o ccinfo.o ccinfo.c

ckey.o: Makefile ckey.c
	cc -c -o ckey.o ckey.c

config.o: Makefile config.c
	cc -c -o config.o config.c

dearmor.o: Makefile dearmor.c
	cc -c -o dearmor.o dearmor.c

delcr.o: Makefile delcr.c
	cc -c -o delcr.o delcr.c

extract.o: Makefile extract.c
	cc -c -o extract.o extract.c

mkarray.o: Makefile mkarray.c
	cc -c -o mkarray.o mkarray.c

mkdep.o: Makefile mkdep.c
	cc -c -o mkdep.o mkdep.c

mkstring.o: Makefile mkstring.c
	cc -c -o mkstring.o mkstring.c

retract.o: Makefile retract.c
	cc -c -o retract.o retract.c

test0.o: Makefile test0.c
	cc -c -o test0.o test0.c

test0.c: Makefile mkarray testarray.c
	./mkarray testarray.c array > test0.c || (rm -f test0.c && exit 1)

test1.o: Makefile test1.c
	cc -c -o test1.o test1.c

test1.c: Makefile mkstring conf-pdesc
	./mkstring conf-pdesc string > test1.c || (rm -f test1.c && exit 1)

testarray.o: Makefile testarray.c
	cc -c -o testarray.o testarray.c

teststring.o: Makefile teststring.c
	cc -c -o teststring.o teststring.c

textpack.o: Makefile textpack.c tree.h
	cc -c -o textpack.o textpack.c

textpand.o: Makefile textpand.c tree.h
	cc -c -o textpand.o textpand.c

unmake.o: Makefile unmake.c
	cc -c -o unmake.o unmake.c

forarg.o: Makefile forarg.c
	cc -c -o forarg.o forarg.c

freq.o: Makefile freq.c
	cc -c -o freq.o freq.c

freq.c: Makefile freq.d mkarray
	./mkarray freq.d freq > freq.c || (rm -f freq.c && exit 1)

freq.d: Makefile makeutil.text ckey
	./ckey < makeutil.text > freq.d || (rm -f freq.d && exit 1)

tree.o: Makefile tree.c tree.h
	cc -c -o tree.o tree.c

test: default
	./retract  < AUTHORS       | ./extract  | diff AUTHORS       -
	./retract  < Changes       | ./extract  | diff Changes       -
	./retract  < EXTERN        | ./extract  | diff EXTERN        -
	./retract  < LICENSE       | ./extract  | diff LICENSE       -
	./retract  < Makefile      | ./extract  | diff Makefile      -
	./retract  < README        | ./extract  | diff README        -
	./retract  < README.b00t   | ./extract  | diff README.b00t   -
	./retract  < TODO          | ./extract  | diff TODO          -
	./retract  < VERSION       | ./extract  | diff VERSION       -
	./retract  < addcr.c       | ./extract  | diff addcr.c       -
	./retract  < armor.c       | ./extract  | diff armor.c       -
	./retract  < b00t.c        | ./extract  | diff b00t.c        -
	./retract  < bootstrap     | ./extract  | diff bootstrap     -
	./retract  < ccinfo.c      | ./extract  | diff ccinfo.c      -
	./retract  < ckey.c        | ./extract  | diff ckey.c        -
	./retract  < conf-pdesc    | ./extract  | diff conf-pdesc    -
	./retract  < config.c      | ./extract  | diff config.c      -
	./retract  < config.db     | ./extract  | diff config.db     -
	./retract  < dearmor.c     | ./extract  | diff dearmor.c     -
	./retract  < delcr.c       | ./extract  | diff delcr.c       -
	./retract  < extract.c     | ./extract  | diff extract.c     -
	./retract  < forarg.c      | ./extract  | diff forarg.c      -
	./retract  < init.l1       | ./extract  | diff init.l1       -
	./retract  < mkarray.c     | ./extract  | diff mkarray.c     -
	./retract  < mkdep.c       | ./extract  | diff mkdep.c       -
	./retract  < mkstring.c    | ./extract  | diff mkstring.c    -
	./retract  < mksystype     | ./extract  | diff mksystype     -
	./retract  < retract.c     | ./extract  | diff retract.c     -
	./retract  < textpand.c    | ./extract  | diff textpand.c    -
	./retract  < textpack.c    | ./extract  | diff textpack.c    -
	./retract  < tree.c        | ./extract  | diff tree.c        -
	./retract  < tree.h        | ./extract  | diff tree.h        -
	./retract  < unmake.c      | ./extract  | diff unmake.c      -
	./retract  < vpath.l1      | ./extract  | diff vpath.l1      -
	./armor    < makeutil.text | ./dearmor  | diff makeutil.text -
	./armor    < AUTHORS       | ./dearmor  | diff AUTHORS       -
	./armor    < Changes       | ./dearmor  | diff Changes       -
	./armor    < EXTERN        | ./dearmor  | diff EXTERN        -
	./armor    < LICENSE       | ./dearmor  | diff LICENSE       -
	./armor    < Makefile      | ./dearmor  | diff Makefile      -
	./armor    < README        | ./dearmor  | diff README        -
	./armor    < README.b00t   | ./dearmor  | diff README.b00t   -
	./armor    < TODO          | ./dearmor  | diff TODO          -
	./armor    < VERSION       | ./dearmor  | diff VERSION       -
	./armor    < addcr.c       | ./dearmor  | diff addcr.c       -
	./armor    < armor.c       | ./dearmor  | diff armor.c       -
	./armor    < b00t.c        | ./dearmor  | diff b00t.c        -
	./armor    < bootstrap     | ./dearmor  | diff bootstrap     -
	./armor    < ccinfo.c      | ./dearmor  | diff ccinfo.c      -
	./armor    < ckey.c        | ./dearmor  | diff ckey.c        -
	./armor    < conf-pdesc    | ./dearmor  | diff conf-pdesc    -
	./armor    < config.c      | ./dearmor  | diff config.c      -
	./armor    < config.db     | ./dearmor  | diff config.db     -
	./armor    < dearmor.c     | ./dearmor  | diff dearmor.c     -
	./armor    < delcr.c       | ./dearmor  | diff delcr.c       -
	./armor    < extract.c     | ./dearmor  | diff extract.c     -
	./armor    < forarg.c      | ./dearmor  | diff forarg.c      -
	./armor    < init.l1       | ./dearmor  | diff init.l1       -
	./armor    < mkarray.c     | ./dearmor  | diff mkarray.c     -
	./armor    < mkdep.c       | ./dearmor  | diff mkdep.c       -
	./armor    < mkstring.c    | ./dearmor  | diff mkstring.c    -
	./armor    < mksystype     | ./dearmor  | diff mksystype     -
	./armor    < retract.c     | ./dearmor  | diff retract.c     -
	./armor    < tree.c        | ./dearmor  | diff tree.c        -
	./armor    < tree.h        | ./dearmor  | diff tree.h        -
	./armor    < unmake.c      | ./dearmor  | diff unmake.c      -
	./armor    < vpath.l1      | ./dearmor  | diff vpath.l1      -
	./addcr    < makeutil.text | ./delcr    | diff makeutil.text -
	./addcr    < AUTHORS       | ./delcr    | diff AUTHORS       -
	./addcr    < Changes       | ./delcr    | diff Changes       -
	./addcr    < EXTERN        | ./delcr    | diff EXTERN        -
	./addcr    < LICENSE       | ./delcr    | diff LICENSE       -
	./addcr    < Makefile      | ./delcr    | diff Makefile      -
	./addcr    < README        | ./delcr    | diff README        -
	./addcr    < README.b00t   | ./delcr    | diff README.b00t   -
	./addcr    < TODO          | ./delcr    | diff TODO          -
	./addcr    < VERSION       | ./delcr    | diff VERSION       -
	./addcr    < addcr.c       | ./delcr    | diff addcr.c       -
	./addcr    < armor.c       | ./delcr    | diff armor.c       -
	./addcr    < b00t.c        | ./delcr    | diff b00t.c        -
	./addcr    < bootstrap     | ./delcr    | diff bootstrap     -
	./addcr    < ccinfo.c      | ./delcr    | diff ccinfo.c      -
	./addcr    < ckey.c        | ./delcr    | diff ckey.c        -
	./addcr    < conf-pdesc    | ./delcr    | diff conf-pdesc    -
	./addcr    < config.c      | ./delcr    | diff config.c      -
	./addcr    < config.db     | ./delcr    | diff config.db     -
	./addcr    < dearmor.c     | ./delcr    | diff dearmor.c     -
	./addcr    < delcr.c       | ./delcr    | diff delcr.c       -
	./addcr    < extract.c     | ./delcr    | diff extract.c     -
	./addcr    < forarg.c      | ./delcr    | diff forarg.c      -
	./addcr    < init.l1       | ./delcr    | diff init.l1       -
	./addcr    < mkarray.c     | ./delcr    | diff mkarray.c     -
	./addcr    < mkdep.c       | ./delcr    | diff mkdep.c       -
	./addcr    < mkstring.c    | ./delcr    | diff mkstring.c    -
	./addcr    < mksystype     | ./delcr    | diff mksystype     -
	./addcr    < retract.c     | ./delcr    | diff retract.c     -
	./addcr    < tree.c        | ./delcr    | diff tree.c        -
	./addcr    < tree.h        | ./delcr    | diff tree.h        -
	./addcr    < unmake.c      | ./delcr    | diff unmake.c      -
	./addcr    < vpath.l1      | ./delcr    | diff vpath.l1      -
	./textpack < makeutil.text | ./textpand | diff makeutil.text -
	./textpack < AUTHORS       | ./textpand | diff AUTHORS       -
	./textpack < Changes       | ./textpand | diff Changes       -
	./textpack < EXTERN        | ./textpand | diff EXTERN        -
	./textpack < LICENSE       | ./textpand | diff LICENSE       -
	./textpack < Makefile      | ./textpand | diff Makefile      -
	./textpack < README        | ./textpand | diff README        -
	./textpack < README.b00t   | ./textpand | diff README.b00t   -
	./textpack < TODO          | ./textpand | diff TODO          -
	./textpack < VERSION       | ./textpand | diff VERSION       -
	./textpack < addcr.c       | ./textpand | diff addcr.c       -
	./textpack < armor.c       | ./textpand | diff armor.c       -
	./textpack < b00t.c        | ./textpand | diff b00t.c        -
	./textpack < bootstrap     | ./textpand | diff bootstrap     -
	./textpack < ccinfo.c      | ./textpand | diff ccinfo.c      -
	./textpack < ckey.c        | ./textpand | diff ckey.c        -
	./textpack < conf-pdesc    | ./textpand | diff conf-pdesc    -
	./textpack < config.c      | ./textpand | diff config.c      -
	./textpack < config.db     | ./textpand | diff config.db     -
	./textpack < dearmor.c     | ./textpand | diff dearmor.c     -
	./textpack < delcr.c       | ./textpand | diff delcr.c       -
	./textpack < extract.c     | ./textpand | diff extract.c     -
	./textpack < forarg.c      | ./textpand | diff forarg.c      -
	./textpack < init.l1       | ./textpand | diff init.l1       -
	./textpack < mkarray.c     | ./textpand | diff mkarray.c     -
	./textpack < mkdep.c       | ./textpand | diff mkdep.c       -
	./textpack < mkstring.c    | ./textpand | diff mkstring.c    -
	./textpack < mksystype     | ./textpand | diff mksystype     -
	./textpack < retract.c     | ./textpand | diff retract.c     -
	./textpack < textpack.c    | ./textpand | diff textpack.c    -
	./textpack < textpand.c    | ./textpand | diff textpand.c    -
	./textpack < tree.c        | ./textpand | diff tree.c        -
	./textpack < tree.h        | ./textpand | diff tree.h        -
	./textpack < unmake.c      | ./textpand | diff unmake.c      -
	./textpack < vpath.l1      | ./textpand | diff vpath.l1      -
	./testarray                             | diff testarray.c   -
	./mkdep    < testdep.c                  | diff testdep       -
	./teststring > test.out
	head -1 conf-pdesc                      | diff test.out      -
	rm -f test.out
	./config -Itest.db test.out
	diff test.bl test.out
	rm -f test.out
	./unmake   < Makefile                   | diff Makefile      -


#
# create makeutil-$VERSION.{tgz,tbz2,text}, MD5, SHA1
#
dist: default
	install -m777 -d                makeutil-`head -1 VERSION`
	install -m666 -c AUTHORS        makeutil-`head -1 VERSION`
	install -m666 -c Changes        makeutil-`head -1 VERSION`
	install -m666 -c EXTERN         makeutil-`head -1 VERSION`
	install -m666 -c LICENSE        makeutil-`head -1 VERSION`
	install -m666 -c Makefile       makeutil-`head -1 VERSION`
	install -m666 -c README         makeutil-`head -1 VERSION`
	install -m666 -c README.b00t    makeutil-`head -1 VERSION`
	install -m666 -c TODO           makeutil-`head -1 VERSION`
	install -m666 -c VERSION        makeutil-`head -1 VERSION`
	install -m666 -c addcr.c        makeutil-`head -1 VERSION`
	install -m666 -c armor.c        makeutil-`head -1 VERSION`
	install -m666 -c b00t.c         makeutil-`head -1 VERSION`
	install -m666 -c bootstrap      makeutil-`head -1 VERSION`
	install -m666 -c ccinfo.c       makeutil-`head -1 VERSION`
	install -m666 -c ckey.c         makeutil-`head -1 VERSION`
	install -m666 -c conf-pdesc     makeutil-`head -1 VERSION`
	install -m666 -c config.c       makeutil-`head -1 VERSION`
	install -m666 -c config.db      makeutil-`head -1 VERSION`
	install -m666 -c dearmor.c      makeutil-`head -1 VERSION`
	install -m666 -c delcr.c        makeutil-`head -1 VERSION`
	install -m666 -c extract.c      makeutil-`head -1 VERSION`
	install -m666 -c forarg.c       makeutil-`head -1 VERSION`
	install -m666 -c init.l1        makeutil-`head -1 VERSION`
	install -m666 -c mkarray.c      makeutil-`head -1 VERSION`
	install -m666 -c mkdep.c        makeutil-`head -1 VERSION`
	install -m666 -c mkstring.c     makeutil-`head -1 VERSION`
	install -m666 -c mksystype      makeutil-`head -1 VERSION`
	install -m666 -c retract.c      makeutil-`head -1 VERSION`
	install -m666 -c test.bl        makeutil-`head -1 VERSION`
	install -m666 -c test.db        makeutil-`head -1 VERSION`
	install -m666 -c test.l1        makeutil-`head -1 VERSION`
	install -m666 -c testarray.c    makeutil-`head -1 VERSION`
	install -m666 -c testdep.c      makeutil-`head -1 VERSION`
	install -m666 -c testdep        makeutil-`head -1 VERSION`
	install -m666 -c teststring.c   makeutil-`head -1 VERSION`
	install -m666 -c textpack.c     makeutil-`head -1 VERSION`
	install -m666 -c textpand.c     makeutil-`head -1 VERSION`
	install -m666 -c tree.c         makeutil-`head -1 VERSION`
	install -m666 -c tree.h         makeutil-`head -1 VERSION`
	install -m666 -c unmake.c       makeutil-`head -1 VERSION`
	install -m666 -c vpath.l1       makeutil-`head -1 VERSION`
	tar cf - makeutil-`head -1 VERSION` \
	> makeutil-`head -1 VERSION`.tar \
	|| (rm -f makeutil-`head -1 VERSION`.tar && exit 1)
	GZIP= gzip -c makeutil-`head -1 VERSION`.tar \
	> makeutil-`head -1 VERSION`.tgz \
	|| (rm -f makeutil-`head -1 VERSION`.tgz && exit 1)
	BZIP= BZIP2= bzip2 -c makeutil-`head -1 VERSION`.tar \
	> makeutil-`head -1 VERSION`.tbz2 \
	|| (rm -f makeutil-`head -1 VERSION`.tbz2 && exit 1)
	rm -f makeutil-`head -1 VERSION`.tar
	rm -rf makeutil-`head -1 VERSION`
	install -m666 -c makeutil.text  makeutil-`head -1 VERSION`.text
	BZIP= BZIP2= bzip2 -c makeutil-`head -1 VERSION`.text \
	> makeutil-`head -1 VERSION`.text.bz2
	rm -f makeutil-`head -1 VERSION`.text
	openssl md5  makeutil-`head -1 VERSION`.* > MD5 \
	|| (rm -f MD5  && exit 1)
	openssl sha1 makeutil-`head -1 VERSION`.* > SHA1 \
	|| (rm -f SHA1 && exit 1)

install: default
	install -c addcr    $(prefix)/bin
	install -c armor    $(prefix)/bin
	install -c b00t     $(prefix)/bin
	install -c config   $(prefix)/bin
	install -c dearmor  $(prefix)/bin
	install -c delcr    $(prefix)/bin
	install -c extract  $(prefix)/bin
	install -c mkarray  $(prefix)/bin
	install -c mkdep    $(prefix)/bin
	install -c mkstring $(prefix)/bin
	install -c retract  $(prefix)/bin
	install -c textpack $(prefix)/bin
	install -c textpand $(prefix)/bin
	install -c unmake   $(prefix)/bin

uninstall:
	rm -f $(prefix)/bin/addcr
	rm -f $(prefix)/bin/armor
	rm -f $(prefix)/bin/b00t
	rm -f $(prefix)/bin/config
	rm -f $(prefix)/bin/dearmor
	rm -f $(prefix)/bin/delcr
	rm -f $(prefix)/bin/extract
	rm -f $(prefix)/bin/mkarray
	rm -f $(prefix)/bin/mkdep
	rm -f $(prefix)/bin/mkstring
	rm -f $(prefix)/bin/retract
	rm -f $(prefix)/bin/textpack
	rm -f $(prefix)/bin/textpand
	rm -f $(prefix)/bin/unmake

clean:
	rm -f addcr
	rm -f armor
	rm -f b00t
	rm -f ccinfo
	rm -f ckey
	rm -f config
	rm -f dearmor
	rm -f delcr
	rm -f extract
	rm -f mkarray
	rm -f mkdep
	rm -f mkstring
	rm -f retract
	rm -f systype
	rm -f testarray
	rm -f teststring
	rm -f textpack
	rm -f textpand
	rm -f unmake
	rm -f makeutil.text
	rm -f addcr.o
	rm -f armor.o
	rm -f b00t.o
	rm -f ccinfo.o
	rm -f ckey.o
	rm -f config.o
	rm -f dearmor.o
	rm -f delcr.o
	rm -f extract.o
	rm -f mkarray.o
	rm -f mkdep.o
	rm -f mkstring.o
	rm -f retract.o
	rm -f test0.o
	rm -f test0.c
	rm -f test1.o
	rm -f test1.c
	rm -f testarray.o
	rm -f teststring.o
	rm -f textpack.o
	rm -f textpand.o
	rm -f unmake.o
	rm -f forarg.o
	rm -f freq.d
	rm -f freq.c
	rm -f freq.o
	rm -f tree.o
	rm -f makeutil-`head -1 VERSION`.text.bz2
	rm -f makeutil-`head -1 VERSION`.tgz
	rm -f makeutil-`head -1 VERSION`.tbz2
	rm -f makeutil-`head -1 VERSION`.zip
	rm -f MD5
	rm -f SHA1

#                                                               ..__
#                                                               `' "

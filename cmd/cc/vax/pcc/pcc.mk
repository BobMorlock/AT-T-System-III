TESTDIR = .
INS = /etc/install
INSDIR =
FRC =
CFLAGS = -O
INSTALL=ccom
FINSTALL=f1
M=/usr/src/cmd/cc/vax/mip
all: comp fort
comp: rodata.o cgram.o xdefs.o scan.o pftn.o trees.o optim.o code.o local.o \
	reader.o local2.o order.o match.o allo.o comm1.o table.o
	$(CC) $(LDFLAGS) -s -i rodata.o cgram.o xdefs.o scan.o pftn.o trees.o \
		optim.o code.o local.o reader.o local2.o order.o match.o \
		allo.o comm1.o table.o -o $(TESTDIR)/comp
trees.o: $M/manifest macdefs $M/mfile1 $M/trees.c
	$(CC) -c $(CFLAGS) -I$M -I.  $M/trees.c
optim.o: $M/manifest macdefs $M/mfile1 $M/optim.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/optim.c
pftn.o: $M/manifest macdefs $M/mfile1 $M/pftn.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/pftn.c
code.o: $M/manifest macdefs $M/mfile1
	$(CC) -c $(CFLAGS) -I$M -I. code.c
local.o: $M/manifest macdefs $M/mfile1
	$(CC) -c $(CFLAGS) -I$M -I. local.c
scan.o: $M/manifest macdefs $M/mfile1 $M/scan.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/scan.c
xdefs.o: $M/manifest $M/mfile1 macdefs $M/xdefs.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/xdefs.c
cgram.o: $M/manifest $M/mfile1 macdefs $M/cgram.c
	$(CC) -c $(CFLAGS) -I. -I$M $M/cgram.c
rodata.o $M/cgram.c: $M/cgram.y
	$(YACC) $M/cgram.y
	rm -f rodata.c
	./:yyfix yyexca yyact yypact yypgo yyr1 yyr2 yychk yydef
	mv y.tab.c $M/cgram.c
	$(CC) $(CFLAGS) -S rodata.c
	./:rofix rodata.s
	as -o rodata.o rodata.s
	rm -f rodata.s rodata.c
comm1.o: $M/manifest $M/mfile1 $M/common macdefs $M/comm1.c
	$(CC) -c $(CFLAGS) -I. -I$M $M/comm1.c
table.o: $M/manifest $M/mfile2 mac2defs macdefs table.c
	$(CC) $(CFLAGS) -S -I$M -I. table.c
	./:rofix table.s
	as -o table.o table.s
	rm -f table.s
reader.o: $M/manifest $M/mfile2 mac2defs macdefs $M/reader.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/reader.c
local2.o: $M/manifest $M/mfile2 mac2defs macdefs
	$(CC) -c $(CFLAGS) -I$M -I. local2.c
order.o: $M/manifest $M/mfile2 mac2defs macdefs
	$(CC) -c $(CFLAGS) -I$M -I. order.c
match.o: $M/manifest $M/mfile2 mac2defs macdefs $M/match.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/match.c
allo.o: $M/manifest $M/mfile2 mac2defs macdefs $M/allo.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/allo.c
shrink:
	rm *.o comp
clean:
	rm -f *.o
clobber: clean
	rm -f $(TESTDIR)/fort $(TESTDIR)/comp
lintall:
	lint -hpv -I. -I$M  $M/cgram.c $M/xdefs.c $M/scan.c $M/pftn.c \
		$M/trees.c $M/optim.c code.c local.c $M/reader \
		local2.c order.c $M/match.c $M/allo.c $M/comm1.c table.c
fort: comp fort.o freader.o fallo.o fmatch.o ftable.o forder.o flocal2.o \
	fcomm2.o
	$(CC) -s -i $(CFLAGS) fort.o freader.o fallo.o fmatch.o ftable.o \
		forder.o flocal2.o fcomm2.o -o $(TESTDIR)/fort
fort.o: fort.h $M/fort.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/fort.c
freader.o: reader.o
	$(CC) -c $(CFLAGS) -I$M -I. $M/freader.c
fallo.o: allo.o
	$(CC) -c $(CFLAGS) -I$M -I. $M/fallo.c
fmatch.o: match.o
	$(CC) -c $(CFLAGS) -I$M -I. $M/fmatch.c
ftable.o: table.o
	$(CC) -S $(CFLAGS) -I$M -I. $M/ftable.c
	./:rofix ftable.s
	as -o ftable.o ftable.s
	rm -f ftable.s
forder.o: order.o
	$(CC) -c $(CFLAGS) -I$M -I. $M/forder.c
flocal2.o: local2.o
	$(CC) -c $(CFLAGS) -I$M -I. $M/flocal2.c
fcomm2.o: $M/common
	$(CC) -c $(CFLAGS) -I$M -I. $M/fcomm2.c
fort.o freader.o fallo.o fmatch.o ftable.o forder.o flocal2.o fcomm2.o: \
	$M/mfile2 $M/manifest macdefs mac2defs
install:	all
	ln $(TESTDIR)/comp $(INSTALL)
	$(INS) -i $(INSTALL) $(DESTDIR)/lib
	rm $(INSTALL)
	ln $(TESTDIR)/fort $(FINSTALL)
	$(INS) -i $(FINSTALL) $(DESTDIR)/lib
	rm $(FINSTALL)

#include "stdio.h"
#define	TSIZE	50		/* maximum configuration table size */
#define	DSIZE	50		/* maximum device table size */
#define	ASIZE	20		/* maximum alias table size */
#define MSIZE	100		/* maximum address map table size */
#define	PSIZE	32		/* maximum keyword table size */
#define	BSIZE	16		/* maximum block device table size */
#define	CSIZE	50		/* maximum character device table size */
#define ONCE	128		/* allow only one specification of device */
#define NOCNT	64		/* suppress device count field */
#define SUPP	32		/* suppress interrupt vector (on print) */
#define REQ	16		/* required device */
#define	BLOCK	8		/* block type device */
#define	CHAR	4		/* character type device */
#define	FLT	2		/* interrupt vector in range 0300 - 0774 */
#define	FIX	1		/* interrupt vector in range 0 - 0274 */
#define INIT	64		/* initialization routine exists */
#define PWR	32		/* power fail routine exists */
#define	OPEN	16		/* open routine exists */
#define	CLOSE	8		/* close routine exists */
#define	READ	4		/* read routine exists */
#define	WRITE	2		/* write routine exists */
#define	IOCTL	1		/* ioctl routine exists */

struct	t	{
	char	*devname;	/* pointer to device name */
	short	vector;		/* interrupt vector location */
	unsigned short address;	/* device address */
	short	buslevel;	/* bus request level */
	short	addrsize;	/* number of bytes at device address */
	short	vectsize;	/* number of bytes for interrupt vector */
	short	type;		/* ONCE,NOCNT,SUPP,REQ,BLOCK,CHAR,FLT,FIX  */
	char	*handler;	/* pointer to interrupt handler */
	short	count;		/* sequence number for this device */
	short	blk;		/* major device number if block type device */
	short	chr;		/* major device number if char. type device */
	short	mlt;		/* number of devices on controller */
}	table	[TSIZE];

struct	t2	{
	char	dev[9];		/* device name */
	short	vsize;		/* interrupt vector size */
	short	asize;		/* device address size */
	short	type2;		/* ONCE,NOCNT,SUPP,REQ,BLOCK,CHAR,FLT,FIX  */
	short	block;		/* major device number if block type device */
	short	charr;		/* major device number if char. type device */
	short	mult;		/* maximum number of devices per controller */
	short	busmax;		/* maximum allowable bus request level */
	short	mask;		/* device mask indicating existing handlers */
	char	hndlr[5];	/* handler name */
	short	dcount;		/* number of controllers present */
	char	entries[3][9];	/* conf. structure definitions */
	short	acount;		/* number of devices */
}	devinfo	[DSIZE];

struct	t3	{
	char	new[9];		/* alias of device */
	char	old[9];		/* reference name of device */
}	alias	[ASIZE];

struct	map	{
	unsigned short start;	/* starting unibus address */
	short	length;		/* gap length */
}	map[MSIZE] =
{
	0160000, 017570
};

struct	t4	{
	char	indef[21];		/* input parameter keyword */
	char	oudef[21];		/* output definition symbol */
	char	value[21];		/* actual parameter value */
	char	defval[21];		/* default parameter value */
}	parms	[PSIZE];

struct	t	*lowptr[128];		/* low core double-word pointers */
struct	t2	*bdevices[BSIZE];	/* pointers to block devices */
struct	t2	*cdevices[CSIZE];	/* pointers to char. devices */
struct	t	*p;			/* configuration table pointer */
struct	t2	*q;			/* master device table pointer */
struct	t3	*r;			/* alias table pointer */
struct	t4	*kwdptr;		/* keyword table pointer */
struct	t	*locate();
struct	t2	*find();
struct	t4	*lookup();
char	*uppermap();

short	power	= 0;	/* power fail restart flag */
short	eflag	= 0;	/* error in configuration */
short	tflag	= 0;	/* table flag */
short	bmax	= -1;	/* dynamic max. major device number for block device */
short	cmax	= -1;	/* dynamic max. major device number for char. device */
short	blockex	= 0;	/* dynamic end of block device table */
short	charex	= 0;	/* dynamic end of character device table */
short	subv	= 0;	/* low core double-word subscript */
short	subtmp	= 0;	/* temporary; for subscripting */
short	abound	= -1;	/* current alias table size */
short	dbound	= -1;	/* current device table size */
short	tbound	= -1;	/* current configuration table size */
short	pbound	= -1;	/* current keyword table size */
short	mbound	= 0;	/* current address map table size */
short	rtmaj	= -1;	/* major device number for root device */
short	swpmaj	= -1;	/* major device number for swap device */
short	pipmaj	= -1;	/* major device number for pipe device */
short	dmpmaj	= -1;	/* major device number for dump device */
short	rtmin	= -1;	/* minor device number for root device */
short	swpmin	= -1;	/* minor device number for swap device */
short	pipmin	= -1;	/* minor device number for pipe device */
short	dmpmin	= -1;	/* minor device number for dump device */
long	swplo	= -1;	/* low disc address for swap area */
short	nswap	= -1;	/* number of disc blocks in swap area */
FILE	*fdconf;	/* configuration file descriptor */
FILE	*fdlow;		/* low core file descriptor */
char	*dmphndlr;	/* pointer to dump handler routine */
char	*mfile;		/* master device file */
char	*cfile;		/* output configuration table file */
char	*lfile;		/* output low core file */
char	*infile;	/* input configuration file */
char	line[101];	/* input line buffer area */
char	dump[81];	/* trap filler for dump routine */
char	pwrbuff[513];	/* power fail routine buffer */
char	initbuff[513];	/* initialization routine buffer */
char	xbuff[101];	/* buffer for external symbol definitions */
char	d[9];		/* buffer area */
char	capitals[9];	/* buffer area */
short	vec;		/* specified interrupt vector */
unsigned short add;	/* specified device address */
short	bl;		/* specified bus level */
short	ml;		/* specified device multiplicity */

char	*opterror =		/* option error message */
{
	"Option re-specification\n"
};

char	*sdev[] = {
	"dm11",
	"hisdm",
	NULL
};

main(argc,argv) 
int argc;
char *argv[];
{
	register i, l;
	register FILE *fd;
	int number;
	char input[21], buff[9], c, symbol[21];
	char *argv2;
	struct t *pt;
	argc--;
	argv++;
	argv2 = argv[0];
	argv++;
/*
 * Scan off the 'c', 'l', 'm', and 't' options.
 */
	while ((argc > 1) && (argv2[0] == '-')) {
		for (i=1; (c=argv2[i]) != NULL; i++) {
			switch (c) {
/*
 * Output configuration file specification
 */
			case 'c':	if (cfile) {
						printf (opterror);
						exit(1);
					}
					cfile = argv[0];
					argv++;
					argc--;
					break;
/*
 * Output univec file specification
 */
			case 'l':	if (lfile) {
						printf (opterror);
						exit(1);
					}
					lfile = argv[0];
					argv++;
					argc--;
					break;
/*
 * Input master table specification
 */
			case 'm':	if (mfile) {
						printf (opterror);
						exit(1);
					}
					mfile = argv[0];
					argv++;
					argc--;
					break;
/*
 * Table flag
 */
			case 't':	tflag++;
					break;

			default:	printf ("Unknown option\n");
					exit(1);
			}
		}
		argc--;
		argv2 = argv[0];
		argv++;
	}
	if (argc != 1) {
		printf ("Usage: config [-t][-l file][-c file][-m file] file\n");
		exit(1);
	}
/*
 * Set up defaults for unspecified files.
 */
	if (cfile == 0)
		cfile = "conf.c";
	if (lfile == 0)
		lfile = "univec.c";
	if (mfile == 0)
		mfile = "/etc/master";
	infile = argv2;
	fd = fopen (infile,"r");
	if (fd == NULL) {
		printf ("Open error for file -- %s\n",infile);
		exit(1);
	}
/*
 * Open configuration file and set modes.
 */
	fdconf = fopen (cfile,"w");
	if (fdconf == NULL) {
		printf ("Open error for file -- %s\n",cfile);
		exit(1);
	}
	chmod (cfile,0644);
/*
 * Print configuration file heading.
 */
	fprintf (fdconf,"/*\n *  Configuration information\n */\n\n\n");
	p = table;
/*
 * Read in the master device table and the alias table.
 */
	file();
/*
 * Start scanning the input.
 */
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '*')
			continue;
		l = sscanf(line,"%20s",input);
		if (l == 0) {
			error ("Incorrect line format");
			continue;
		}
		if (equal(input,"root")) {
/*
 * Root device specification
 */
			l = sscanf(line,"%*8s%8s%o",buff,&number);
			if (l != 2) {
				error ("Incorrect line format");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				error ("No such device");
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error ("Not a block device");
				continue;
			}
			if (number > 255) {
				error ("Invalid minor device number");
				continue;
			}
			if (rtmin >= 0) {
				error ("Root re-specification");
				continue;
			}
			rtmin = number;
			rtmaj = pt->blk;
		}
		else
		    if (equal(input,"swap")) {
/*
 * Swap device specification
 */
			l = sscanf(line,"%*8s%8s%o%ld%hd",
					buff,&number,
					&swplo,&nswap);
			if (l != 4) {
				error ("Incorrect line format");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				error ("No such device");
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error ("Not a block device");
				continue;
			}
			if (number > 255) {
				error ("Invalid minor device number");
				continue;
			}
			if (swpmin >= 0) {
				error ("Swap re-specification");
				continue;
			}
			if (nswap < 1) {
				error ("Invalid nswap");
				continue;
			}
			if (swplo < 0) {
				error ("Invalid swplo");
				continue;
			}
			swpmin = number;
			swpmaj = pt->blk;
		}
		else
		    if (equal(input,"pipe")) {
/*
 * Pipe device specification
 */
			l = sscanf(line,"%*8s%8s%o",buff,&number);
			if (l != 2) {
				error ("Incorrect line format");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				error ("No such device");
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error ("Not a block device");
				continue;
			}
			if (number > 255) {
				error ("Invalid minor device number");
				continue;
			}
			if (pipmin >= 0) {
				error ("Pipe re-specification");
				continue;
			}
			pipmin = number;
			pipmaj = pt->blk;
		}
		else
		    if (equal(input,"dump")) {
/*
 * Dump device specification
 */
			l = sscanf(line,"%*8s%8s%o",buff,&number);
			if (l != 2) {
				error ("Incorrect line format");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				error ("No such device");
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error ("Not a block device");
				continue;
			}
			if (number > 255) {
				error ("Invalid minor device number");
				continue;
			}
			if (dmpmin >= 0) {
				error ("Dump re-specification");
				continue;
			}
			dmpmin = number;
			dmpmaj = pt->blk;
			dmphndlr = pt->handler;
		}
		else {
/*
 * Device or parameter specification other than root, swap, pipe, or dump.
 * If power fail recovery is specified, remember it.
 */
			kwdptr = lookup(input);
			if (kwdptr) {
				l = sscanf(line,"%20s%20s",input,symbol);
				if (l != 2) {
					error ("Incorrect line format");
					continue;
				}
				if (strlen(kwdptr->value)) {
					error ("Parameter re-specification");
					continue;
				}
				if (equal(input,"power") && equal(symbol,"1"))
					power++;
				strcpy(kwdptr->value,symbol);
				continue;
			}
			l = sscanf(line,"%8s%ho%ho%hd%hd",d,&vec,&add,&bl,&ml);
			if (l < 4) {
				error ("Incorrect line format");
				continue;
			}
/*
 * Does such a device exist, and if so, do we allow its specification?
 */
			if ((q=find(d)) == 0) {
				error ("No such device");
				continue;
			}
			if ((q->type2 & ONCE) && (locate(d))) {
				error ("Only one specification allowed");
				continue;
			}
/*
 * Is the address valid?
 */
			if (q->asize) {
				if (add % 2) {
					error ("Address alignment");
					continue;
				}
				if (add < 0160000) {
					error ("Invalid address");
					continue;
				}
				if (addrcheck(add,q->asize)) {
					error ("Address collision");
					continue;
				}
			}
			else {
				if (add) {
					error ("Address not null");
					continue;
				}
			}
/*
 * If the vector size field is non-zero, process as a regular device.
 */
			if (q->vsize) {
/*
 * Is the interrupt vector on an appropriate boundary?
 */
				if (vec%q->vsize) {
					error ("Vector alignment");
					continue;
				}
/*
 * Is the interrupt vector in low core?
 */
				if (vec<0 || vec>0774) {
					error ("Vector not in low core range");
					continue;
				}
				switch (q->type2 & (FIX|FLT)) {
/*
 * This interrupt vector should be in the 0 - 0274 range.
 */
				case FIX:
					if (vec > 0274) {
						error ("Vector not in 0-0274 range");
						continue;
					}
					break;
/*
 * This interrupt vector should be in the 0300 - 0774 range.
 */
				case FLT:
					if (vec < 0300) {
						error ("Vector not in 0300-0774 range");
						continue;
					}
					break;
				}
/*
 * Is the bus request level a valid one for this device?
 */
				if (bl<4 || bl>q->busmax) {
					error ("Invalid bus level");
					continue;
				}
/*
 * Get the double-word offset for the interrupt vector.
 */
				subv = vec/4;
/*
 * Make sure the interrupt vector is free.
 */
				if (lowptr[subv]) {
					error ("Vector allocated");
					continue;
				}
				if (q->vsize == 8) {
					if (lowptr[subv+1]) {
						error ("Vector collision");
						continue;
					}
				}
			}
/*
 * Device is not interrupt driven, so interrupt vector and
   bus request levels should be zero.
 */
			else {
				if (vec) {
					error ("Interrupt vector not null");
					continue;
				}
				if (bl) {
					error ("Bus level not null");
					continue;
				}
				subv = 0;
			}
/*
 * See if the optional device multiplier was specified, and if so, see
 * if it is a valid one.
 */
			if (l == 5) {
				if ((ml > q->mult) || (ml < 1)) {
					error ("Invalid device multiplier");
					continue;
				}
			}
/*
 * Multiplier not specified, so take a default value from master device table.
 */
			else
				ml = q->mult;
/*
 * Fill in the contents of the configuration table node for this device.
 */
			enterdev(vec,add,bl,ml,d);
		}
	}
/*
 * Make sure that the root, swap, pipe and dump devices were specified.
 * Set up default values for tunable things where applicable.
 */
	if (rtmaj < 0) {
		printf ("root device not specified\n");
		eflag++;
	}
	if (swpmaj < 0) {
		printf ("swap device not specified\n");
		eflag++;
	}
	if (pipmaj < 0) {
		printf ("pipe device not specified\n");
		eflag++;
	}
	if (dmpmaj < 0) {
		printf ("dump device not specified\n");
		eflag++;
	}
	for (kwdptr=parms; kwdptr<= &parms[pbound]; kwdptr++) {
		if (strlen(kwdptr->value) == 0) {
			if (strlen(kwdptr->defval) == 0) {
				printf ("%s not specified\n",kwdptr->indef);
				eflag++;
			}
			strcpy(kwdptr->value,kwdptr->defval);
		}
	}
/*
 * See if configuration is to be terminated.
 */
	if (eflag) {
		printf ("\nConfiguration aborted.\n");
		exit (1);
	}
/*
 * Configuration is okay, so write the two files and quit.
 */
	for (q=devinfo; q<= &devinfo[dbound]; q++) {
		if (q->type2 & REQ) {
			if (!(locate(q->dev))) {
				enterdev(0,0,0,q->mult,q->dev);
			}
		}
	}

	prtconf();
	prtlow();
	exit(0);
}

/*
 * This routine writes out the univec program.
 */
prtlow()
{
	register int i;
	register struct t *ptr;
	register struct t *ktemp;
/*
 * Open univec file and set modes.
 */
	fdlow = fopen (lfile,"w");
	if (fdlow == NULL) {
		printf ("Open error for file -- %s\n",lfile);
		exit(1);
	}
	chmod (lfile,0644);
/*
 * Print some headings.
 */
	fprintf (fdlow,"/* Unibus vector table */\n\n");
	fprintf (fdlow,"#define	NULL	(int *)0\n");
	fprintf (fdlow,"#define	D0	0x00000000\n");
	fprintf (fdlow,"#define	D1	0x08000000\n");
	fprintf (fdlow,"#define	D2	0x10000000\n");
	fprintf (fdlow,"#define	D3	0x18000000\n");
	fprintf (fdlow,"#define	D4	0x20000000\n");
	fprintf (fdlow,"#define	D5	0x28000000\n");
	fprintf (fdlow,"#define	D6	0x30000000\n");
	fprintf (fdlow,"#define	D7	0x38000000\n");
	fprintf (fdlow,"\n/* Interrupt Service Routine addresses */\n");
/*
 * Go through configuration table and print out all required handler references.
 * For devices that are not interrupt driven, and those that have the
 * SUPP bit on, the printing of the handler name is suppressed.
 */
	for (ptr= table; ptr->devname!=0; ptr++) {
		if (ptr->count == 0) {
			if ((ptr->type & SUPP) || (ptr->vectsize == 0))
				continue;
			if (ptr->vectsize == 4) {
				fprintf (fdlow,"extern %sintr();\n",
					ptr->handler);
			}
			else {
				fprintf (fdlow,"extern %srint(), %sxint();\n",
					ptr->handler,
					ptr->handler);
			}
		}
	}
/*
 * Go through low core in double-word increments.
 */
	fprintf (fdlow,"int *UNIvec[128] = {\n");
	for (i=0; i<128; i++) {
		ktemp = lowptr[i];
/*
 * If the pointer is 0, set vector up for stray interrupt catcher.
 */
		if (ktemp == 0)
			fprintf (fdlow,"\tNULL,\n");
		else {
			fprintf (fdlow,"/* %o */\n",i*4);
/*
 * Pointer refers to a device, so set up the handler routine address with
 * the bus level and device sequence number, unless SUPP bit is on.
 */
			ptr = ktemp;
			if (ptr->vectsize == 8) {
				if (ptr->type & SUPP) {
					fprintf (fdlow,"\tNULL,\n");
					fprintf (fdlow,"\tNULL,\n");
				}
				else {
					fprintf (fdlow,"\t(int *)((int)%srint+D%d),\n",
						ptr->handler,
						ptr->count);
					fprintf (fdlow,"\t(int *)((int)%sxint+D%d),\n",
						ptr->handler,
						ptr->count);
				}
				i++;
			}
			else {
				if (ptr->type & SUPP) {
					fprintf (fdlow,"\tNULL,\n");
				}
				else {
					fprintf (fdlow,"\t(int *)((int)%sintr+D%d),\n",
						ptr->handler,
						ptr->count);
				}
			}
		}
	}
	fprintf (fdlow,"};\n");
}

/*
 * This routine is used to read in the master device table and the
 * alias table.  In the master device file, these two tables are separated
 * by a line that contains an asterisk in column 1.  
 */
file() 
{
	register l;
	register FILE *fd;
	fd = fopen(mfile,"r");
	if (fd == NULL) {
		printf ("Open error for file -- %s\n",mfile);
		exit(1);
	}
	q = devinfo;
	
	while (fgets(line,100,fd) != NULL) {
/*
 * Check for the delimiter that indicates the beginning of the
 * alias table entries.
 */
		if (line[0] == '$')
			break;
/*
 * Check for comment.
 */
		if (line[0] == '*')
			continue;
		dbound++;
		if (dbound == DSIZE) {
			printf ("Device table overflow\n");
			exit(1);
		}
		l = sscanf(line,"%8s%hd%ho%ho%4s%hd%hd%hd%hd%hd%8s%8s%8s",
			q->dev,&(q->vsize),&(q->mask),&(q->type2),
			q->hndlr,&(q->asize),&(q->block),
			&(q->charr),&(q->mult),&(q->busmax),
			q->entries[0],q->entries[1],q->entries[2]);
		if (l < 10) {
			printf ("%s\nDevice parameter count\n",line);
			exit(1);
		}
		if (q->type2 & REQ) {
			if (q->vsize || q->asize || q->busmax) {
				printf ("%s\nParameter inconsistency\n",line);
				exit(1);
			}
		}
/*
 * Update the ends of the block and character device tables.
 */
		if (q->type2 & BLOCK)
			if ((blockex = max(blockex,q->block)) >= BSIZE) {
				printf ("%s\nBad major device number\n",line);
				exit(1);
			}
		if (q->type2 & CHAR)
			if ((charex = max(charex,q->charr)) >= CSIZE) {
				printf ("%s\nBad major device number\n",line);
				exit(1);
			}
		q++;
	}
	r = alias;
	while (fgets(line,100,fd) != NULL) {
/*
 * Check for the delimiter that indicates the beginning of the
 * keyword table entries.
 */
		if (line[0] == '$')
			break;
/*
 * Check for comment.
 */
		if (line[0] == '*')
			continue;
		abound++;
		if (abound == ASIZE) {
			printf ("Alias table overflow\n");
			exit(1);
		}
		l = sscanf(line,"%8s%8s",r->new,r->old);
		if (l < 2) {
			printf ("%s\nAlias parameter count\n",line);
			exit(1);
		}
		r++;
	}
	kwdptr = parms;
	while (fgets(line,100,fd) != NULL) {
/*
 * Check for comment.
 */
		if (line[0] == '*')
			continue;
		pbound++;
		if (pbound == PSIZE) {
			printf ("Keyword table overflow\n");
			exit(1);
		}
		l = sscanf(line,"%20s%20s%20s",kwdptr->indef,
			kwdptr->oudef,kwdptr->defval);
		if (l < 2) {
			printf ("%s\nTunable parameter count\n",line);
			exit(1);
		}
		if (l == 2)
			*(kwdptr->defval) = NULL;
		kwdptr++;
	}
	return;
}

/*
 * This routine compares two null terminated strings for equality.
 */
equal(s1,s2)
register char *s1, *s2;
{
	while (*s1++ == *s2) {
		if (*s2++ == 0)
			return(1);
	}
	return(0);
}

/*
 * This routine is used to print a configuration time error message
 * and then set a flag indicating a contaminated configuration.
 */
error(message)
char *message;
{
	printf ("%s%s\n\n",line,message);
	eflag++;
	return;
}

/*
 * This routine is used to search through the master device table for
 * some specified device.  If the device is found, we return a pointer to 
 * the device.  If the device is not found, we search the alias table for
 * this device.  If the device is not found in the alias table, we return a
 * zero.  If the device is found, we change its name to the reference name of
 * the device and re-initiate the search for this new name in the master
 * device table.
 */
struct t2 *
find(device)
char *device;
{
	register struct t2 *q;
	register struct t3 *r;
	for (;;) {
/*
 * Search the master device table for the specified device.
 */
		for (q=devinfo; q<= &devinfo[dbound]; q++) {
			if (equal(device,q->dev))
				return(q);
		}
/*
 * Since the device was not found in the master device table,
 * we now search through the alias table.
 */
		for (r=alias; r<= &alias[abound]; r++) {
			if (equal(device,r->new)) {
				device = r->old;
				break;
			}
		}
/*
 * See if the device was found in the alias table. If it wasn't, return a 0.
 */
		if (r > &alias[abound])
			return(0);
/*
 * It was found in the alias table, so we change its name back to the
 * reference name of the device and re-initiate the master device table search.
 */
	}
}

/*
 * This routine is used to set the character and/or block table pointers
 * to point to an entry of the master device table.
 */
setq()
{
	register struct t2 *ptr;
/*
 * Do a switch on the type of device.
 */
	switch (q->type2 & (BLOCK|CHAR)) {
/*
 * Device is not a block or character device.
 */
	case 0:	break;
/*
 * Device is a character type device.
 */
	case CHAR:
		subtmp = q->charr;
		ptr = cdevices[subtmp];
		if (ptr) {
			if (!equal(ptr->dev,q->dev)) {
				charex++;
				if (charex == CSIZE) {
					printf("Character table overflow\n");
					exit(1);
				}
				q->charr = subtmp = charex;
			}
		}
		cdevices[subtmp] = q;
		cmax = max (cmax,subtmp);
		break;
/*
 * Device is a block type device.
 */
	case BLOCK:
		subtmp = q->block;
		ptr = bdevices[subtmp];
		if (ptr) {
			if (!equal(ptr->dev,q->dev)) {
				blockex++;
				if (blockex == BSIZE) {
					printf ("Block table overflow\n");
					exit(1);
				}
				q->block = subtmp = blockex;
			}
		}
		bdevices[subtmp] = q;
		bmax = max (bmax,subtmp);
		break;
/*
 * Device is both a character and block type device.
 */
	case BLOCK|CHAR:
		subtmp = q->charr;
		ptr = cdevices[subtmp];
		if (ptr) {
			if (!equal(ptr->dev,q->dev)) {
				charex++;
				if (charex == CSIZE) {
					printf("Character table overflow\n");
					exit(1);
				}
				q->charr = subtmp = charex;
			}
		}
		cdevices[subtmp] = q;
		cmax = max (cmax,subtmp);
		subtmp = q->block;
		ptr = bdevices[subtmp];
		if (ptr) {
			if (!equal(ptr->dev,q->dev)) {
				blockex++;
				if (blockex == BSIZE) {
					printf ("Block table overflow\n");
					exit(1);
				}
				q->block = subtmp = blockex;
			}
		}
		bdevices[subtmp] = q;
		bmax = max (bmax,subtmp);
		break;
	}
	return;
}

/*
 * This routine writes out the configuration file (C program.)
 */
prtconf()
{
	register i, j, counter;
	short k;
	char *sname;
	struct t2 *ptr;
/*
 * Print some headings.
 */
	fprintf (fdconf,"\n");
	for (kwdptr=parms; kwdptr<= &parms[pbound]; kwdptr++) {
		fprintf (fdconf,"#define\t%s\t%s\n",
			kwdptr->oudef, kwdptr->value);
	}
	fprintf (fdconf,"\n#include\t\"sys/param.h\"\n");
	fprintf (fdconf,"#include\t\"sys/io.h\"\n");
	fprintf (fdconf,"#include\t\"sys/space.h\"\n");
	fprintf (fdconf,"#include\t\"sys/conf.h\"\n");
	fprintf (fdconf,"#include\t\"sys/uba.h\"\n\n");

/*
 * Print the extern statement for the nodev and nulldev entries.
 */
	fprintf (fdconf,"extern nodev(), nulldev();\n");

/*
 * Search the configuration table and generate an extern statement for
 * any routines that are needed.
 */
	for (p=table; p<= &table[tbound]; p++) {
		if (p->count)
			continue;
		switch (p->type & (BLOCK|CHAR)) {
		case CHAR:
			q = find(p->devname);
			sprintf (xbuff,"extern ");
			if (q->mask & OPEN) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"open(), ");
			}
			if (q->mask & CLOSE) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"close(), ");
			}
			if (q->mask & READ) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"read(), ");
			}
			if (q->mask & WRITE) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"write(), ");
			}
			if (q->mask & IOCTL) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"ioctl(), ");
			}
			if (q->mask & PWR)
				if (power) {
					strcat (xbuff,q->hndlr);
					strcat (xbuff,"clr(), ");
				}
			if (q->mask & INIT) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"init(), ");
			}
			xbuff[strlen(xbuff)-2] = ';';
			xbuff[strlen(xbuff)-1] = NULL;
			fprintf (fdconf,"%s\n",xbuff);
			break;
		case BLOCK:
			fprintf (fdconf,"extern %sopen(), %sclose(), %sstrategy();\n",
				p->handler,p->handler,p->handler);
			fprintf (fdconf,"extern struct iobuf %stab;\n", p->handler);
			break;
		case BLOCK|CHAR:
			q = find(p->devname);
			sprintf (xbuff,"extern %sopen(), %sclose(), ",
				q->hndlr,q->hndlr);
			if (q->mask & READ) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"read(), ");
			}
			if (q->mask & WRITE) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"write(), ");
			}
			if (q->mask & IOCTL) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"ioctl(), ");
			}
			if (q->mask & PWR)
				if (power) {
					strcat (xbuff,q->hndlr);
					strcat (xbuff,"clr(), ");
				}
			if (q->mask & INIT) {
				strcat (xbuff,q->hndlr);
				strcat (xbuff,"init(), ");
			}
			strcat (xbuff,q->hndlr);
			strcat (xbuff,"strategy();");
			fprintf (fdconf,"%s\n",xbuff);
			fprintf (fdconf,"extern struct iobuf %stab;\n", q->hndlr);
			break;
		}
	}

	fprintf (fdconf,"\nstruct bdevsw bdevsw[] = {\n");
/*
 * Go through block device table and indicate addresses of required routines.
 * If a particular device is not present, fill in "nodev" entries.
 */
	for (i=0; i<=bmax; i++) {
		ptr = bdevices[i];
		fprintf (fdconf,"/*%2d*/\t",i);
		if (ptr) {
			fprintf (fdconf,"%sopen,\t%sclose,\t%sstrategy,\t&%stab,\n",
				ptr->hndlr,
				ptr->hndlr,
				ptr->hndlr,
				ptr->hndlr);
		}
		else {
			fprintf (fdconf,"nodev, \tnodev, \tnodev, \t0, \n");
		}
	}
	fprintf (fdconf,"};\n\n");
	fprintf (fdconf,"struct cdevsw cdevsw[] = {\n");
/*
 * Go through character device table and indicate addresses of required
 * routines, or indicate "nulldev" if routine is not present.  If a 
 * particular device is not present, fill in "nodev" entries.
 */
	for (j=0; j<=cmax; j++) {
		ptr = cdevices[j];
		fprintf (fdconf,"/*%2d*/",j);
		if (ptr) {
			if (ptr->mask & OPEN)
				fprintf (fdconf,"\t%sopen,",ptr->hndlr);
			else
				fprintf (fdconf,"\tnulldev,");
			if (ptr->mask & CLOSE)
				fprintf (fdconf,"\t%sclose,",ptr->hndlr);
			else
				fprintf (fdconf,"\tnulldev,");
			if (ptr->mask & READ)
				fprintf (fdconf,"\t%sread,",ptr->hndlr);
			else
				fprintf (fdconf,"\tnodev, ");
			if (ptr->mask & WRITE)
				fprintf (fdconf,"\t%swrite,",ptr->hndlr);
			else
				fprintf (fdconf,"\tnodev, ");
			if (ptr->mask & IOCTL)
				fprintf (fdconf,"\t%sioctl,\n",ptr->hndlr);
			else
				fprintf (fdconf,"\tnodev, \n");
		}
		else
			fprintf (fdconf,"\tnodev, \tnodev, \tnodev, \tnodev, \tnodev,\n");
	}
/*
 * Print out block and character device counts, root, swap, and dump device
 * information, and the swplo, and nswap values.
 */
	fprintf (fdconf,"};\n\n");
	fprintf (fdconf,"int\tbdevcnt = %d;\nint\tcdevcnt = %d;\n\n",i,j);
	fprintf (fdconf,"dev_t\trootdev = makedev(%d, %d);\n",rtmaj,rtmin);
	fprintf (fdconf,"dev_t\tpipedev = makedev(%d, %d);\n",pipmaj,pipmin);
	fprintf (fdconf,"dev_t\tdumpdev = makedev(%d, %d);\n",dmpmaj,dmpmin);
	fprintf (fdconf,"dev_t\tswapdev = makedev(%d, %d);\n",swpmaj,swpmin);
	fprintf (fdconf,"daddr_t\tswplo = %lu;\nint\tnswap = %d;\n\n",swplo,nswap);
/*
 * Initialize the power fail and init handler buffers.
 */
	sprintf (pwrbuff,"\nint\t(*pwr_clr[])() = \n{\n");
	sprintf (initbuff,"\nint\t(*dev_init[])() = \n{\n");
/*
 * Go through the block device table, ignoring any zero entries.
 * Add power fail and init handler entries to the buffers as appropriate.
 */
	if (tflag)
		printf ("Block Devices\nmajor\tdevice\thandler\tcount\n");
	for (i=0; i<=bmax; i++) {
		if ((q=bdevices[i]) == 0)
			continue;
		if (power)
			if (q->mask & PWR)
				sprintf (&pwrbuff[strlen(pwrbuff)],"\t%sclr,\n",q->hndlr);
		if (q->mask & INIT)
				sprintf (&initbuff[strlen(initbuff)],"\t&%sinit,\n",q->hndlr);
		counter = 0;
/*
 * For each of these devices, go through the configuration table and
 * look for matches. For each match, print the device address, and keep
 * a count of the number of matches.
 */
		fprintf (fdconf,"\n");
		if (q->asize)
			fprintf (fdconf,"int\t%s_addr[] = {\n",q->hndlr);
		for (p=table; p<= &table[tbound]; p++) {
			if (equal(q->dev,p->devname)) {
				if (q->asize)
					fprintf(fdconf,"\tUBA_DEV+%.7o,\n",p->address);
				counter += p->mlt;
			}
		}
		if (q->asize)
			fprintf (fdconf,"};\n");
		if (!(q->type2 & NOCNT))
			fprintf (fdconf,"int\t%s_cnt = %d;\n",
				q->hndlr,counter);
		if (tflag)
			printf ("%2d\t%s\t%s\t%2d\n",i,q->dev,q->hndlr,counter);
		q->acount = counter;
/*
 * Print any required structure definitions.
 */
		for (j=0; j<3; j++) {
			if (*(q->entries[j]) != NULL)
				fprintf (fdconf,"struct\t%s\t%s_%s[%d];\n",
					q->entries[j],q->hndlr,
					q->entries[j],counter);
		}
	}
/*
 * Go through the character device table, ignoring any zero entries,
 * as well as those that are not strictly character devices.
 * Add power fail and init handler entries to the buffers as appropriate.
 */
	if (tflag)
		printf ("Character Devices\nmajor\tdevice\thandler\tcount\n");
	for (i=0; i<=cmax; i++) {
		if ((q=cdevices[i]) == 0)
			continue;
		if ((q->type2 & (BLOCK|CHAR)) != CHAR) {
			if (tflag)
				printf ("%2d\t%s\t%s\t%2d\n",i,q->dev,q->hndlr,q->acount);
			continue;
		}
		if (power)
			if (q->mask & PWR)
				sprintf (&pwrbuff[strlen(pwrbuff)],"\t%sclr,\n",q->hndlr);
		if (q->mask & INIT)
				sprintf (&initbuff[strlen(initbuff)],"\t&%sinit(),\n",q->hndlr);
		counter = 0;
/*
 * For each of these devices, go through the configuration table and
 * look for matches. For each match, print the device address, and keep
 * a count of the number of matches.
 */
		fprintf (fdconf,"\n");
		if (q->asize)
			fprintf (fdconf,"int\t%s_addr[] = {\n",q->hndlr);
		for (p=table; p<= &table[tbound]; p++) {
			if (equal(q->dev,p->devname)) {
				if (q->asize)
					fprintf(fdconf,"\tUBA_DEV+%.7o,\n",p->address);
				counter += p->mlt;
			}
		}
		if (q->asize)
			fprintf (fdconf,"};\n");
		if (!(q->type2 & NOCNT))
			fprintf (fdconf,"int\t%s_cnt = %d;\n",
				q->hndlr,counter);
		if (tflag)
			printf ("%2d\t%s\t%s\t%2d\n",i,q->dev,q->hndlr,counter);
/*
 * Print any required structure definitions.
 */
		for (j=0; j<3; j++) {
			if (*(q->entries[j]) != NULL)
				fprintf (fdconf,"struct\t%s\t%s_%s[%d];\n",
					q->entries[j],q->hndlr,
					q->entries[j],counter);
		}
	}
/*
 * Repeat this swill for the weirdo devices.
 */
	for (k=0; (sname=sdev[k]) != NULL; k++)
		if (locate(sname)) {
			q = find(sname);
			counter = 0;
			fprintf (fdconf,"\nint\t%s_addr[] = {\n",q->hndlr);
			for (p=table; p<= &table[tbound]; p++) {
				if (equal (p->devname,sname)) {
					fprintf (fdconf,"\tUBA_DEV+%.7o,\n",p->address);
					counter += p->mlt;
				}
			}
			fprintf (fdconf,"};\n");
			if (!(q->type2 & NOCNT))
				fprintf (fdconf,"int\t%s_cnt = %d;\n",
					q->hndlr,counter);
			for (j=0; j<3; j++) {
				if (*(q->entries[j]) != NULL)
					fprintf (fdconf,"struct\t%s\t%s_%s[%d];\n",
						q->entries[j],q->hndlr,
						q->entries[j],counter);
			}
		}
/*
 * Write out NULL entry into power fail and init buffers.
 * Then write the buffers out into the configuration file.
 */
	sprintf (&pwrbuff[strlen(pwrbuff)],"\t(int (*)())0\n};\n");
	sprintf (&initbuff[strlen(initbuff)],"\t(int (*)())0\n};\n");
	fprintf (fdconf,"%s",pwrbuff);
	fprintf (fdconf,"%s",initbuff);
	return;
}

max(a,b)
int a, b;
{
	return (a>b ? a:b);
}

/*
 * This routine is used to search the configuration table for some
 * specified device.  If the device is found we return a pointer to
 * that device.  If the device is not found, we search the alias
 * table for this device.  If the device is not found in the alias table
 * we return a zero.  If the device is found, we change its name to
 * the reference name of the device and re-initiate the search for this
 * new name in the configuration table.
 */

struct t *
locate(device)
char *device;
{
	register struct t *p;
	register struct t3 *r;
	for (;;) {
/*
 * Search the configuration table for the specified device.
 */
		for (p=table; p<= &table[tbound]; p++) {
			if (equal(device,p->devname))
				return(p);
		}
/*
 * Since the device was not found in the configuration table, we now
 * search through the alias table.
 */
		for (r=alias; r<= &alias[abound]; r++) {
			if (equal(device,r->new)) {
				device = r->old;
				break;
			}
		}
/*
 * See if the device was found in the alias table. If it wasn't, return a 0.
 */
		if (r > &alias[abound])
			return(0);
/*
 * It was found in alias table, so we change its name back to the
 * reference name of the device and re-initiate the configuration
 * table search.
 */
	}
}

/*
 * This routine is used to search the parameter table
 * for the keyword that was specified in the configuration.  If the
 * keyword cannot be found in this table, a value of zero is returned.
 * If the keyword is found, a pointer to that entry is returned.
 */
struct t4 *
lookup(keyword)
char *keyword;
{
	register struct t4 *kwdptr;
	for (kwdptr=parms; kwdptr<= &parms[pbound]; kwdptr++) {
		if (equal(keyword,kwdptr->indef))
			return (kwdptr);
	}
	return(0);
}

/*
 * This routine is used to map lower case alphabetics into upper case.
 */
char *
uppermap(device,caps)
char *device;
char *caps;
{
	register char *ptr;
	register char *ptr2;
	ptr2 = caps;
	for (ptr=device; *ptr!=NULL; ptr++) {
		if ('a' <= *ptr && *ptr <= 'z')
			*ptr2++ = *ptr + 'A' - 'a';
		else
			*ptr2++ = *ptr;
	}
	*ptr2 = NULL;
	return (caps);
}

/*
 * This routine enters the device in the configuration table.
 */
enterdev(vec,add,bl,ml,name)
int	vec, bl, ml;
unsigned	add;
char	*name;
{
	tbound++;
	if (tbound == TSIZE) {
		printf ("Configuration table overflow\n");
		exit(1);
	}
/*
 * Write upper case define statement and sequence number for device.
 */
	fprintf (fdconf,"#define\t%s_%d 1\n",
		uppermap (name,capitals),
		q->dcount);
	if (q->vsize)
		setlow(vec);
	setq();
	p->devname = q->dev;
	p->vector = vec;
	p->address = add;
	p->buslevel = bl;
	p->addrsize = q->asize;
	p->vectsize = q->vsize;
	p->type = q->type2;
	p->handler = q->hndlr;
	p->count = q->dcount;
	p->mlt = ml;
	p->blk = q->block;
	p->chr = q->charr;
	p++;
	q->dcount++;
	return;
}

/*
 * This routine enters the appropriate pointer for the device
 * into low core.
 */
setlow(vec)
int vec;
{
	subv = vec/4;
	if (q->vsize == 8)
		lowptr[subv+1] = p;
	lowptr[subv] = p;
	return;
}

addrcheck(add,size)
unsigned short add;
short size;
{
	register struct map *mp;
	register struct map *hold;

	for (mp= &map[mbound]; mp >= map; mp--) {
		if (add >= mp->start) {
			if ((add+size) > (mp->start+mp->length))
				return(1);
			if ((add == mp->start) &&
				((size+add)==(mp->start+mp->length))) {
				mbound--;
				while (mp <= &map[mbound]) {
					mp->start = (mp+1)->start;
					mp->length = (mp+1)->length;
					mp++;
				}
				return(0);
			}
			if (add == mp->start) {
				mp->start += size;
				mp->length -= size;
				return(0);
			}
			if ((size + add) == (mp->start + mp->length)) {
				mp->length -= size;
				return(0);
			}
			hold = mp;
			if ((mbound + 1) == MSIZE) {
				printf ("Address map table overflow\n");
				exit(1);
			}
			for (mp= &map[mbound]; mp >= hold; mp--) {
				(mp+1)->start = mp->start;
				(mp+1)->length = mp->length;
			}
			mp++;
			mp->length = add - mp->start;
			(mp+1)->start = size + add;
			(mp+1)->length = (mp+1)->length-(size+add)+mp->start;
			mbound++;
			return(0);
		}
	}
	return(1);
}
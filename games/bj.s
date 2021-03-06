/ bj -- black jack

	sys	48.; 2; prtot
	jsr	r5,mesg; <Black Jack\n\0>; .even
	mov	sp,gsp
	clr	r0
1:
	movb	r0,deck(r0)
	inc	r0
	cmp	r0,$52.
	blo	1b

game:
	mov	gsp,sp
	jsr	r5,mesg; <new game\n\0>; .even
	clr	dbjf
	add	taction,action
	clr	taction
	jsr	pc,shuffle
	jsr	r5,quest5
	mov	total,stotal
	jsr	pc,card
	mov	cval,upval
	mov	aceflg,upace
	jsr	pc,peek
	mov	r0,dhole
	mov	cval,r0
	add	upval,r0
	cmp	r0,$21.
	bne	1f
	inc	dbjf
1:
	jsr	r5,mesg; < up\n\0>; .even
	jsr	pc,card
	mov	cval,ival
	mov	aceflg,iace
	mov	$'+,r0
	jsr	pc,putc
	mov	$-1,-(sp)
	mov	$-1,-(sp) / safety
	clr	dflag
	br 	1f

doubl:
	cmp	dflag,$1
	beq	2f
	jmp	dmove
2:
	inc	dflag
1:
	mov	ival,value
	mov	iace,naces
	mov	$1,fcard
	mov	bet,-(sp)

hit:
	jsr	pc,card
	jsr	pc,nline
	jsr	pc,nline
	add	cval,value
	add	aceflg,naces
	dec	fcard
	bge	2f
	jmp	1f
2:
	add	bet,taction
	tst	dflag
	bne	2f
	tst	upace
	beq	2f
	jsr	r5,mesg; <Insurance? \0>; .even
	jsr	r5,quest1
		br 2f
	mov	bet,r0
	add	r0,total
	asr	r0
	add	r0,tacti2b
	cmp	r1,$ibuf+4
	bne	1b
	jsr	pc,getc
	cmp	r0,$'\n
	bne	error
	mov	$ibuf,r1
	mov	$act,r2
1:
	cmpb	(r1)+,(r2)+
	bne	2f
	inc	nbulls
	dec	ncows
2:
	cmp	r1,$ibuf+4
	bne	1b
	mov	nbulls,r0
	jsr	pc,decml
	jsr	r5,mesg; < bulls; \0>; .even
	mov	ncows,r0
	jsr	pc,decml
	jsr	r5,mesg; < cows\n\0>; .even
	cmp	nbulls,$4
	beq	1f
	inc	nguess
	jmp	loop
1:
	mov	nguess,r0
	jsr	pc,decml
	jsr	r5,mesg; < guesses\n\n\0>; .even
	jmp	game

act:	.=.+4
ibuf:	.=.+6
ncows:	.=.+2
nbulls:	.=.+2
nguess:	.=.+2

o?q ???   ??U?   ?src/games/ttt.s / ttt -- learning tic-tac-toe

	jsr	r5,mesg; <Tic-Tac-Toe\n\0>; .even
	jsr	r5,mesg; <Accumulated knowledge? \0>; .even
	jsr	r5,quest
		br 1f
	sys	open; ttt.k; 0
	bes	1f
	mov	r0,r1
	sys	read; badbuf; 1000.
	mov	r1,r0
	sys	close
1:
	mov	$badbuf,r0
1:
	tst	(r0)+
	bne	1b
	tst	-(r0)
	mov	r0,badp
	sub	$badbuf,r0
	asr	r0
	jsr	pc,decml
	jsr	r5,mesg; < 'bits' of knowledge\n\0>; .even
	sys	48.; 2; addno

game:
	jsr	r5,mesg; <new game\n\0>; .even

/ initialize board

	mov	$singbuf,singp
	mov	$board,r0
1:
	clrb	(r0)+
	cmp	r0,$board+9
	blo	1b

loop:

/ print board

	mov	$board,r1
1:
	movb	(r1)+,r0
	beq	3f
	dec	r0
	beq	2f
	mov	$'X,r0
	br	4f
2:
	mov	$'O,r0
	br	4f
3:
	mov	r1,r0
	sub	$board-'0,r0
4:
	jsr	pc,putc
	cmp	r1,$board+3
	beq	2f
	cmp	r1,$board+6
	beq	2f
	cmp	r1,$board+9
	bne	1b
2:
	jsr	pc,nline
	cmp	r1,$board+9
	blo	1b
	jsr	pc,nline

/ get bad move

	jsr	r5,mesg; <? \0>; .even
	jsr	pc,getc
	cmp	r0,$'\n
	beq	1f
	sub	$'1,r0
	cmp	r0,$8
	bhi	badmov
	tstb	board(r0)
	bne	badmov
	mov	r0,r1
	jsr	pc,getc
	cmp	r0,$'\n
	bne	badmov
	movb	$2,board(r1)
1:

/ check if he won

	jsr	r5,check; 6.
		br loose

/ check if cat game

	jsr	pc,catgam

/ select good move

	clr	ntrial
	clr	trial
	mov	$board,r4
1:
	tstb	(r4)
	bne	2f
	movb	$1,(r4)
	jsr	r5,check; 3
		br win
	jsr	pc,getsit
	clrb	(r4)
	mov	$badbuf,r1
3:
	cmp	r1,badp
	bhis	3f
	cmp	r0,(r1)+
	bne	3b
	br	2f
3:
	cmp	r0,trial
	beq	2f
	blo	3f
	mov	r0,trial
	mov	r4,move
3:
	inc	ntrial
2:
	inc	r4
	cmp	r4,$board+9
	blo	1b

/ install move

	tst	ntrial
	beq	conced
1:
	cmp	ntrial,$1
	beq	1f
	mov	$singbuf,singp
1:
	mov	singp,r0
	mov	trial,(r0)+
	mov	r0,singp
	mov	move,r0
	movb	$1,(r0)

	jmp	loop

badmov:
	jsr	r5,mesg; <Illegal move\n\0>; .even
	jsr	pc,flush
	jmp	loop

loose:
	jsr	r5,mesg; <You win\n\0>; .even
	br	1f

conced:
	jsr	r5,mesg; <I concede\n\0>; .even
1:
	mov	badp,r1
	mov	$singbuf,r2
1:
	cmp	r2,singp
	bhis	1f
	mov	(r2)+,(r1)+
	br	1b
1:
	mov	r1,badp
	jmp	game

win:
	jsr	r5,mesg; <I win\n\0>; .even
	jmp	game

getsit:
	mov	$maps,r2
	clr	r3
1:
	mov	$9,r1
	mov	r5,-(sp)
	clr	r5
2:
	mul	$3,r5
	movb	(r2)+,r0
	movb	board(r0),r0
	add	r0,r5
	sob	r1,2b
	mov	r5,r0
	mov	(sp)+,r5
	cmp	r0,r3
	blo	2f
	mov	r0,r3
2:
	cmp	r2,$maps+[9*8]
	blo	1b
	mov	r3,r0
	rts	pc

catgam:
	mov	$board,r0
1:
	tstb	(r0)+
	beq	1f
	cmp	r0,$board+9
	blo	1b
	jsr	r5,mesg; <Draw\n\0>; .even
	jmp	game
1:
	rts	pc

check:
	mov	$wins,r1
1:
	jsr	pc,1f
	mov	r0,r2
	jsr	pc,1f
	add	r0,r2
	jsr	pc,1f
	add	r0,r2
	cmp	r2,(r5)
	beq	2f
	cmp	r1,$wins+[8*3]
	blo	1b
	tst	(r5)+
2:
	tst	(r5)+
	rts	r5
1:
	movb	(r1)+,r0
	movb	board(r0),r0
	bne	1f
	mov	$10.,r0
1:
	rts	pc

addno:
	sys	open; ttt.k; 0
	bes	1f
	mov	r0,r1
	mov	badp,0f
	sys	re
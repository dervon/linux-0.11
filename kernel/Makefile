#                                                                               ## 1# 
# Makefile for the FREAX-kernel.                                                ## 2# 
#                                                                               ## 3# 
# Note! Dependencies are done automagically by 'make dep', which also           ## 4# 
# removes any old dependencies. DON'T put your own dependencies here            ## 5# 
# unless it's something special (ie not a .c file).                             ## 6# 
#                                                                               ## 7# 
                                                                                ## 8# 
AR	=gar                                                                    ## 9# 
AS	=gas                                                                    ##10# 
LD	=gld                                                                    ##11# 
LDFLAGS	=-s -x                                                                  ##12# 
CC	=gcc                                                                    ##13# 
CFLAGS	=-Wall -O -fstrength-reduce -fomit-frame-pointer -fcombine-regs \       ##14# 
	-finline-functions -mstring-insns -nostdinc -I../include                ##15# 
CPP	=gcc -E -nostdinc -I../include                                          ##16# 
                                                                                ##17# 
.c.s:                                                                           ##18# 
	$(CC) $(CFLAGS) \                                                       ##19# 
	-S -o $*.s $<                                                           ##20# 
.s.o:                                                                           ##21# 
	$(AS) -c -o $*.o $<                                                     ##22# 
.c.o:                                                                           ##23# 
	$(CC) $(CFLAGS) \                                                       ##24# 
	-c -o $*.o $<                                                           ##25# 
                                                                                ##26# 
OBJS  = sched.o system_call.o traps.o asm.o fork.o \                            ##27# 将sched.c system_call.s traps.c asm.s fork.c panic.c printk.c
	panic.o printk.o vsprintf.o sys.o exit.o \                              ##28#  vsprintf.c sys.c exit.c signal.c mktime.c编译成*.o模块
	signal.o mktime.o                                                       ##29# 
                                                                                ##30# 
kernel.o: $(OBJS)                                                               ##31# 
	$(LD) -r -o kernel.o $(OBJS)                                            ##32# [b;]将*.o模块链接生成kernel.o模块
	sync                                                                    ##33# 
                                                                                ##34# 
clean:                                                                          ##35# 
	rm -f core *.o *.a tmp_make keyboard.s                                  ##36# 
	for i in *.c;do rm -f `basename $$i .c`.s;done                          ##37# 
	(cd chr_drv; make clean)                                                ##38# 
	(cd blk_drv; make clean)                                                ##39# 
	(cd math; make clean)                                                   ##40# 
                                                                                ##41# 
dep:                                                                            ##42# 
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make                      ##43# 
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \            ##44# 
		$(CPP) -M $$i;done) >> tmp_make                                 ##45# 
	cp tmp_make Makefile                                                    ##46# 
	(cd chr_drv; make dep)                                                  ##47# 
	(cd blk_drv; make dep)                                                  ##48# 
                                                                                ##49# 
### Dependencies:                                                               ##50# 
exit.s exit.o : exit.c ../include/errno.h ../include/signal.h \                 ##51# 
  ../include/sys/types.h ../include/sys/wait.h ../include/linux/sched.h \       ##52# 
  ../include/linux/head.h ../include/linux/fs.h ../include/linux/mm.h \         ##53# 
  ../include/linux/kernel.h ../include/linux/tty.h ../include/termios.h \       ##54# 
  ../include/asm/segment.h                                                      ##55# 
fork.s fork.o : fork.c ../include/errno.h ../include/linux/sched.h \            ##56# 
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \        ##57# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \         ##58# 
  ../include/asm/segment.h ../include/asm/system.h                              ##59# 
mktime.s mktime.o : mktime.c ../include/time.h                                  ##60# 
panic.s panic.o : panic.c ../include/linux/kernel.h ../include/linux/sched.h \  ##61# 
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \        ##62# 
  ../include/linux/mm.h ../include/signal.h                                     ##63# 
printk.s printk.o : printk.c ../include/stdarg.h ../include/stddef.h \          ##64# 
  ../include/linux/kernel.h                                                     ##65# 
sched.s sched.o : sched.c ../include/linux/sched.h ../include/linux/head.h \    ##66# 
  ../include/linux/fs.h ../include/sys/types.h ../include/linux/mm.h \          ##67# 
  ../include/signal.h ../include/linux/kernel.h ../include/linux/sys.h \        ##68# 
  ../include/linux/fdreg.h ../include/asm/system.h ../include/asm/io.h \        ##69# 
  ../include/asm/segment.h                                                      ##70# 
signal.s signal.o : signal.c ../include/linux/sched.h ../include/linux/head.h \ ##71# 
  ../include/linux/fs.h ../include/sys/types.h ../include/linux/mm.h \          ##72# 
  ../include/signal.h ../include/linux/kernel.h ../include/asm/segment.h        ##73# 
sys.s sys.o : sys.c ../include/errno.h ../include/linux/sched.h \               ##74# 
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \        ##75# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/tty.h \            ##76# 
  ../include/termios.h ../include/linux/kernel.h ../include/asm/segment.h \     ##77# 
  ../include/sys/times.h ../include/sys/utsname.h                               ##78# 
traps.s traps.o : traps.c ../include/string.h ../include/linux/head.h \         ##79# 
  ../include/linux/sched.h ../include/linux/fs.h ../include/sys/types.h \       ##80# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \         ##81# 
  ../include/asm/system.h ../include/asm/segment.h ../include/asm/io.h          ##82# 
vsprintf.s vsprintf.o : vsprintf.c ../include/stdarg.h ../include/string.h      ##83# 

#                                                                                     ## 1# 
# Makefile for the FREAX-kernel character device drivers.                             ## 2# 
#                                                                                     ## 3# 
# Note! Dependencies are done automagically by 'make dep', which also                 ## 4# 
# removes any old dependencies. DON'T put your own dependencies here                  ## 5# 
# unless it's something special (ie not a .c file).                                   ## 6# 
#                                                                                     ## 7# 
                                                                                      ## 8# 
AR	=gar                                                                          ## 9# 
AS	=gas                                                                          ##10# 
LD	=gld                                                                          ##11# 
LDFLAGS	=-s -x                                                                        ##12# 
CC	=gcc                                                                          ##13# 
CFLAGS	=-Wall -O -fstrength-reduce -fomit-frame-pointer -fcombine-regs \             ##14# 
	-finline-functions -mstring-insns -nostdinc -I../../include                   ##15# 
CPP	=gcc -E -nostdinc -I../../include                                             ##16# 
                                                                                      ##17# 
.c.s:                                                                                 ##18# 
	$(CC) $(CFLAGS) \                                                             ##19# 
	-S -o $*.s $<                                                                 ##20# 
.s.o:                                                                                 ##21# 
	$(AS) -c -o $*.o $<                                                           ##22# 
.c.o:                                                                                 ##23# 
	$(CC) $(CFLAGS) \                                                             ##24# 
	-c -o $*.o $<                                                                 ##25# 
                                                                                      ##26# 
OBJS  = tty_io.o console.o keyboard.o serial.o rs_io.o \                              ##27# 将*.c或*.s生成对应的*.o模块
	tty_ioctl.o                                                                   ##28# 
                                                                                      ##29# 
chr_drv.a: $(OBJS)                                                                    ##30# 
	$(AR) rcs chr_drv.a $(OBJS)                                                   ##31# 将*.o模块连接成目标chr_drv.a库文件
	sync                                                                          ##32# 
                                                                                      ##33# 
keyboard.s: keyboard.S ../../include/linux/config.h                                   ##34# 将*.S预处理生成对应的*.s模块(使用大写的.S结尾的文件，可以在该汇编程序中使用很多C语言的伪指令，例如“#include”、“#if”等)
	$(CPP) -traditional keyboard.S -o keyboard.s                                  ##35# 
                                                                                      ##36# 
clean:                                                                                ##37# 
	rm -f core *.o *.a tmp_make keyboard.s                                        ##38# 
	for i in *.c;do rm -f `basename $$i .c`.s;done                                ##39# 
                                                                                      ##40# 
dep:                                                                                  ##41# 
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make                            ##42# 
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \                  ##43# 
		$(CPP) -M $$i;done) >> tmp_make                                       ##44# 
	cp tmp_make Makefile                                                          ##45# 
                                                                                      ##46# 
### Dependencies:                                                                     ##47# 
console.s console.o : console.c ../../include/linux/sched.h \                         ##48# 
  ../../include/linux/head.h ../../include/linux/fs.h \                               ##49# 
  ../../include/sys/types.h ../../include/linux/mm.h ../../include/signal.h \         ##50# 
  ../../include/linux/tty.h ../../include/termios.h ../../include/asm/io.h \          ##51# 
  ../../include/asm/system.h                                                          ##52# 
serial.s serial.o : serial.c ../../include/linux/tty.h ../../include/termios.h \      ##53# 
  ../../include/linux/sched.h ../../include/linux/head.h \                            ##54# 
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \       ##55# 
  ../../include/signal.h ../../include/asm/system.h ../../include/asm/io.h            ##56# 
tty_io.s tty_io.o : tty_io.c ../../include/ctype.h ../../include/errno.h \            ##57# 
  ../../include/signal.h ../../include/sys/types.h \                                  ##58# 
  ../../include/linux/sched.h ../../include/linux/head.h \                            ##59# 
  ../../include/linux/fs.h ../../include/linux/mm.h ../../include/linux/tty.h \       ##60# 
  ../../include/termios.h ../../include/asm/segment.h \                               ##61# 
  ../../include/asm/system.h                                                          ##62# 
tty_ioctl.s tty_ioctl.o : tty_ioctl.c ../../include/errno.h ../../include/termios.h \ ##63# 
  ../../include/linux/sched.h ../../include/linux/head.h \                            ##64# 
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \       ##65# 
  ../../include/signal.h ../../include/linux/kernel.h \                               ##66# 
  ../../include/linux/tty.h ../../include/asm/io.h \                                  ##67# 
  ../../include/asm/segment.h ../../include/asm/system.h                              ##68# 

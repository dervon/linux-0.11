#                                                                                         ## 1# 
# Makefile for the FREAX-kernel block device drivers.                                     ## 2# 
#                                                                                         ## 3# 
# Note! Dependencies are done automagically by 'make dep', which also                     ## 4# 
# removes any old dependencies. DON'T put your own dependencies here                      ## 5# 
# unless it's something special (ie not a .c file).                                       ## 6# 
#                                                                                         ## 7# 
                                                                                          ## 8# 
AR	=gar                                                                              ## 9# 
AS	=gas                                                                              ##10# 
LD	=gld                                                                              ##11# 
LDFLAGS	=-s -x                                                                            ##12# 
CC	=gcc                                                                              ##13# 
CFLAGS	=-Wall -O -fstrength-reduce -fomit-frame-pointer -fcombine-regs \                 ##14# 
	-finline-functions -mstring-insns -nostdinc -I../../include                       ##15# 
CPP	=gcc -E -nostdinc -I../../include                                                 ##16# 
                                                                                          ##17# 
.c.s:                                                                                     ##18# 
	$(CC) $(CFLAGS) \                                                                 ##19# 
	-S -o $*.s $<                                                                     ##20# 
.s.o:                                                                                     ##21# 
	$(AS) -c -o $*.o $<                                                               ##22# 
.c.o:                                                                                     ##23# 
	$(CC) $(CFLAGS) \                                                                 ##24# 
	-c -o $*.o $<                                                                     ##25# 
                                                                                          ##26# 
OBJS  = ll_rw_blk.o floppy.o hd.o ramdisk.o                                               ##27# 将*.c生成对应的*.o模块
                                                                                          ##28# 
blk_drv.a: $(OBJS)                                                                        ##29# 
	$(AR) rcs blk_drv.a $(OBJS)                                                       ##30# 将*.o模块连接成目标blk_drv.a库文件
	sync                                                                              ##31# 
                                                                                          ##32# 
clean:                                                                                    ##33# 
	rm -f core *.o *.a tmp_make                                                       ##34# 
	for i in *.c;do rm -f `basename $$i .c`.s;done                                    ##35# 
                                                                                          ##36# 
dep:                                                                                      ##37# 
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make                                ##38# 
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \                      ##39# 
		$(CPP) -M $$i;done) >> tmp_make                                           ##40# 
	cp tmp_make Makefile                                                              ##41# 
                                                                                          ##42# 
### Dependencies:                                                                         ##43# 
floppy.s floppy.o : floppy.c ../../include/linux/sched.h ../../include/linux/head.h \     ##44# 
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \           ##45# 
  ../../include/signal.h ../../include/linux/kernel.h \                                   ##46# 
  ../../include/linux/fdreg.h ../../include/asm/system.h \                                ##47# 
  ../../include/asm/io.h ../../include/asm/segment.h blk.h                                ##48# 
hd.s hd.o : hd.c ../../include/linux/config.h ../../include/linux/sched.h \               ##49# 
  ../../include/linux/head.h ../../include/linux/fs.h \                                   ##50# 
  ../../include/sys/types.h ../../include/linux/mm.h ../../include/signal.h \             ##51# 
  ../../include/linux/kernel.h ../../include/linux/hdreg.h \                              ##52# 
  ../../include/asm/system.h ../../include/asm/io.h \                                     ##53# 
  ../../include/asm/segment.h blk.h                                                       ##54# 
ll_rw_blk.s ll_rw_blk.o : ll_rw_blk.c ../../include/errno.h ../../include/linux/sched.h \ ##55# 
  ../../include/linux/head.h ../../include/linux/fs.h \                                   ##56# 
  ../../include/sys/types.h ../../include/linux/mm.h ../../include/signal.h \             ##57# 
  ../../include/linux/kernel.h ../../include/asm/system.h blk.h                           ##58# 

AR	=gar                                                                ##  1# 
AS	=gas                                                                ##  2# 
CC	=gcc                                                                ##  3# 
LD	=gld                                                                ##  4# 
CFLAGS	=-Wall -O -fstrength-reduce -fcombine-regs -fomit-frame-pointer \   ##  5# 
	-mstring-insns -nostdinc -I../include                               ##  6# 
CPP	=gcc -E -nostdinc -I../include                                      ##  7# 
                                                                            ##  8# 
.c.s:                                                                       ##  9# 
	$(CC) $(CFLAGS) \                                                   ## 10# 
	-S -o $*.s $<                                                       ## 11# 
.c.o:                                                                       ## 12# 
	$(CC) $(CFLAGS) \                                                   ## 13# 
	-c -o $*.o $<                                                       ## 14# 
.s.o:                                                                       ## 15# 
	$(AS) -o $*.o $<                                                    ## 16# 
                                                                            ## 17# 
OBJS=	open.o read_write.o inode.o file_table.o buffer.o super.o \         ## 18# 将*.c生成对应的*.o模块
	block_dev.o char_dev.o file_dev.o stat.o exec.o pipe.o namei.o \    ## 19# 
	bitmap.o fcntl.o ioctl.o truncate.o                                 ## 20# 
                                                                            ## 21# 
fs.o: $(OBJS)                                                               ## 22# 
	$(LD) -r -o fs.o $(OBJS)                                            ## 23# 将*.o模块链接生成fs.o模块
                                                                            ## 24# 
clean:                                                                      ## 25# 
	rm -f core *.o *.a tmp_make                                         ## 26# 
	for i in *.c;do rm -f `basename $$i .c`.s;done                      ## 27# 
                                                                            ## 28# 
dep:                                                                        ## 29# 
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make                  ## 30# 
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make                    ## 31# 
	cp tmp_make Makefile                                                ## 32# 
                                                                            ## 33# 
### Dependencies:                                                           ## 34# 
bitmap.o : bitmap.c ../include/string.h ../include/linux/sched.h \          ## 35# 
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \    ## 36# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h       ## 37# 
block_dev.o : block_dev.c ../include/errno.h ../include/linux/sched.h \     ## 38# 
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \    ## 39# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \     ## 40# 
  ../include/asm/segment.h ../include/asm/system.h                          ## 41# 
buffer.o : buffer.c ../include/stdarg.h ../include/linux/config.h \         ## 42# 
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \  ## 43# 
  ../include/sys/types.h ../include/linux/mm.h ../include/signal.h \        ## 44# 
  ../include/linux/kernel.h ../include/asm/system.h ../include/asm/io.h     ## 45# 
char_dev.o : char_dev.c ../include/errno.h ../include/sys/types.h \         ## 46# 
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \  ## 47# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \     ## 48# 
  ../include/asm/segment.h ../include/asm/io.h                              ## 49# 
exec.o : exec.c ../include/errno.h ../include/string.h \                    ## 50# 
  ../include/sys/stat.h ../include/sys/types.h ../include/a.out.h \         ## 51# 
  ../include/linux/fs.h ../include/linux/sched.h ../include/linux/head.h \  ## 52# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \     ## 53# 
  ../include/asm/segment.h                                                  ## 54# 
fcntl.o : fcntl.c ../include/string.h ../include/errno.h \                  ## 55# 
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \  ## 56# 
  ../include/sys/types.h ../include/linux/mm.h ../include/signal.h \        ## 57# 
  ../include/linux/kernel.h ../include/asm/segment.h ../include/fcntl.h \   ## 58# 
  ../include/sys/stat.h                                                     ## 59# 
file_dev.o : file_dev.c ../include/errno.h ../include/fcntl.h \             ## 60# 
  ../include/sys/types.h ../include/linux/sched.h ../include/linux/head.h \ ## 61# 
  ../include/linux/fs.h ../include/linux/mm.h ../include/signal.h \         ## 62# 
  ../include/linux/kernel.h ../include/asm/segment.h                        ## 63# 
file_table.o : file_table.c ../include/linux/fs.h ../include/sys/types.h    ## 64# 
inode.o : inode.c ../include/string.h ../include/sys/stat.h \               ## 65# 
  ../include/sys/types.h ../include/linux/sched.h ../include/linux/head.h \ ## 66# 
  ../include/linux/fs.h ../include/linux/mm.h ../include/signal.h \         ## 67# 
  ../include/linux/kernel.h ../include/asm/system.h                         ## 68# 
ioctl.o : ioctl.c ../include/string.h ../include/errno.h \                  ## 69# 
  ../include/sys/stat.h ../include/sys/types.h ../include/linux/sched.h \   ## 70# 
  ../include/linux/head.h ../include/linux/fs.h ../include/linux/mm.h \     ## 71# 
  ../include/signal.h                                                       ## 72# 
namei.o : namei.c ../include/linux/sched.h ../include/linux/head.h \        ## 73# 
  ../include/linux/fs.h ../include/sys/types.h ../include/linux/mm.h \      ## 74# 
  ../include/signal.h ../include/linux/kernel.h ../include/asm/segment.h \  ## 75# 
  ../include/string.h ../include/fcntl.h ../include/errno.h \               ## 76# 
  ../include/const.h ../include/sys/stat.h                                  ## 77# 
open.o : open.c ../include/string.h ../include/errno.h ../include/fcntl.h \ ## 78# 
  ../include/sys/types.h ../include/utime.h ../include/sys/stat.h \         ## 79# 
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \  ## 80# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/tty.h \        ## 81# 
  ../include/termios.h ../include/linux/kernel.h ../include/asm/segment.h   ## 82# 
pipe.o : pipe.c ../include/signal.h ../include/sys/types.h \                ## 83# 
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \  ## 84# 
  ../include/linux/mm.h ../include/asm/segment.h                            ## 85# 
read_write.o : read_write.c ../include/sys/stat.h ../include/sys/types.h \  ## 86# 
  ../include/errno.h ../include/linux/kernel.h ../include/linux/sched.h \   ## 87# 
  ../include/linux/head.h ../include/linux/fs.h ../include/linux/mm.h \     ## 88# 
  ../include/signal.h ../include/asm/segment.h                              ## 89# 
stat.o : stat.c ../include/errno.h ../include/sys/stat.h \                  ## 90# 
  ../include/sys/types.h ../include/linux/fs.h ../include/linux/sched.h \   ## 91# 
  ../include/linux/head.h ../include/linux/mm.h ../include/signal.h \       ## 92# 
  ../include/linux/kernel.h ../include/asm/segment.h                        ## 93# 
super.o : super.c ../include/linux/config.h ../include/linux/sched.h \      ## 94# 
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \    ## 95# 
  ../include/linux/mm.h ../include/signal.h ../include/linux/kernel.h \     ## 96# 
  ../include/asm/system.h ../include/errno.h ../include/sys/stat.h          ## 97# 
truncate.o : truncate.c ../include/linux/sched.h ../include/linux/head.h \  ## 98# 
  ../include/linux/fs.h ../include/sys/types.h ../include/linux/mm.h \      ## 99# 
  ../include/signal.h ../include/sys/stat.h                                 ##100# 

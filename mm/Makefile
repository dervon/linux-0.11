CC	=gcc                                                                 ## 1# 
CFLAGS	=-O -Wall -fstrength-reduce -fcombine-regs -fomit-frame-pointer \    ## 2# 
	-finline-functions -nostdinc -I../include                            ## 3# 
AS	=gas                                                                 ## 4# 
AR	=gar                                                                 ## 5# 
LD	=gld                                                                 ## 6# 
CPP	=gcc -E -nostdinc -I../include                                       ## 7# 
                                                                             ## 8# 
.c.o:                                                                        ## 9# 
	$(CC) $(CFLAGS) \                                                    ##10# 
	-c -o $*.o $<                                                        ##11# 
.s.o:                                                                        ##12# 
	$(AS) -o $*.o $<                                                     ##13# 
.c.s:                                                                        ##14# 
	$(CC) $(CFLAGS) \                                                    ##15# 
	-S -o $*.s $<                                                        ##16# 
                                                                             ##17# 
OBJS	= memory.o page.o                                                    ##18# 将memory.c和page.s编译成*.o模块
                                                                             ##19# 
all: mm.o                                                                    ##20# 
                                                                             ##21# 
mm.o: $(OBJS)                                                                ##22# [b;]将memory.o和page.o模块链接生成mm.o模块
	$(LD) -r -o mm.o $(OBJS)                                             ##23# 
                                                                             ##24# 
clean:                                                                       ##25# 
	rm -f core *.o *.a tmp_make                                          ##26# 
	for i in *.c;do rm -f `basename $$i .c`.s;done                       ##27# 
                                                                             ##28# 
dep:                                                                         ##29# 
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make                   ##30# 
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make                     ##31# 
	cp tmp_make Makefile                                                 ##32# 
                                                                             ##33# 
### Dependencies:                                                            ##34# 
memory.o : memory.c ../include/signal.h ../include/sys/types.h \             ##35# 
  ../include/asm/system.h ../include/linux/sched.h ../include/linux/head.h \ ##36# 
  ../include/linux/fs.h ../include/linux/mm.h ../include/linux/kernel.h      ##37# 

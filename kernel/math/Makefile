#                                                                         ## 1# 
# Makefile for the FREAX-kernel character device drivers.                 ## 2# 
#                                                                         ## 3# 
# Note! Dependencies are done automagically by 'make dep', which also     ## 4# 
# removes any old dependencies. DON'T put your own dependencies here      ## 5# 
# unless it's something special (ie not a .c file).                       ## 6# 
#                                                                         ## 7# 
                                                                          ## 8# 
AR	=gar                                                              ## 9# 
AS	=gas                                                              ##10# 
LD	=gld                                                              ##11# 
LDFLAGS	=-s -x                                                            ##12# 
CC	=gcc                                                              ##13# 
CFLAGS	=-Wall -O -fstrength-reduce -fomit-frame-pointer -fcombine-regs \ ##14# 
	-finline-functions -mstring-insns -nostdinc -I../../include       ##15# 
CPP	=gcc -E -nostdinc -I../../include                                 ##16# 
                                                                          ##17# 
.c.s:                                                                     ##18# 
	$(CC) $(CFLAGS) \                                                 ##19# 
	-S -o $*.s $<                                                     ##20# 
.s.o:                                                                     ##21# 
	$(AS) -c -o $*.o $<                                               ##22# 
.c.o:                                                                     ##23# 
	$(CC) $(CFLAGS) \                                                 ##24# 
	-c -o $*.o $<                                                     ##25# 
                                                                          ##26# 
OBJS  = math_emulate.o                                                    ##27# 将*.c生成对应的*.o模块
                                                                          ##28# 
math.a: $(OBJS)                                                           ##29# 
	$(AR) rcs math.a $(OBJS)                                          ##30# 将*.o模块连接成目标math.a库文件
	sync                                                              ##31# 
                                                                          ##32# 
clean:                                                                    ##33# 
	rm -f core *.o *.a tmp_make                                       ##34# 
	for i in *.c;do rm -f `basename $$i .c`.s;done                    ##35# 
                                                                          ##36# 
dep:                                                                      ##37# 
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make                ##38# 
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \      ##39# 
		$(CPP) -M $$i;done) >> tmp_make                           ##40# 
	cp tmp_make Makefile                                              ##41# 
                                                                          ##42# 
### Dependencies:                                                         ##43# 

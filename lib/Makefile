#                                                                              ## 1# 
# Makefile for some libs needed in the kernel.                                 ## 2# 
#                                                                              ## 3# 
# Note! Dependencies are done automagically by 'make dep', which also          ## 4# 
# removes any old dependencies. DON'T put your own dependencies here           ## 5# 
# unless it's something special (ie not a .c file).                            ## 6# 
#                                                                              ## 7# 
                                                                               ## 8# 
AR	=gar                                                                   ## 9# 
AS	=gas                                                                   ##10# 
LD	=gld                                                                   ##11# 
LDFLAGS	=-s -x                                                                 ##12# 
CC	=gcc                                                                   ##13# 
CFLAGS	=-Wall -O -fstrength-reduce -fomit-frame-pointer -fcombine-regs \      ##14# 
	-finline-functions -mstring-insns -nostdinc -I../include               ##15# 
CPP	=gcc -E -nostdinc -I../include                                         ##16# 
                                                                               ##17# 
.c.s:                                                                          ##18# 
	$(CC) $(CFLAGS) \                                                      ##19# 
	-S -o $*.s $<                                                          ##20# 
.s.o:                                                                          ##21# 
	$(AS) -c -o $*.o $<                                                    ##22# 
.c.o:                                                                          ##23# 
	$(CC) $(CFLAGS) \                                                      ##24# 
	-c -o $*.o $<                                                          ##25# 
                                                                               ##26# 
OBJS  = ctype.o _exit.o open.o close.o errno.o write.o dup.o setsid.o \        ##27# 将*.c生成对应的*.o模块
	execve.o wait.o string.o malloc.o                                      ##28# 
                                                                               ##29# 
lib.a: $(OBJS)                                                                 ##30# 
	$(AR) rcs lib.a $(OBJS)                                                ##31# 将*.o模块链接生成lib.a模块
	sync                                                                   ##32# 
                                                                               ##33# 
clean:                                                                         ##34# 
	rm -f core *.o *.a tmp_make                                            ##35# 
	for i in *.c;do rm -f `basename $$i .c`.s;done                         ##36# 
                                                                               ##37# 
dep:                                                                           ##38# 
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make                     ##39# 
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \           ##40# 
		$(CPP) -M $$i;done) >> tmp_make                                ##41# 
	cp tmp_make Makefile                                                   ##42# 
                                                                               ##43# 
### Dependencies:                                                              ##44# 
_exit.s _exit.o : _exit.c ../include/unistd.h ../include/sys/stat.h \          ##45# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##46# 
  ../include/utime.h                                                           ##47# 
close.s close.o : close.c ../include/unistd.h ../include/sys/stat.h \          ##48# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##49# 
  ../include/utime.h                                                           ##50# 
ctype.s ctype.o : ctype.c ../include/ctype.h                                   ##51# 
dup.s dup.o : dup.c ../include/unistd.h ../include/sys/stat.h \                ##52# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##53# 
  ../include/utime.h                                                           ##54# 
errno.s errno.o : errno.c                                                      ##55# 
execve.s execve.o : execve.c ../include/unistd.h ../include/sys/stat.h \       ##56# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##57# 
  ../include/utime.h                                                           ##58# 
malloc.s malloc.o : malloc.c ../include/linux/kernel.h ../include/linux/mm.h \ ##59# 
  ../include/asm/system.h                                                      ##60# 
open.s open.o : open.c ../include/unistd.h ../include/sys/stat.h \             ##61# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##62# 
  ../include/utime.h ../include/stdarg.h                                       ##63# 
setsid.s setsid.o : setsid.c ../include/unistd.h ../include/sys/stat.h \       ##64# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##65# 
  ../include/utime.h                                                           ##66# 
string.s string.o : string.c ../include/string.h                               ##67# 
wait.s wait.o : wait.c ../include/unistd.h ../include/sys/stat.h \             ##68# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##69# 
  ../include/utime.h ../include/sys/wait.h                                     ##70# 
write.s write.o : write.c ../include/unistd.h ../include/sys/stat.h \          ##71# 
  ../include/sys/types.h ../include/sys/times.h ../include/sys/utsname.h \     ##72# 
  ../include/utime.h                                                           ##73# 

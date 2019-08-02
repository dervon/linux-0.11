#ifndef _UNISTD_H                                                                //  1/ 
#define _UNISTD_H                                                                //  2/ 
                                                                                 //  3/ 
/* ok, this may be a joke, but I'm working on it */                              //  4/ 
#define _POSIX_VERSION 198808L                                                   //  5/ 
                                                                                 //  6/ 
#define _POSIX_CHOWN_RESTRICTED	/* only root can do a chown (I think..) */       //  7/ 
#define _POSIX_NO_TRUNC		/* no pathname truncation (but see in kernel) */ //  8/ 
#define _POSIX_VDISABLE '\0'	/* character to disable things like ^C */        //  9/ 
/*#define _POSIX_SAVED_IDS */	/* we'll get to this yet */                      // 10/ 
/*#define _POSIX_JOB_CONTROL */	/* we aren't there quite yet. Soon hopefully */  // 11/ 
                                                                                 // 12/ 
#define STDIN_FILENO	0                                                        // 13/ 标准输入文件描述符号(即文件句柄)
#define STDOUT_FILENO	1                                                        // 14/ 标准输出文件描述符号
#define STDERR_FILENO	2                                                        // 15/ 标准错误文件描述符号
                                                                                 // 16/ 
#ifndef NULL                                                                     // 17/ 
#define NULL    ((void *)0)                                                      // 18/ 定义空指针NULL
#endif                                                                           // 19/ 
                                                                                 // 20/ 
/* access */                                                                     // 21/ 22-25行被用于access函数
#define F_OK	0                                                                // 22/ 检测文件是否存在
#define X_OK	1                                                                // 23/ 检测文件是否可执行(搜索)
#define W_OK	2                                                                // 24/ 检测是否可写
#define R_OK	4                                                                // 25/ 检测是否可读
                                                                                 // 26/ 
/* lseek */                                                                      // 27/ 28-30行被用于lseek函数
#define SEEK_SET	0                                                        // 28/ 将文件读写指针设置为偏移值
#define SEEK_CUR	1                                                        // 29/ 将文件读写指针设置为当前值加上偏移值
#define SEEK_END	2                                                        // 30/ 将文件读写指针设置为文件长度加上偏移值
                                                                                 // 31/ 
/* _SC stands for System Configuration. We don't use them much */                // 32/ 33-40行常用于sysconf()函数，_SC表示系统配置，很少使用
#define _SC_ARG_MAX		1                                                // 33/ 最大变量数
#define _SC_CHILD_MAX		2                                                // 34/ 子进程最大数
#define _SC_CLOCKS_PER_SEC	3                                                // 35/ 每秒滴答数
#define _SC_NGROUPS_MAX		4                                                // 36/ 最大组数
#define _SC_OPEN_MAX		5                                                // 37/ 最大打开文件数
#define _SC_JOB_CONTROL		6                                                // 38/ 作业控制
#define _SC_SAVED_IDS		7                                                // 39/ 保存的标识符
#define _SC_VERSION		8                                                // 40/ 版本
                                                                                 // 41/ 
/* more (possibly) configurable things - now pathnames */                        // 42/ 
#define _PC_LINK_MAX		1                                                // 43/ 
#define _PC_MAX_CANON		2                                                // 44/ 
#define _PC_MAX_INPUT		3                                                // 45/ 
#define _PC_NAME_MAX		4                                                // 46/ 
#define _PC_PATH_MAX		5                                                // 47/ 
#define _PC_PIPE_BUF		6                                                // 48/ 
#define _PC_NO_TRUNC		7                                                // 49/ 
#define _PC_VDISABLE		8                                                // 50/ 
#define _PC_CHOWN_RESTRICTED	9                                                // 51/ 
                                                                                 // 52/ 
#include <sys/stat.h>                                                            // 53/ 
#include <sys/times.h>                                                           // 54/ 
#include <sys/utsname.h>                                                         // 55/ 
#include <utime.h>                                                               // 56/ 
                                                                                 // 57/ 
#ifdef __LIBRARY__                                                               // 58/ 定义该变量为了确定是否将unistd.h中的系统调用号和内嵌汇编等包含进去 [b;]60-131行是内核实现的系统调用符号，用作系统调用函数表(在sys.h)中的索引值
                                                                                 // 59/ 
#define __NR_setup	0	/* used only by init, to get system going */     // 60/ __NR_setup仅用于初始化，以启动系统
#define __NR_exit	1                                                        // 61/ [b;]如果要使用系统调用函数必须先使用下面的_syscall*()定义，用户使用的
#define __NR_fork	2                                                        // 62/ [b;]库中也是使用的_syscall*()
#define __NR_read	3                                                        // 63/ 
#define __NR_write	4                                                        // 64/ 
#define __NR_open	5                                                        // 65/ 
#define __NR_close	6                                                        // 66/ 
#define __NR_waitpid	7                                                        // 67/ 
#define __NR_creat	8                                                        // 68/ 
#define __NR_link	9                                                        // 69/ 
#define __NR_unlink	10                                                       // 70/ 
#define __NR_execve	11                                                       // 71/ 
#define __NR_chdir	12                                                       // 72/ 
#define __NR_time	13                                                       // 73/ 
#define __NR_mknod	14                                                       // 74/ 
#define __NR_chmod	15                                                       // 75/ 
#define __NR_chown	16                                                       // 76/ 
#define __NR_break	17                                                       // 77/ 
#define __NR_stat	18                                                       // 78/ 
#define __NR_lseek	19                                                       // 79/ 
#define __NR_getpid	20                                                       // 80/ 
#define __NR_mount	21                                                       // 81/ 
#define __NR_umount	22                                                       // 82/ 
#define __NR_setuid	23                                                       // 83/ 
#define __NR_getuid	24                                                       // 84/ 
#define __NR_stime	25                                                       // 85/ 
#define __NR_ptrace	26                                                       // 86/ 
#define __NR_alarm	27                                                       // 87/ 
#define __NR_fstat	28                                                       // 88/ 
#define __NR_pause	29                                                       // 89/ 
#define __NR_utime	30                                                       // 90/ 
#define __NR_stty	31                                                       // 91/ 
#define __NR_gtty	32                                                       // 92/ 
#define __NR_access	33                                                       // 93/ 
#define __NR_nice	34                                                       // 94/ 
#define __NR_ftime	35                                                       // 95/ 
#define __NR_sync	36                                                       // 96/ 
#define __NR_kill	37                                                       // 97/ 
#define __NR_rename	38                                                       // 98/ 
#define __NR_mkdir	39                                                       // 99/ 
#define __NR_rmdir	40                                                       //100/ 
#define __NR_dup	41                                                       //101/ 
#define __NR_pipe	42                                                       //102/ 
#define __NR_times	43                                                       //103/ 
#define __NR_prof	44                                                       //104/ 
#define __NR_brk	45                                                       //105/ 
#define __NR_setgid	46                                                       //106/ 
#define __NR_getgid	47                                                       //107/ 
#define __NR_signal	48                                                       //108/ 
#define __NR_geteuid	49                                                       //109/ 
#define __NR_getegid	50                                                       //110/ 
#define __NR_acct	51                                                       //111/ 
#define __NR_phys	52                                                       //112/ 
#define __NR_lock	53                                                       //113/ 
#define __NR_ioctl	54                                                       //114/ 
#define __NR_fcntl	55                                                       //115/ 
#define __NR_mpx	56                                                       //116/ 
#define __NR_setpgid	57                                                       //117/ 
#define __NR_ulimit	58                                                       //118/ 
#define __NR_uname	59                                                       //119/ 
#define __NR_umask	60                                                       //120/ 
#define __NR_chroot	61                                                       //121/ 
#define __NR_ustat	62                                                       //122/ 
#define __NR_dup2	63                                                       //123/ 
#define __NR_getppid	64                                                       //124/ 
#define __NR_getpgrp	65                                                       //125/ 
#define __NR_setsid	66                                                       //126/ 
#define __NR_sigaction	67                                                       //127/ 
#define __NR_sgetmask	68                                                       //128/ 
#define __NR_ssetmask	69                                                       //129/ 
#define __NR_setreuid	70                                                       //130/ 
#define __NR_setregid	71                                                       //131/ 
                                                                                 //132/ 
#define _syscall0(type,name) \                                                   //133/ [b;]133-183行定义系统调用嵌入式汇编宏函数
type name(void) \                                                                //134/ 
{ \                                                                              //135/ 
long __res; \                                                                    //136/ 
__asm__ volatile ("int $0x80" \                                                  //137/ 
	: "=a" (__res) \                                                         //138/ 
	: "0" (__NR_##name)); \                                                  //139/ 
if (__res >= 0) \                                                                //140/ 
	return (type) __res; \                                                   //141/ 
errno = -__res; \                                                                //142/ error为正值
return -1; \                                                                     //143/ 返回-1
}                                                                                //144/ 
                                                                                 //145/ 
#define _syscall1(type,name,atype,a) \                                           //146/ 
type name(atype a) \                                                             //147/ 
{ \                                                                              //148/ 
long __res; \                                                                    //149/ 
__asm__ volatile ("int $0x80" \                                                  //150/ 
	: "=a" (__res) \                                                         //151/ 
	: "0" (__NR_##name),"b" ((long)(a))); \                                  //152/ 
if (__res >= 0) \                                                                //153/ 
	return (type) __res; \                                                   //154/ 
errno = -__res; \                                                                //155/ 
return -1; \                                                                     //156/ 
}                                                                                //157/ 
                                                                                 //158/ 
#define _syscall2(type,name,atype,a,btype,b) \                                   //159/ 
type name(atype a,btype b) \                                                     //160/ 
{ \                                                                              //161/ 
long __res; \                                                                    //162/ 
__asm__ volatile ("int $0x80" \                                                  //163/ 
	: "=a" (__res) \                                                         //164/ 
	: "0" (__NR_##name),"b" ((long)(a)),"c" ((long)(b))); \                  //165/ 
if (__res >= 0) \                                                                //166/ 
	return (type) __res; \                                                   //167/ 
errno = -__res; \                                                                //168/ 
return -1; \                                                                     //169/ 
}                                                                                //170/ 
                                                                                 //171/ 
#define _syscall3(type,name,atype,a,btype,b,ctype,c) \                           //172/ 
type name(atype a,btype b,ctype c) \                                             //173/ 
{ \                                                                              //174/ 
long __res; \                                                                    //175/ 
__asm__ volatile ("int $0x80" \                                                  //176/ 
	: "=a" (__res) \                                                         //177/ 
	: "0" (__NR_##name),"b" ((long)(a)),"c" ((long)(b)),"d" ((long)(c))); \  //178/ 
if (__res>=0) \                                                                  //179/ 
	return (type) __res; \                                                   //180/ 
errno=-__res; \                                                                  //181/ 
return -1; \                                                                     //182/ 
}                                                                                //183/ 
                                                                                 //184/ 
#endif /* __LIBRARY__ */                                                         //185/ 
                                                                                 //186/ 
extern int errno;                                                                //187/ 引用linux/lib/errno.c中定义的error，是全局变量
                                                                                 //188/ 
int access(const char * filename, mode_t mode);                                  //189/ 33 [b;]189-251行引用标准C库中的函数，默认都带有extern属性
int acct(const char * filename);                                                 //190/ 51 
int alarm(int sec);                                                              //191/ 27
int brk(void * end_data_segment);                                                //192/ 45
void * sbrk(ptrdiff_t increment);                                                //193/ 特殊，不在上面的系统调用列表中
int chdir(const char * filename);                                                //194/ 12
int chmod(const char * filename, mode_t mode);                                   //195/ 15
int chown(const char * filename, uid_t owner, gid_t group);                      //196/ 16
int chroot(const char * filename);                                               //197/ 61
int close(int fildes);                                                           //198/ 6
int creat(const char * filename, mode_t mode);                                   //199/ 8
int dup(int fildes);                                                             //200/ 41
int execve(const char * filename, char ** argv, char ** envp);                   //201/ 11
int execv(const char * pathname, char ** argv);                                  //202/ 特殊，不在上面的系统调用列表中
int execvp(const char * file, char ** argv);                                     //203/ 特殊，不在上面的系统调用列表中
int execl(const char * pathname, char * arg0, ...);                              //204/ 特殊，不在上面的系统调用列表中
int execlp(const char * file, char * arg0, ...);                                 //205/ 特殊，不在上面的系统调用列表中
int execle(const char * pathname, char * arg0, ...);                             //206/ 特殊，不在上面的系统调用列表中
volatile void exit(int status);                                                  //207/ 1                         [b;]volatile用于告诉编译器gcc该函数不会返回
volatile void _exit(int status);                                                 //208/ 特殊，不在上面的系统调用列表中  volatile关键字是一种类型修饰符，用它声明
int fcntl(int fildes, int cmd, ...);                                             //209/ 55                         的类型变量表示可以被某些编译器未知的因素更
int fork(void);                                                                  //210/ 2                          改，比如：操作系统、硬件或者其它线程等。遇
int getpid(void);                                                                //211/ 20                         到这个关键字声明的变量，编译器对访问该变量
int getuid(void);                                                                //212/ 24                         的代码就不再进行优化，从而可以提供对特殊地
int geteuid(void);                                                               //213/ 49                         址的稳定访问
int getgid(void);                                                                //214/ 47
int getegid(void);                                                               //215/ 50
int ioctl(int fildes, int cmd, ...);                                             //216/ 54
int kill(pid_t pid, int signal);                                                 //217/ 37
int link(const char * filename1, const char * filename2);                        //218/ 9
int lseek(int fildes, off_t offset, int origin);                                 //219/ 19
int mknod(const char * filename, mode_t mode, dev_t dev);                        //220/ 14
int mount(const char * specialfile, const char * dir, int rwflag);               //221/ 21
int nice(int val);                                                               //222/ 34
int open(const char * filename, int flag, ...);                                  //223/ 5
int pause(void);                                                                 //224/ 29
int pipe(int * fildes);                                                          //225/ 42
int read(int fildes, char * buf, off_t count);                                   //226/ 3
int setpgrp(void);                                                               //227/ 特殊，不在上面的系统调用列表中
int setpgid(pid_t pid,pid_t pgid);                                               //228/ 57
int setuid(uid_t uid);                                                           //229/ 23
int setgid(gid_t gid);                                                           //230/ 46
void (*signal(int sig, void (*fn)(int)))(int);                                   //231/ 48
int stat(const char * filename, struct stat * stat_buf);                         //232/ 18
int fstat(int fildes, struct stat * stat_buf);                                   //233/ 28
int stime(time_t * tptr);                                                        //234/ 25
int sync(void);                                                                  //235/ 36
time_t time(time_t * tloc);                                                      //236/ 13
time_t times(struct tms * tbuf);                                                 //237/ 43
int ulimit(int cmd, long limit);                                                 //238/ 58
mode_t umask(mode_t mask);                                                       //239/ 60
int umount(const char * specialfile);                                            //240/ 22
int uname(struct utsname * name);                                                //241/ 59
int unlink(const char * filename);                                               //242/ 10
int ustat(dev_t dev, struct ustat * ubuf);                                       //243/ 62
int utime(const char * filename, struct utimbuf * times);                        //244/ 30
pid_t waitpid(pid_t pid,int * wait_stat,int options);                            //245/ 7
pid_t wait(int * wait_stat);                                                     //246/ 特殊，不在上面的系统调用列表中
int write(int fildes, const char * buf, off_t count);                            //247/ 4
int dup2(int oldfd, int newfd);                                                  //248/ 63
int getppid(void);                                                               //249/ 64
pid_t getpgrp(void);                                                             //250/ 65
pid_t setsid(void);                                                              //251/ 66
                                                                                 //252/ 
#endif                                                                           //253/ 

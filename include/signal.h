#ifndef _SIGNAL_H                                                             // 1/ 
#define _SIGNAL_H                                                             // 2/ 
                                                                              // 3/ 
#include <sys/types.h>                                                        // 4/ 类型头文件，定义了基本都系统数据类型
                                                                              // 5/ 
typedef int sig_atomic_t;                                                     // 6/ 定义信号原子操作类型
typedef unsigned int sigset_t;		/* 32 bits */                         // 7/ 定义信号集类型
                                                                              // 8/ 
#define _NSIG             32                                                  // 9/ 定义信号种类--32种
#define NSIG		_NSIG                                                 //10/ NSIG = _NSIG
                                                                              //11/ 
#define SIGHUP		 1                                                    //12/ 挂断控制终端或进程 [b;]12-34行是0.11内核定义都信号，其中包括了POSIX.1要求的所有20个信号
#define SIGINT		 2                                                    //13/ 来自键盘的中断
#define SIGQUIT		 3                                                    //14/ 来自键盘的退出
#define SIGILL		 4                                                    //15/ 非法指令
#define SIGTRAP		 5                                                    //16/ 跟踪断点
#define SIGABRT		 6                                                    //17/ 异常结束
#define SIGIOT		 6                                                    //18/ 同上
#define SIGUNUSED	 7                                                    //19/ 没有使用
#define SIGFPE		 8                                                    //20/ 斜处理器出错
#define SIGKILL		 9                                                    //21/ 强迫进程终止
#define SIGUSR1		10                                                    //22/ 用户信号1，进程可使用
#define SIGSEGV		11                                                    //23/ 无效内存引用
#define SIGUSR2		12                                                    //24/ 用户信号2，进程可使用
#define SIGPIPE		13                                                    //25/ 管道写出错，无读者
#define SIGALRM		14                                                    //26/ 实时定时器报警
#define SIGTERM		15                                                    //27/ 进程终止
#define SIGSTKFLT	16                                                    //28/ 栈出错(斜处理器)
#define SIGCHLD		17                                                    //29/ 子进程停止或被终止
#define SIGCONT		18                                                    //30/ 恢复进程继续执行
#define SIGSTOP		19                                                    //31/ 停止进程的执行
#define SIGTSTP		20                                                    //32/ tty发出停止进程，可忽略
#define SIGTTIN		21                                                    //33/ 后台进程请求输入
#define SIGTTOU		22                                                    //34/ 后台进程请求输出
                                                                              //35/ 
/* Ok, I haven't implemented sigactions, but trying to keep headers POSIX */  //36/ 
#define SA_NOCLDSTOP	1                                                     //37/ 当子进程处于停止状态，就不对SIGCHLD处理
#define SA_NOMASK	0x40000000                                            //38/ 不阻止在指定的信号处理程序中再收到该信号
#define SA_ONESHOT	0x80000000                                            //39/ 信号句柄一旦被调用过就恢复到默认处理句柄
                                                                              //40/ 
#define SIG_BLOCK          0	/* for blocking signals */                    //41/ 在阻塞信号集中加上给定信号
#define SIG_UNBLOCK        1	/* for unblocking signals */                  //42/ 在阻塞信号集中加上给定信号
#define SIG_SETMASK        2	/* for setting the signal mask */             //43/ 设置阻塞信号集
                                                                              //44/ 
#define SIG_DFL		((void (*)(int))0)	/* default signal handling */ //45/ 对信号采用默认处理
#define SIG_IGN		((void (*)(int))1)	/* ignore signal */           //46/ 忽略信号的句柄，即对收到的信号不作处理
                                                                              //47/ 
struct sigaction {                                                            //48/ 
	void (*sa_handler)(int);                                              //49/ 对应的信号指定要采取的行动
	sigset_t sa_mask;                                                     //50/ 对应信号的屏蔽码，在信号处理程序执行时将阻塞这些信号
	int sa_flags;                                                         //51/ 指定改变信号处理过程的信号集，由37-39行的位标志定义
	void (*sa_restorer)(void);                                            //52/ 是恢复函数指针，由Libc提供，清理用户态堆栈，用于在信号处理程序结束后恢复系统
};                                                                            //53/ 调用返回时几个寄存器的原有值以及系统调用的返回值，就好像系统调用没有执行过信号
                                                                              //54/ 处理程序而直接返回到用户程序一样
void (*signal(int _sig, void (*_func)(int)))(int);                            //55/ 
int raise(int sig);                                                           //56/ 
int kill(pid_t pid, int sig);                                                 //57/ 
int sigaddset(sigset_t *mask, int signo);                                     //58/ 
int sigdelset(sigset_t *mask, int signo);                                     //59/ 
int sigemptyset(sigset_t *mask);                                              //60/ 
int sigfillset(sigset_t *mask);                                               //61/ 
int sigismember(sigset_t *mask, int signo); /* 1 - is, 0 - not, -1 error */   //62/ 
int sigpending(sigset_t *set);                                                //63/ 
int sigprocmask(int how, sigset_t *set, sigset_t *oldset);                    //64/ 
int sigsuspend(sigset_t *sigmask);                                            //65/ 
int sigaction(int sig, struct sigaction *act, struct sigaction *oldact);      //66/ 
                                                                              //67/ 
#endif /* _SIGNAL_H */                                                        //68/ 

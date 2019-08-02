/*                                                                       //  1/ 
 *  linux/kernel/signal.c                                                //  2/ 
 *                                                                       //  3/ 
 *  (C) 1991  Linus Torvalds                                             //  4/ 
 */                                                                      //  5/ 
                                                                         //  6/ 
#include <linux/sched.h>                                                 //  7/ 
#include <linux/kernel.h>                                                //  8/ 
#include <asm/segment.h>                                                 //  9/ 
                                                                         // 10/ 
#include <signal.h>                                                      // 11/ 
                                                                         // 12/ 
volatile void do_exit(int error_code);                                   // 13/ volatile告诉编译器该函数不会返回
                                                                         // 14/ 
int sys_sgetmask()                                                       // 15/ [b;]系统调用-获取当前任务的屏蔽信号位图
{                                                                        // 16/ 
	return current->blocked;                                         // 17/ 
}                                                                        // 18/ 
                                                                         // 19/ 
int sys_ssetmask(int newmask)                                            // 20/ [b;]系统调用-为当前任务设置新的屏蔽信号位图(SIGKILL不能被屏蔽，奇怪没考虑SIGSTOP，可能是do_signal还不能处理SIGSTOP等信号)，返回旧的屏蔽信号位图
{                                                                        // 21/ 
	int old=current->blocked;                                        // 22/ 
                                                                         // 23/ 
	current->blocked = newmask & ~(1<<(SIGKILL-1));                  // 24/ 
	return old;                                                      // 25/ 
}                                                                        // 26/ 
                                                                         // 27/ 
static inline void save_old(char * from,char * to)                       // 28/ [b;]复制from处的sigaction结构数据到fs数据段to处，即从内核空间复制到用户(任务)数据段中
{                                                                        // 29/ 
	int i;                                                           // 30/ 
                                                                         // 31/ 
	verify_area(to, sizeof(struct sigaction));                       // 32/ 对当前任务的逻辑地址从to到to+sizeof(struct sigaction)这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
	for (i=0 ; i< sizeof(struct sigaction) ; i++) {                  // 33/ 
		put_fs_byte(*from,to);                                   // 34/ 将from指向的一字节数据存放在fs段中to指定的内存地址处
		from++;                                                  // 35/ 
		to++;                                                    // 36/ 
	}                                                                // 37/ 
}                                                                        // 38/ 
                                                                         // 39/ 
static inline void get_new(char * from,char * to)                        // 40/ [b;]复制fs数据段from处的sigaction结构数据到to地址处，即从用户(任务)数据段复制到内核空间中
{                                                                        // 41/ 
	int i;                                                           // 42/ 
                                                                         // 43/ 
	for (i=0 ; i< sizeof(struct sigaction) ; i++)                    // 44/ 
		*(to++) = get_fs_byte(from++);                           // 45/ 将fs段中from++指定的内存地址处的一个字节存取出来，存放到to++指定的地址处
}                                                                        // 46/ 
                                                                         // 47/ 
int sys_signal(int signum, long handler, long restorer)                  // 48/ [b;]系统调用-为当前任务指定的的信号signum安装新的sigaction结构(新的信号句柄handler、空的信号屏蔽码、信号句柄一旦被调用过就恢复到默认处理句柄；不阻止在指定的信号处理程序中再收到该信号)并返回旧的信号句柄。(参数restorer是恢复函数指针，由Libc提供，用于清理用户态堆栈)
{                                                                        // 49/ 
	struct sigaction tmp;                                            // 50/ 
                                                                         // 51/ 
	if (signum<1 || signum>32 || signum==SIGKILL)                    // 52/ 保证信号值在有效范围内(1-32)，并且不得是SIGKILL，(奇怪没考虑SIGSTOP，因为SIGKILL和SIGSTOP都不能被进程捕获，可能是do_signal还不能处理SIGSTOP等信号)
		return -1;                                               // 53/ 
	tmp.sa_handler = (void (*)(int)) handler;                        // 54/ 54-57行根据参数建立一个临时的sigaction结构
	tmp.sa_mask = 0;                                                 // 55/ 
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;                           // 56/ 信号句柄一旦被调用过就恢复到默认处理句柄；不阻止在指定的信号处理程序中再收到该信号
	tmp.sa_restorer = (void (*)(void)) restorer;                     // 57/ 
	handler = (long) current->sigaction[signum-1].sa_handler;        // 58/ 取出旧的信号句柄用于返回
	current->sigaction[signum-1] = tmp;                              // 59/ 将新建的tmp覆盖当前任务中对应信号的sigaction结构
	return handler;                                                  // 60/ 
}                                                                        // 61/ 
                                                                         // 62/ 
int sys_sigaction(int signum, const struct sigaction * action,           // 63/ [b;]系统调用-将新的sigaction结构action赋给当前任务中的信号signum对应的sigaction结构，并根据新sigaction结构中的sa_flags设置信号屏蔽码sa_mask；如果oldaction不为空，则还要把当前任务中的信号signum对应的旧的sigaction结构赋给oldaction
	struct sigaction * oldaction)                                    // 64/ 
{                                                                        // 65/ 
	struct sigaction tmp;                                            // 66/ 
                                                                         // 67/ 
	if (signum<1 || signum>32 || signum==SIGKILL)                    // 68/ 保证信号值在有效范围内(1-32)，并且不得是SIGKILL，(奇怪没考虑SIGSTOP，因为SIGKILL和SIGSTOP都不能被进程捕获，可能是do_signal还不能处理SIGSTOP等信号)
		return -1;                                               // 69/ 
	tmp = current->sigaction[signum-1];                              // 70/ 
	get_new((char *) action,                                         // 71/ 
		(char *) (signum-1+current->sigaction));                 // 72/ 
	if (oldaction)                                                   // 73/ 
		save_old((char *) &tmp,(char *) oldaction);              // 74/ 
	if (current->sigaction[signum-1].sa_flags & SA_NOMASK)           // 75/ 如果允许信号在自己的信号处理程序中再被收到
		current->sigaction[signum-1].sa_mask = 0;                // 76/ 设信号屏蔽码sa_mask为0，即在信号处理程序中不阻塞该信号
	else                                                             // 77/ 
		current->sigaction[signum-1].sa_mask |= (1<<(signum-1)); // 78/ 否则将该信号加入信号屏蔽码sa_mask，即在信号处理程序中阻塞该信号
	return 0;                                                        // 79/ 
}                                                                        // 80/ 
                                                                         // 81/ 
void do_signal(long signr,long eax, long ebx, long ecx, long edx,        // 82/ [b;]将信号处理程序插入到用户程序栈中，并在本系统调用结束(即中断返回)后立即执行信号处理程序，然后继续执行用户的程序(此函数处理比较粗略，还不能处理SIGSTOP等信号)
	long fs, long es, long ds,                                       // 83/ 
	long eip, long cs, long eflags,                                  // 84/ 
	unsigned long * esp, long ss)                                    // 85/ 
{                                                                        // 86/ 
	unsigned long sa_handler;                                        // 87/ 
	long old_eip=eip;                                                // 88/ 保存旧eip的值，之后压入用户态栈中，用于在信号处理程序执行完后跳转继续执行用户程序
	struct sigaction * sa = current->sigaction + signr - 1;          // 89/ 将当前任务中信号signr的sigaction结构数据赋给sa
	int longs;                                                       // 90/ 
	unsigned long * tmp_esp;                                         // 91/ 
                                                                         // 92/ 
	sa_handler = (unsigned long) sa->sa_handler;                     // 93/ 取当前任务中信号signr的sigaction结构的信号句柄
	if (sa_handler==1)                                               // 94/ 如果句柄为SIG_IGN(1,默认忽略句柄)，则直接返回
		return;                                                  // 95/ 
	if (!sa_handler) {                                               // 96/ 如果句柄为SIG_DEL(0,默认处理)
		if (signr==SIGCHLD)                                      // 97/ 如果信号是SIGCHLD，则直接返回
			return;                                          // 98/ 
		else                                                     // 99/ 如果是其他信号，则终止当前进程，调度执行其他任务
			do_exit(1<<(signr-1));                           //100/ 
	}                                                                //101/ 
	if (sa->sa_flags & SA_ONESHOT)                                   //102/ 如果该信号句柄只需使用一次，则将该句柄置空
		sa->sa_handler = NULL;                                   //103/ 
	*(&eip) = sa_handler;                                            //104/ 将内核态栈中旧的eip位置的内容改为当前任务中信号signr的sigaction结构的信号句柄，使得中断返回后先执行eip指向的信号处理程序，之后再继续执行用户程序
	longs = (sa->sa_flags & SA_NOMASK)?7:8;                          //105/ 根据是否允许在信号处理程序中收到信号自己，在用户态栈中开辟7/8个长字的空间
	*(&esp) -= longs;                                                //106/ *(&esp)与esp的区别可能是告诉编译器更改指定的栈中的内容，而不是只更改临时寄存器
	verify_area(esp,longs*4);                                        //107/ 对当前任务的逻辑地址从esp到esp+longs*4这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
	tmp_esp=esp;                                                     //108/ 
	put_fs_long((long) sa->sa_restorer,tmp_esp++);                   //109/ 109-117行用于在新开辟的用户态栈(fs此时指向任务自身的数据段/栈段)中保存——sa_restorer(用于清理在用户态栈中开辟出来的7/8个长字空间)、信号signr、当前任务的信号屏蔽位图blocked、系统调用处理函数的返回值eax、刚进system_call时压入的寄存器ecx和edx、中断压入的eflags和旧的eip值(信号处理程序返回到用户程序继续执行时使用)
	put_fs_long(signr,tmp_esp++);                                    //110/ 
	if (!(sa->sa_flags & SA_NOMASK))                                 //111/ 
		put_fs_long(current->blocked,tmp_esp++);                 //112/ 
	put_fs_long(eax,tmp_esp++);                                      //113/ 
	put_fs_long(ecx,tmp_esp++);                                      //114/ 
	put_fs_long(edx,tmp_esp++);                                      //115/ 
	put_fs_long(eflags,tmp_esp++);                                   //116/ 
	put_fs_long(old_eip,tmp_esp++);                                  //117/ 
	current->blocked |= sa->sa_mask;                                 //118/ 将进程阻塞码(类似全局有效)添上当前任务中信号signr的sigaction结构中的屏蔽码(类似局部有效，即只在该信号的信号处理程序中有效)
}                                                                        //119/ 

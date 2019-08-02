/*                                                                      //  1/ 
 *  linux/kernel/exit.c                                                 //  2/ 
 *                                                                      //  3/ 
 *  (C) 1991  Linus Torvalds                                            //  4/ 
 */                                                                     //  5/ 
                                                                        //  6/ 
#include <errno.h>                                                      //  7/ 
#include <signal.h>                                                     //  8/ 
#include <sys/wait.h>                                                   //  9/ 
                                                                        // 10/ 
#include <linux/sched.h>                                                // 11/ 
#include <linux/kernel.h>                                               // 12/ 
#include <linux/tty.h>                                                  // 13/ 
#include <asm/segment.h>                                                // 14/ 
                                                                        // 15/ 
int sys_pause(void);                                                    // 16/ 
int sys_close(int fd);                                                  // 17/ 
                                                                        // 18/ 
void release(struct task_struct * p)                                    // 19/ [b;]释放指定进程占用的任务槽及其任务数据结构占用的内存页面(可能内核态栈也随之释放了)
{                                                                       // 20/ 
	int i;                                                          // 21/ 
                                                                        // 22/ 
	if (!p)                                                         // 23/ 如果参数为NULL，直接返回
		return;                                                 // 24/ 
	for (i=1 ; i<NR_TASKS ; i++)                                    // 25/ 遍寻63(1-63)个任务槽
		if (task[i]==p) {                                       // 26/ 如果其中有一个槽里放着参数p指向的任务
			task[i]=NULL;                                   // 27/ 清空该任务槽
			free_page((long)p);                             // 28/ 释放物理地址p指向的位置所在处的1页面内存,即释放参数p指向的任务数据结构占用的内存页面(可能内核态栈也随之释放了)
			schedule();                                     // 29/ 重新调度
			return;                                         // 30/ 
		}                                                       // 31/ 
	panic("trying to release non-existent task");                   // 32/ 如果63个槽中都没有指定的任务，直接死机
}                                                                       // 33/ 
                                                                        // 34/ 
static inline int send_sig(long sig,struct task_struct * p,int priv)    // 35/ [b;]向p指定的任务发送信号sig，权限为priv(是强制发送标志，非0时表示不需要考虑进程用户属性或级别而能发送信号的权利)
{                                                                       // 36/ 
	if (!p || sig<1 || sig>32)                                      // 37/ 若任务指针为空，或信号不再有效范围，直接返回
		return -EINVAL;                                         // 38/ 
	if (priv || (current->euid==p->euid) || suser())                // 39/ 如果强制发送标志有效、或者当前进程的有效标识符就是指定进程的euid、或者当前进程是超级用户进程
		p->signal |= (1<<(sig-1));                              // 40/ 则在进程的信号位图中添加该信号
	else                                                            // 41/ 
		return -EPERM;                                          // 42/ 
	return 0;                                                       // 43/ 
}                                                                       // 44/ 
                                                                        // 45/ 
static void kill_session(void)                                          // 46/ [b;]终止当前进程所属的会话
{                                                                       // 47/ 
	struct task_struct **p = NR_TASKS + task;                       // 48/ 
	                                                                // 49/ 
	while (--p > &FIRST_TASK) {                                     // 50/ 遍寻63(1-63)个任务槽
		if (*p && (*p)->session == current->session)            // 51/ 找出进程号等于当前进程所属会话号的进程
			(*p)->signal |= 1<<(SIGHUP-1);                  // 52/ 向该进程发送挂断信号SIGHUP，以终止当前会话
	}                                                               // 53/ 
}                                                                       // 54/ 
                                                                        // 55/ 
/*                                                                      // 56/ 
 * XXX need to check permissions needed to send signals to process      // 57/ 
 * groups, etc. etc.  kill() permissions semantics are tricky!          // 58/ 
 */                                                                     // 59/ 
int sys_kill(int pid,int sig)                                           // 60/ [b;]根据pid的值，向不同的进程发送信号sig
{                                                                       // 61/ 
	struct task_struct **p = NR_TASKS + task;                       // 62/ 
	int err, retval = 0;                                            // 63/ 
                                                                        // 64/ 
	if (!pid) while (--p > &FIRST_TASK) {                           // 65/ 若pid == 0，表明当前进程是进程组组长，则向该进程组中的所有进程发送信号sig
		if (*p && (*p)->pgrp == current->pid)                   // 66/ 
			if (err=send_sig(sig,*p,1))                     // 67/ 
				retval = err;                           // 68/ 
	} else if (pid>0) while (--p > &FIRST_TASK) {                   // 69/ 若pid > 0，则向进程号是pid的进程发送信号sig
		if (*p && (*p)->pid == pid)                             // 70/ 
			if (err=send_sig(sig,*p,0))                     // 71/ 
				retval = err;                           // 72/ 
	} else if (pid == -1) while (--p > &FIRST_TASK)                 // 73/ 若pid == -1，则向除任务0(初始任务)之外的所有进程发送信号sig
		if (err = send_sig(sig,*p,0))                           // 74/ 
			retval = err;                                   // 75/ 
	else while (--p > &FIRST_TASK)                                  // 76/ 若pid < -1，则向进程组号为-pid的所有进程发送信号sig
		if (*p && (*p)->pgrp == -pid)                           // 77/ 
			if (err = send_sig(sig,*p,0))                   // 78/ 
				retval = err;                           // 79/ 
	return retval;                                                  // 80/ 
}                                                                       // 81/ 
                                                                        // 82/ 
static void tell_father(int pid)                                        // 83/ [b;]向进程号为pid的父进程发送SIGCHLD信号，若父进程先行终止，则当前进程自己释放
{                                                                       // 84/ 
	int i;                                                          // 85/ 
                                                                        // 86/ 
	if (pid)                                                        // 87/ 
		for (i=0;i<NR_TASKS;i++) {                              // 88/ 遍寻64(0-63)个任务槽
			if (!task[i])                                   // 89/ 
				continue;                               // 90/ 
			if (task[i]->pid != pid)                        // 91/ 寻找进程号为pid的父进程
				continue;                               // 92/ 
			task[i]->signal |= (1<<(SIGCHLD-1));            // 93/ 若找到该父进程，则在父进程的信号位图中添加SIGCHLD信号，之后返回
			return;                                         // 94/ 
		}                                                       // 95/ 
/* if we don't find any fathers, we just release ourselves */           // 96/ 
/* This is not really OK. Must change it to make father 1 */            // 97/ 
	printk("BAD BAD - no father found\n\r");                        // 98/ 
	release(current);                                               // 99/ 若未找到该父进程(父进程已先行终止)，则释放当前进程占用的任务槽及其任务数据结构占用的内存页面(可能内核态栈也随之释放了)
}                                                                       //100/ 
                                                                        //101/ 
int do_exit(long code)                                                  //102/ [b;]终止当前任务，调度执行其他任务
{                                                                       //103/ 
	int i;                                                          //104/ 
                                                                        //105/ 
	free_page_tables(get_base(current->ldt[1]),get_limit(0x0f));    //106/ 将当前任务的代码段线性地址映射用到的页目录项清空、页表项清空、物理页面释放
	free_page_tables(get_base(current->ldt[2]),get_limit(0x17));    //107/ 将当前任务的数据段线性地址映射用到的页目录项清空、页表项清空、物理页面释放
	for (i=0 ; i<NR_TASKS ; i++)                                    //108/ 遍寻64(0-63)个任务槽
		if (task[i] && task[i]->father == current->pid) {       //109/ 如果某个进程是当前进程的子进程，就将该进程的父进程改为进程1，即init进程
			task[i]->father = 1;                            //110/ 			
			if (task[i]->state == TASK_ZOMBIE)              //111/ 如果该进程的状态是僵死状态，则向进程1强制发送终止信号SIGCHLD
				/* assumption task[1] is always init */ //112/ 
				(void) send_sig(SIGCHLD, task[1], 1);   //113/ 
		}                                                       //114/ 
	for (i=0 ; i<NR_OPEN ; i++)                                     //115/ [r;]115-117行关闭当前进程打开着的所有文件
		if (current->filp[i])                                   //116/ 
			sys_close(i);                                   //117/ 
	iput(current->pwd);                                             //118/ [r;]118-125
	current->pwd=NULL;                                              //119/ 
	iput(current->root);                                            //120/ 
	current->root=NULL;                                             //121/ 
	iput(current->executable);                                      //122/ 
	current->executable=NULL;                                       //123/ 
	if (current->leader && current->tty >= 0)                       //124/ 
		tty_table[current->tty].pgrp = 0;                       //125/ 
	if (last_task_used_math == current)                             //126/ 如果上个使用过协处理器的任务是当前任务，则将其清空
		last_task_used_math = NULL;                             //127/ 
	if (current->leader)                                            //128/ 如果当前进程是会话首进程，则终止当前进程所属的会话
		kill_session();                                         //129/ 
	current->state = TASK_ZOMBIE;                                   //130/ 将当前进程状态置为僵死状态
	current->exit_code = code;                                      //131/ 将参数code赋给当前进程的终止退出码
	tell_father(current->father);                                   //132/ 向当前进程的父进程发送SIGCHLD信号，若父进程先行终止，则当前进程自己释放
	schedule();                                                     //133/ 重新调度
	return (-1);	/* just to suppress warnings */                 //134/ 
}                                                                       //135/ 
                                                                        //136/ 
int sys_exit(int error_code)                                            //137/ [b;]系统调用-终止当前任务，调度执行其他任务(error_code是用户程序提供的退出状态信息，只有低字节有效，左移8位是wait或waitpid函数的要求，低字节用来保存wait()的状态信息)
{                                                                       //138/ 
	return do_exit((error_code&0xff)<<8);                           //139/ 
}                                                                       //140/ 
                                                                        //141/ 
int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options)       //142/ [b;]挂起当前进程，直到pid指定的子进程退出 或者收到要求终止该进程的信号 或者需要调用一个信号句柄
{                                                                       //143/ 
	int flag, code;                                                 //144/ 
	struct task_struct ** p;                                        //145/ 
                                                                        //146/ 
	verify_area(stat_addr,4);                                       //147/ 对当前任务的逻辑地址从stat_addr到stat_addr+4这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
repeat:                                                                 //148/ 
	flag=0;                                                         //149/ 
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p) {                   //150/ 遍寻63(1-63)个任务槽
		if (!*p || *p == current)                               //151/ 屏蔽掉空任务或当前任务
			continue;                                       //152/ 
		if ((*p)->father != current->pid)                       //153/ 筛选出当前任务的子进程
			continue;                                       //154/ 
		if (pid>0) {                                            //155/ 如果pid > 0，锁定当前进程的子进程中进程号为pid的子进程
			if ((*p)->pid != pid)                           //156/ 
				continue;                               //157/ 
		} else if (!pid) {                                      //158/ 如果pid == 0，锁定当前进程的子进程中与当前进程同属一个组的子进程
			if ((*p)->pgrp != current->pgrp)                //159/ 
				continue;                               //160/ 
		} else if (pid != -1) {                                 //161/ 如果pid < -1，锁定当前进程的子进程中进程组号为-pid的子进程
			if ((*p)->pgrp != -pid)                         //162/ 
				continue;                               //163/ 
		}                                                       //164/ 如果pid == -1，锁定当前进程的任何子进程
		switch ((*p)->state) {                                  //165/ 
			case TASK_STOPPED:                              //166/ 当锁定的进程处于停止状态
				if (!(options & WUNTRACED))             //167/ 如果参数options中设置了WUNTRACED(表示子进程是停止的也马上返回，报告停止执行的子进程状态)
					continue;                       //168/ 
				put_fs_long(0x7f,stat_addr);            //169/ 则将一长字0x7f(状态信息)存放在fs段(fs此时指向任务自身的数据段/栈段)中stat_addr(参数二)指定的内存地址处
				return (*p)->pid;                       //170/ 返回锁定进程的进程号
			case TASK_ZOMBIE:                               //171/ 当锁定的进程处于僵死状态
				current->cutime += (*p)->utime;         //172/ 将锁定进程的用户态时间加到其父进程(即当前进程)的子进程用户态时间上
				current->cstime += (*p)->stime;         //173/ 将锁定进程的内核态时间加到其父进程(即当前进程)的子进程内核态时间上
				flag = (*p)->pid;                       //174/ 取出锁定进程的pid放入flag
				code = (*p)->exit_code;                 //175/ 取出锁定进程的退出码放入code
				release(*p);                            //176/ 释放锁定进程占用的任务槽及其任务数据结构占用的内存页面(可能内核态栈也随之释放了)
				put_fs_long(code,stat_addr);            //177/ 将锁定进程的退出码存放在fs段(fs此时指向任务自身的数据段/栈段)中stat_addr(参数二)指定的内存地址处
				return flag;                            //178/ 返回锁定进程的pid
			default:                                        //179/ 当锁定的进程处于其他状态
				flag=1;                                 //180/ 置flag=1，表示找到过符合上面要求的子进程，但都处于运行态或睡眠态
				continue;                               //181/ 
		}                                                       //182/ 
	}                                                               //183/ 
	if (flag) {                                                     //184/ 如果找到过符合上面要求的子进程，但都处于运行态或睡眠态
		if (options & WNOHANG)                                  //185/ 如果参数options中设置了WNOHANG(表示若没有子进程处于退出或终止状态就立刻返回)
			return 0;                                       //186/ 则立刻返回
		current->state=TASK_INTERRUPTIBLE;                      //187/ 如果未设置WNOHANG，则置当前进程为可中断睡眠状态
		schedule();                                             //188/ 重新调度
		if (!(current->signal &= ~(1<<(SIGCHLD-1))))            //189/ 当本进程被唤醒又开始执行时，若本进程没有收到除SIGCHLD外的其他信号，则重复处理
			goto repeat;                                    //190/ 
		else                                                    //191/ 当本进程被唤醒又开始执行时，若收到了其他信号，则返回出错码(中断的系统调用)
			return -EINTR;                                  //192/ 
	}                                                               //193/ 
	return -ECHILD;                                                 //194/ 如果根本没有找到过符合上面要求的子进程，则flag=0，返回出错码(子进程不存在)
}                                                                       //195/ 
                                                                        //196/ 
                                                                        //197/ 

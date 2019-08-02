#ifndef _SCHED_H                                                              //  1/ 
#define _SCHED_H                                                              //  2/ 
                                                                              //  3/ 
#define NR_TASKS 64                                                           //  4/ 
#define HZ 100                                                                //  5/ 
                                                                              //  6/ 
#define FIRST_TASK task[0]                                                    //  7/ 系统中第一个任务槽里面的任务
#define LAST_TASK task[NR_TASKS-1]                                            //  8/ 系统中最后一个任务槽里面的任务
                                                                              //  9/ 
#include <linux/head.h>                                                       // 10/ 
#include <linux/fs.h>                                                         // 11/ 
#include <linux/mm.h>                                                         // 12/ 
#include <signal.h>                                                           // 13/ 
                                                                              // 14/ 
#if (NR_OPEN > 32)                                                            // 15/ 
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc" // 16/ 
#endif                                                                        // 17/ 
                                                                              // 18/ 
#define TASK_RUNNING		0                                             // 19/ 就绪态
#define TASK_INTERRUPTIBLE	1                                             // 20/ 可中断睡眠状态
#define TASK_UNINTERRUPTIBLE	2                                             // 21/ 不可中断睡眠状态
#define TASK_ZOMBIE		3                                             // 22/ 僵死状态
#define TASK_STOPPED		4                                             // 23/ 停止状态
                                                                              // 24/ 
#ifndef NULL                                                                  // 25/ 
#define NULL ((void *) 0)                                                     // 26/ 
#endif                                                                        // 27/ 
                                                                              // 28/ 
extern int copy_page_tables(unsigned long from, unsigned long to, long size); // 29/ 
extern int free_page_tables(unsigned long from, unsigned long size);          // 30/ 
                                                                              // 31/ 
extern void sched_init(void);                                                 // 32/ 
extern void schedule(void);                                                   // 33/ 
extern void trap_init(void);                                                  // 34/ 
extern void panic(const char * str);                                          // 35/ 
extern int tty_write(unsigned minor,char * buf,int count);                    // 36/ 
                                                                              // 37/ 
typedef int (*fn_ptr)();                                                      // 38/ 
                                                                              // 39/ 
struct i387_struct {                                                          // 40/ 40-48行的结构用于保存协处理器80387的状态
	long	cwd;                                                          // 41/ 控制字
	long	swd;                                                          // 42/ 状态字
	long	twd;                                                          // 43/ 标记字
	long	fip;                                                          // 44/ 协处理器代码指针
	long	fcs;                                                          // 45/ 协处理器代码段寄存器
	long	foo;                                                          // 46/ 内存操作数的偏移位置
	long	fos;                                                          // 47/ 内存操作数的段值
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */   // 48/ 8个10字节的协处理器累加器
};                                                                            // 49/ 
                                                                              // 50/ 
struct tss_struct {                                                           // 51/ 
	long	back_link;	/* 16 high bits zero */                       // 52/ 保存前一个任务的TSS段选择符
	long	esp0;                                                         // 53/ 静态只读，保存任务内核栈的栈指针值
	long	ss0;		/* 16 high bits zero */                       // 54/ 静态只读，保存任务内核栈的栈段选择符
	long	esp1;                                                         // 55/ 静态只读，Linux0.11内核不使用
	long	ss1;		/* 16 high bits zero */                       // 56/ 静态只读，Linux0.11内核不使用
	long	esp2;                                                         // 57/ 静态只读，Linux0.11内核不使用
	long	ss2;		/* 16 high bits zero */                       // 58/ 静态只读，Linux0.11内核不使用
	long	cr3;                                                          // 59/ 静态只读，保存任务页目录表的基地址
	long	eip;                                                          // 60/ 保存任务对应寄存器的值
	long	eflags;                                                       // 61/ 保存任务对应寄存器的值
	long	eax,ecx,edx,ebx;                                              // 62/ 保存任务对应寄存器的值
	long	esp;                                                          // 63/ 保存任务用户栈的栈指针值
	long	ebp;                                                          // 64/ 保存任务对应寄存器的值
	long	esi;                                                          // 65/ 保存任务对应寄存器的值
	long	edi;                                                          // 66/ 保存任务对应寄存器的值
	long	es;		/* 16 high bits zero */                       // 67/ 保存任务对应寄存器的值
	long	cs;		/* 16 high bits zero */                       // 68/ 保存任务对应寄存器的值
	long	ss;		/* 16 high bits zero */                       // 69/ 保存任务用户栈的栈段选择符
	long	ds;		/* 16 high bits zero */                       // 70/ 保存任务对应寄存器的值
	long	fs;		/* 16 high bits zero */                       // 71/ 保存任务对应寄存器的值
	long	gs;		/* 16 high bits zero */                       // 72/ 保存任务对应寄存器的值
	long	ldt;		/* 16 high bits zero */                       // 73/ 静态只读，保存任务的LDT段选择符
	long	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */             // 74/ 静态只读，位0保存软件调试位，位16-31保存IO映射区基地址
	struct i387_struct i387;                                              // 75/ 用于保存协处理器80387的状态
};                                                                            // 76/ 
                                                                              // 77/ 
struct task_struct {                                                          // 78/ 任务数据结构(即进程描述符)
/* these are hardcoded - don't touch */                                       // 79/ 
	long state;	/* -1 unrunnable, 0 runnable, >0 stopped */           // 80/ 进程的当前状态
	long counter;                                                         // 81/ 进程在被暂时停止本次运行之前还能运行的时间滴答数，即正常情况还要多久才切换到另一进程
	long priority;                                                        // 82/ 优先权，用于给counter赋值
	long signal;                                                          // 83/ 进程当前所收到的信号的位图，linux内核最多有32个信号
	struct sigaction sigaction[32];                                       // 84/ 保存处理各信号所使用的操作和属性，每一项对应一个信号
	long blocked;	/* bitmap of masked signals */                        // 85/ 进程当前不想处理的信号的阻塞位图
/* various fields */                                                          // 86/ 
	int exit_code;                                                        // 87/ 保存进程终止时的退出码，可供父进程查阅
	unsigned long start_code,end_code,end_data,brk,start_stack;           // 88/ 进程线性地址空间的代码开始位置、代码字节长度值、代码+数据字节长度值、代码+数据+bss字节长度值、栈开始位置
	long pid,father,pgrp,session,leader;                                  // 89/ 进程号、父进程号、所属进程组号、进程的会话号、leader非0表示该进程是会话首进程(一般是shell进程)
	unsigned short uid,euid,suid;                                         // 90/ 拥有该进程的用户标识号、有效用户标识号(指明访问文件的能力)、保存的用户标识号(当执行文件的设置用户ID标志置位时，suid中保存着执行文件的uid。否则suid等于进程的euid)
	unsigned short gid,egid,sgid;                                         // 91/ 用户所属组标识号(指明拥有该进程的用户组)、有效组标识号(指明该组用户访问文件的权限)、保存的用户组标识号(当执行文件的设置组ID标志置位时，sgid中保存着执行文件的gid。否则sgid等于进程的egid)
	long alarm;                                                           // 92/ 报警的定时值(滴答数)
	long utime,stime,cutime,cstime,start_time;                            // 93/ 累计进程在用户态运行的时间(滴答数)、累计进程在内核态运行的时间(滴答数)、已终止子进程用户态时间(滴答数，可能是已终止的子进程的用户态时间总和)、已终止子进程内核态时间(滴答数，可能是已终止的子进程的内核态时间总和)、进程生成并开始运行的时刻
	unsigned short used_math;                                             // 94/ 标志是否使用了协处理器
/* file system info */                                                        // 95/ 
	int tty;		/* -1 if no tty, so it must be signed */      // 96/ 进程使用tty终端的子设备号，-1表示没有使用
	unsigned short umask;                                                 // 97/ 进程创建新文件时使用的属性屏蔽位，即新建文件所设置的访问属性
	struct m_inode * pwd;                                                 // 98/ 进程当前的工作目录i节点结构指针，用于解析相对路径名
	struct m_inode * root;                                                // 99/ 进程自己的根目录i节点结构指针，用于解析绝对路径名
	struct m_inode * executable;                                          //100/ 进程运行的可执行文件在内存中i节点结构指针，可用来判断是否有另一个进程也在执行该可执行文件
	unsigned long close_on_exec;                                          //101/ 进程文件描述符的位图标志，每个位代表一个文件描述符，用于在调用execve()时需要关闭的文件描述符
	struct file * filp[NR_OPEN];                                          //102/ 进程使用的所有打开文件的文件结构指针表，最多32项，文件描述符的值即是该结构中的索引值
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */                             //103/ 
	struct desc_struct ldt[3];                                            //104/ 进程的局部描述符表结构，定义了该任务在虚拟地址空间中的代码段和数据段。项0是空项、项1是代码段描述符、项2是数据段描述符(包括数据和堆栈)
/* tss for this task */                                                       //105/ 
	struct tss_struct tss;                                                //106/ 进程的任务状态段TSS
};                                                                            //107/ 
                                                                              //108/ 
/*                                                                            //109/ 
 *  INIT_TASK is used to set up the first task table, touch at                //110/ 
 * your own risk!. Base=0, limit=0x9ffff (=640kB)                             //111/ 
 */                                                                           //112/ 
#define INIT_TASK \                                                           //113/ 113-134行用于定义初始任务(即任务0)的任务数据结构
/* state etc */	{ 0,15,15, \                                                  //114/ 
/* signals */	0,{{},},0, \                                                  //115/ 
/* ec,brk... */	0,0,0,0,0,0, \                                                //116/ 
/* pid etc.. */	0,-1,0,0,0, \                                                 //117/ 
/* uid etc */	0,0,0,0,0,0, \                                                //118/ 
/* alarm */	0,0,0,0,0,0, \                                                //119/ 
/* math */	0, \                                                          //120/ 
/* fs info */	-1,0022,NULL,NULL,NULL,0, \                                   //121/ 
/* filp */	{NULL,}, \                                                    //122/ 
	{ \                                                                   //123/ 
		{0,0}, \                                                      //124/ 
/* ldt */	{0x9f,0xc0fa00}, \                                            //125/ 
		{0x9f,0xc0f200}, \                                            //126/ 
	}, \                                                                  //127/ 
/*tss*/	{0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir,\            //128/ 
	 0,0,0,0,0,0,0,0, \                                                   //129/ 
	 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \                                 //130/ 
	 _LDT(0),0x80000000, \                                                //131/ 
		{} \                                                          //132/ 
	}, \                                                                  //133/ 
}                                                                             //134/ 
                                                                              //135/ 
extern struct task_struct *task[NR_TASKS];                                    //136/ 
extern struct task_struct *last_task_used_math;                               //137/ 
extern struct task_struct *current;                                           //138/ 
extern long volatile jiffies;                                                 //139/ 
extern long startup_time;                                                     //140/ 
                                                                              //141/ 
#define CURRENT_TIME (startup_time+jiffies/HZ)                                //142/ 当前时间，即从1970.1.1:0:0:0开始到此时此刻经过的秒数
                                                                              //143/ 
extern void add_timer(long jiffies, void (*fn)(void));                        //144/ 
extern void sleep_on(struct task_struct ** p);                                //145/ 
extern void interruptible_sleep_on(struct task_struct ** p);                  //146/ 
extern void wake_up(struct task_struct ** p);                                 //147/ 
                                                                              //148/ 
/*                                                                            //149/ 
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall       //150/ 
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...                                             //151/ 
 */                                                                           //152/ (GDT中，0-nul, 1-cs, 2-ds, 3-syscall，4-TSS0, 5-LDT0, 6-TSS1 etc ...)
#define FIRST_TSS_ENTRY 4                                                     //153/ GDT中第1个TSS描述符的选择符索引号
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)                                   //154/ GDT中第1个LDT描述符的选择符索引号
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))               //155/ 根据任务号n计算在GDT中第n+1个任务的TSS段描述符的选择符值的偏移量部分，低三位为000
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))               //156/ 根据任务号n计算在GDT中第n+1个任务的LDT段描述符的选择符值的偏移量部分，低三位为000
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))                             //157/ 根据任务号n把第n+1个任务的TSS段选择符加载到任务寄存器TR中
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))                           //158/ 根据任务号n把第n+1个任务的LDT段选择符加载到局部描述赋表寄存器LDTR中
#define str(n) \                                                              //159/ 159-164行用于将当前运行任务的任务号取出存入n中
__asm__("str %%ax\n\t" \                                                      //160/ 将TR中的TSS段选择符保存到寄存器ax中
	"subl %2,%%eax\n\t" \                                                 //161/ 
	"shrl $4,%%eax" \                                                     //162/ 
	:"=a" (n) \                                                           //163/ 
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3))                                    //164/ 
/*                                                                            //165/ 
 *	switch_to(n) should switch tasks to task nr n, first                  //166/ 
 * checking that n isn't the current task, in which case it does nothing.     //167/ 
 * This also clears the TS-flag if the task we switched to has used           //168/ 
 * tha math co-processor latest.                                              //169/ 
 */                                                                           //170/ 
#define switch_to(n) {\                                                       //171/ 171-184行用于将任务切换到新任务(即由任务号n指定的任务)
struct {long a,b;} __tmp; \                                                   //172/ 
__asm__("cmpl %%ecx,_current\n\t" \                                           //173/ 判断任务号n指定的任务是否就是当前任务
	"je 1f\n\t" \                                                         //174/ 如果是就直接退出
	"movw %%dx,%1\n\t" \                                                  //175/ 将新任务的TSS的16位选择符存入__tmp.b中
	"xchgl %%ecx,_current\n\t" \                                          //176/ 使得current指向新任务，ecx指向被切换出去的旧任务
	"ljmp %0\n\t" \                                                       //177/ [b;]‘ljmp tss段选择符(__tmp.b) 偏移值(__tmp.a中，忽略不使用)’，切换到新任务去执行
	"cmpl %%ecx,_last_task_used_math\n\t" \                               //178/ [b;]当任务切换回来时从此处接着执行，判断旧任务上次使用过协处理器吗
	"jne 1f\n\t" \                                                        //179/ 没有使用则跳转退出
	"clts\n" \                                                            //180/ 如果使用过协处理器，则清cr0中的任务切换标志TS
	"1:" \                                                                //181/ 
	::"m" (*&__tmp.a),"m" (*&__tmp.b), \                                  //182/ 
	"d" (_TSS(n)),"c" ((long) task[n])); \                                //183/ 
}                                                                             //184/ 
                                                                              //185/ 
#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)                                //186/ 
                                                                              //187/ 
#define _set_base(addr,base) \                                                //188/ 将地址addr处的描述符中的段基地值字段设为base
__asm__("movw %%dx,%0\n\t" \                                                  //189/ 
	"rorl $16,%%edx\n\t" \                                                //190/ 
	"movb %%dl,%1\n\t" \                                                  //191/ 
	"movb %%dh,%2" \                                                      //192/ 
	::"m" (*((addr)+2)), \                                                //193/ 
	  "m" (*((addr)+4)), \                                                //194/ 
	  "m" (*((addr)+7)), \                                                //195/ 
	  "d" (base) \                                                        //196/ 
	:"dx")                                                                //197/ 告诉编译器edx寄存器的值已被嵌入式汇编程序改变了
                                                                              //198/ 
#define _set_limit(addr,limit) \                                              //199/ 将地址addr处的描述符中的段限长字段设为limit
__asm__("movw %%dx,%0\n\t" \                                                  //200/ 
	"rorl $16,%%edx\n\t" \                                                //201/ 
	"movb %1,%%dh\n\t" \                                                  //202/ 
	"andb $0xf0,%%dh\n\t" \                                               //203/ 
	"orb %%dh,%%dl\n\t" \                                                 //204/ 
	"movb %%dl,%1" \                                                      //205/ 
	::"m" (*(addr)), \                                                    //206/ 
	  "m" (*((addr)+6)), \                                                //207/ 
	  "d" (limit) \                                                       //208/ 
	:"dx")                                                                //209/ 告诉编译器edx寄存器的值已被嵌入式汇编程序改变了
                                                                              //210/ 
#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , base )               //211/ 将段描述符ldt(只是临时参数名，并非局部描述符表)中的段基地值字段设为base
#define set_limit(ldt,limit) _set_limit( ((char *)&(ldt)) , (limit-1)>>12 )   //212/ 将段描述符ldt(只是临时参数名，并非局部描述符表)中的段限长字段设为limit减一
                                                                              //213/ 
#define _get_base(addr) ({\                                                   //214/ [b;]将地址addr处的段描述符中的段基地址取出返回
unsigned long __base; \                                                       //215/ 
__asm__("movb %3,%%dh\n\t" \                                                  //216/ 
	"movb %2,%%dl\n\t" \                                                  //217/ 
	"shll $16,%%edx\n\t" \                                                //218/ 
	"movw %1,%%dx" \                                                      //219/ 
	:"=d" (__base) \                                                      //220/ 
	:"m" (*((addr)+2)), \                                                 //221/ 
	 "m" (*((addr)+4)), \                                                 //222/ 
	 "m" (*((addr)+7))); \                                                //223/ 
__base;})                                                                     //224/ 
                                                                              //225/ 
#define get_base(ldt) _get_base( ((char *)&(ldt)) )                           //226/ [b;]将段描述符ldt(只是临时参数名，并非局部描述符表)中段基地址取出返回
                                                                              //227/ 
#define get_limit(segment) ({ \                                               //228/ [b;]通过传入段选择子，将其指定的段描述符中的段界限字段值加一后返回
unsigned long __limit; \                                                      //229/ 
__asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \              //230/ lsll是加载段界限的指令，即把%1指定的段描述符中的段界限字段装入寄存器%0中
__limit;})                                                                    //231/ 
                                                                              //232/ 
#endif                                                                        //233/ 

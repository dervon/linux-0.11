/*                                                                            //  1/ 
 *  linux/kernel/sched.c                                                      //  2/ 
 *                                                                            //  3/ 
 *  (C) 1991  Linus Torvalds                                                  //  4/ 
 */                                                                           //  5/ 
                                                                              //  6/ 
/*                                                                            //  7/ 
 * 'sched.c' is the main kernel file. It contains scheduling primitives       //  8/ 
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system      //  9/ 
 * call functions (type getpid(), which just extracts a field from            // 10/ 
 * current-task                                                               // 11/ 
 */                                                                           // 12/ 
#include <linux/sched.h>                                                      // 13/ 
#include <linux/kernel.h>                                                     // 14/ 
#include <linux/sys.h>                                                        // 15/ 
#include <linux/fdreg.h>                                                      // 16/ 
#include <asm/system.h>                                                       // 17/ 
#include <asm/io.h>                                                           // 18/ 
#include <asm/segment.h>                                                      // 19/ 
                                                                              // 20/ 
#include <signal.h>                                                           // 21/ 
                                                                              // 22/ 
#define _S(nr) (1<<((nr)-1))                                                  // 23/ 取信号nr(1-32)在信号位图中对应位的二进制数值，如...001000b
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))                             // 24/ 定义所有可阻塞信号集合的二进制数值，入...1011 1111 1110 1111 1111b,除SIGKILL、SIGSTOP之外的其他信号都是可阻塞的
                                                                              // 25/ 
void show_task(int nr,struct task_struct * p)                                 // 26/ [b;]显示任务号nr对应的进程的进程号、进程状态及内核态栈中空闲(等于0)的字节数
{                                                                             // 27/ 
	int i,j = 4096-sizeof(struct task_struct);                            // 28/ 
                                                                              // 29/ 
	printk("%d: pid=%d, state=%d, ",nr,p->pid,p->state);                  // 30/ 
	i=0;                                                                  // 31/ 
	while (i<j && !((char *)(p+1))[i])                                    // 32/ 
		i++;                                                          // 33/ 
	printk("%d (of %d) chars free in kernel stack\n\r",i,j);              // 34/ 
}                                                                             // 35/ 
                                                                              // 36/ 
void show_stat(void)                                                          // 37/ [b;]显示64个任务槽中已存在的各个任务的任务号、进程号、进程状态以及内核态栈中空闲(等于0)的字节数
{                                                                             // 38/ 
	int i;                                                                // 39/ 
                                                                              // 40/ 
	for (i=0;i<NR_TASKS;i++)                                              // 41/ 
		if (task[i])                                                  // 42/ 
			show_task(i,task[i]);                                 // 43/ 
}                                                                             // 44/ 
                                                                              // 45/ 
#define LATCH (1193180/HZ)                                                    // 46/ 设置8253定时芯片通道0的初值
                                                                              // 47/ 
extern void mem_use(void);                                                    // 48/ 系统中没有任何地方定义和引用了该函数
                                                                              // 49/ 
extern int timer_interrupt(void);                                             // 50/ 时钟中断处理程序(定时芯片8253触发的)
extern int system_call(void);                                                 // 51/ 系统调用中断处理程序
                                                                              // 52/ 
union task_union {                                                            // 53/ 定义任务联合体，在一个页面内包含任务数据结构和任务的内核态堆栈结构
	struct task_struct task;                                              // 54/ 定义任务数据结构
	char stack[PAGE_SIZE];                                                // 55/ 定义任务内核态栈(#define PAGE_SIZE 4096)
};                                                                            // 56/ 
                                                                              // 57/ 
static union task_union init_task = {INIT_TASK,};                             // 58/ 定义初始任务(即任务0)联合体的初始化
                                                                              // 59/ 
long volatile jiffies=0;                                                      // 60/ 定义从开机开始算起到目前的滴答数，是全局变量，在时钟中断处理程序中递增，volatile阻止优化，使引用该变量时一定要从指定内存中取得其值，因为时钟中断过程等程序会修改它的值
long startup_time=0;                                                          // 61/ 从1970.1.1:0:0:0开始到系统刚开机时经过的秒数
struct task_struct *current = &(init_task.task);                              // 62/ 定义当前任务指针(初始化时指向任务0)
struct task_struct *last_task_used_math = NULL;                               // 63/ 指向上一个使用过协处理器的任务的指针
                                                                              // 64/ 
struct task_struct * task[NR_TASKS] = {&(init_task.task), };                  // 65/ 定义任务指针数组(初始化时将初始任务作为任务0)
                                                                              // 66/ 
long user_stack [ PAGE_SIZE>>2 ] ;                                            // 67/ 67-72用于定义任务0的用户态栈
                                                                              // 68/ 
struct {                                                                      // 69/ 
	long * a;                                                             // 70/ esp = a
	short b;                                                              // 71/ ss = b
	} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };               // 72/ 
/*                                                                            // 73/ 
 *  'math_state_restore()' saves the current math information in the          // 74/ 
 * old math state array, and gets the new ones from the current task          // 75/ 
 */                                                                           // 76/ 
void math_state_restore()                                                     // 77/ [b;]用于在任务切换时保存原任务的协处理器状态(上下文)并恢复新调度进来的当前任务的协处理器执行状态
{                                                                             // 78/ 
	if (last_task_used_math == current)                                   // 79/ 如果上个使用过协处理器的任务就是当前任务，直接返回
		return;                                                       // 80/ 
	__asm__("fwait");                                                     // 81/ 否则发送协处理器命令之前要先发送WAIT指令
	if (last_task_used_math) {                                            // 82/ 如果上个使用过协处理器的任务还存在，没有终止
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));    // 83/ 则为上次使用过协处理器的任务保存协处理器的全部状态到那个任务的任务数据结构中的.tss.i387中
	}                                                                     // 84/ 
	last_task_used_math=current;                                          // 85/ 因为当前进程要使用协处理器，所以将last_task_used_math指向当前任务
	if (current->used_math) {                                             // 86/ 如果当前任务使用过协处理器
		__asm__("frstor %0"::"m" (current->tss.i387));                // 87/ 将当前任务保存的协处理器状态恢复到协处理器中
	} else {                                                              // 88/ 
		__asm__("fninit"::);                                          // 89/ 否则说明是第一次使用，那么向协处理器发送初始化命令
		current->used_math=1;                                         // 90/ 将当前任务设置为已使用过协处理器
	}                                                                     // 91/ 
}                                                                             // 92/ 
                                                                              // 93/ 
/*                                                                            // 94/ 
 *  'schedule()' is the scheduler function. This is GOOD CODE! There          // 95/ 
 * probably won't be any reason to change this, as it should work well        // 96/ 
 * in all circumstances (ie gives IO-bound processes good response etc).      // 97/ 
 * The one thing you might take a look at is the signal-handler code here.    // 98/ 
 *                                                                            // 99/ 
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other       //100/ 
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'      //101/ 
 * information in task[0] is never used.                                      //102/ 任务0是个闲置任务，只有当没有其他任务可以运行时才调用它，
 */                                                                           //103/ 它不能被杀死，也不能睡眠，任务0中的state从来都不用
void schedule(void)                                                           //104/ [b;]进程调度程序
{                                                                             //105/ 
	int i,next,c;                                                         //106/ 
	struct task_struct ** p;                                              //107/ 
                                                                              //108/ 
/* check alarm, wake up any interruptible tasks that have got a signal */     //109/ 
                                                                              //110/ 
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)                           //111/ 遍寻系统中的63(1-63)个任务槽，0槽位除外
		if (*p) {                                                     //112/ 如果槽中有任务存在
			if ((*p)->alarm && (*p)->alarm < jiffies) {           //113/ 如果任务设置了报警定时，而且定时时间到了
					(*p)->signal |= (1<<(SIGALRM-1));     //114/ 将SIGALRM信号加入任务收到的信号的位图
					(*p)->alarm = 0;                      //115/ 清报警值alarm
				}                                             //116/ 
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) && //117/ 如果任务收到的信号中除掉阻塞的信号外还有其他信号，并且
			(*p)->state==TASK_INTERRUPTIBLE)                      //118/ 任务处于可中断睡眠状态(可由信号唤醒)
				(*p)->state=TASK_RUNNING;                     //119/ 那么将任务置为就绪态
		}                                                             //120/ 
                                                                              //121/ 
/* this is the scheduler proper: */                                           //122/ 
                                                                              //123/ 
	while (1) {                                                           //124/ 
		c = -1;                                                       //125/ 
		next = 0;                                                     //126/ 
		i = NR_TASKS;                                                 //127/ 
		p = &task[NR_TASKS];                                          //128/ 
		while (--i) {                                                 //129/ 遍寻系统中的63(1-63)个任务槽，0槽位除外
			if (!*--p)                                            //130/ 如果槽位中无任务，直接返回去检测下一个槽位
				continue;                                     //131/ 
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c) //132/ 比较所有就绪态任务的可运行时间滴答值
				c = (*p)->counter, next = i;                  //133/ 取出最大的可运行时间滴答值赋给c，将next指向其对应的任务
		}                                                             //134/ 
		if (c) break;                                                 //135/ 如果所有就绪态任务中有counter值大于0或者系统中没有就绪态的任务存在，则退出while(1)循环；如果系统中有就绪态任务存在，且它们的counter值都为0，则执行下面的for循环
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)                   //136/ 遍寻系统中的63(1-63)个任务槽，0槽位除外
			if (*p)                                               //137/ 如果槽中有任务，就根据任务的优先权priority将该任务的counter重置
				(*p)->counter = ((*p)->counter >> 1) +        //138/ 
						(*p)->priority;               //139/ 
	}                                                                     //140/ 
	switch_to(next);                                                      //141/ 切换到就绪态任务中counter值最大的那个任务，或者切换都任务0(所有就绪态任务的counter都为0或没有就绪态任务的情况下)
}                                                                             //142/ 
                                                                              //143/ 
int sys_pause(void)                                                           //144/ [b;]pause()系统调用，将当前任务的状态切换到可中断的睡眠状态，并重新调度
{                                                                             //145/ 
	current->state = TASK_INTERRUPTIBLE;                                  //146/ 
	schedule();                                                           //147/ 
	return 0;                                                             //148/ 
}                                                                             //149/ 
                                                                              //150/ 
void sleep_on(struct task_struct **p)                                         //151/ [b;]把当前任务置为不可中断的睡眠状态，让当前任务的tmp指向*p指向的旧睡眠队列头，而让*p指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up唤醒了*p指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
{                                                                             //152/ 
	struct task_struct *tmp;                                              //153/ 
                                                                              //154/ 
	if (!p)                                                               //155/ 若等待任务队列头指针无效，直接返回
		return;                                                       //156/ 
	if (current == &(init_task.task))                                     //157/ 如果当前任务是任务0，直接死机。因为任务0的运行不依赖state，将任务0置为睡眠状态没有意义
		panic("task[0] trying to sleep");                             //158/ 
	tmp = *p;                                                             //159/ 
	*p = current;                                                         //160/ 
	current->state = TASK_UNINTERRUPTIBLE;                                //161/ 
	schedule();                                                           //162/ 
	if (tmp)                                                              //163/ 
		tmp->state=0;                                                 //164/ 
}                                                                             //165/ 
                                                                              //166/ 
void interruptible_sleep_on(struct task_struct **p)                           //167/ [b;]把当前任务置为可中断的睡眠状态，让当前任务的tmp指向*p指向的旧睡眠队列头，而让*p指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up或者中断或者收到信号唤醒了*p指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
{                                                                             //168/ 
	struct task_struct *tmp;                                              //169/ 
                                                                              //170/ 
	if (!p)                                                               //171/ 
		return;                                                       //172/ 
	if (current == &(init_task.task))                                     //173/ 
		panic("task[0] trying to sleep");                             //174/ 
	tmp=*p;                                                               //175/ 
	*p=current;                                                           //176/ 
repeat:	current->state = TASK_INTERRUPTIBLE;                                  //177/ 
	schedule();                                                           //178/ 
	if (*p && *p != current) {                                            //179/ 179-182用于检测当不是用wake_up唤醒等待队列头，而是用中断或信号唤醒了等待队列中间的某个任务时，那么将该任务还置为可中断的睡眠状态，而从等待队列头开始唤醒，将等待队列头置为就绪态
		(**p).state=0;                                                //180/ 
		goto repeat;                                                  //181/ 
	}                                                                     //182/ 
	*p=NULL;                                                              //183/ 
	if (tmp)                                                              //184/ 
		tmp->state=0;                                                 //185/ 
}                                                                             //186/ 
                                                                              //187/ 
void wake_up(struct task_struct **p)                                          //188/ [b;]唤醒*p指向的任务，*p是任务等待队列头指针
{                                                                             //189/ 
	if (p && *p) {                                                        //190/ 
		(**p).state=0;                                                //191/ 
		*p=NULL;                                                      //192/ 
	}                                                                     //193/ 
}                                                                             //194/ 
                                                                              //195/ 
/*                                                                            //196/ 
 * OK, here are some floppy things that shouldn't be in the kernel            //197/ 
 * proper. They are here because the floppy needs a timer, and this           //198/ 
 * was the easiest way of doing it.                                           //199/ 
 */                                                                           //200/ 
static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL};            //201/ 存放各软驱(A-D)马达启动到正常转速的进程指针
static int  mon_timer[4]={0,0,0,0};                                           //202/ 存放各软驱(A-D)马达启动剩余所需要的滴答数。程序中刚开始设为50个滴答
static int moff_timer[4]={0,0,0,0};                                           //203/ 存放各软驱(A-D)马达停转之前剩余需维持的时间。程序中刚开始设为10000个滴答
unsigned char current_DOR = 0x0C;                                             //204/ 当前数字输出寄存器变量，用于将0x0C写入当前软盘控制器中的数字输出寄存器中(位7-4:分别控制软驱D-A马达的启动，1-启动，0-关闭；位3:1-允许DMA和中断请求，0-禁止DMA和中断请求；位2:1-启动软盘控制器，0-复位软盘控制器；位1-0:00-11，用于选择控制的软驱A-D)
                                                                              //205/ 
int ticks_to_floppy_on(unsigned int nr)                                       //206/ [b;]将nr指定的软驱马达停转所需维持的滴答数设为10000滴答，判断软驱是否被选定，如果未被选定，则判断nr指定软驱马达是否开启，若未开启，则开启马达，启动软盘控制器，允许DMA和中断请求，并设置好启动所需的剩余滴答数；若已开启，如果启动所需的剩余滴答数小于2，则重置其启动所需的剩余滴答数为2。返回nr指定软驱马达启动所需的剩余滴答数
{                                                                             //207/ 
	extern unsigned char selected;                                        //208/ 软驱已选定标志
	unsigned char mask = 0x10 << nr;                                      //209/ 
                                                                              //210/ 
	if (nr>3)                                                             //211/ 如果参数nr指定的软驱号不在0-3的范围内
		panic("floppy_on: nr>3");                                     //212/ 则直接死机
	moff_timer[nr]=10000;		/* 100 s = very big :-) */            //213/ 将指定软驱的马达停转之前需维持的时间指定为10000个滴答
	cli();				/* use floppy_off to turn it off */   //214/ 关中断
	mask |= current_DOR;                                                  //215/ 用于开启nr指定的驱动器马达
	if (!selected) {                                                      //216/ 如果软驱未被选定，则自动选定nr指定的软驱
		mask &= 0xFC;                                                 //217/ 
		mask |= nr;                                                   //218/ 
	}                                                                     //219/ 
	if (mask != current_DOR) {                                            //220/ 如果组装好的数字输出寄存器值mask与当前软盘控制器中的数字输出寄存器中的值不同
		outb(mask,FD_DOR);                                            //221/ 则将组装好的值mask(允许DMA和中断请求,启动软盘控制器,将nr指定的软驱马达启动)写进软盘控制器中数字输出寄存器中
		if ((mask ^ current_DOR) & 0xf0)                              //222/ 如果组装的mask中指定启动一个之前没有启动的软驱
			mon_timer[nr] = HZ/2;                                 //223/ 则将该软驱马达启动所需的剩余滴答数设为50滴答
		else if (mon_timer[nr] < 2)                                   //224/ 否则在mask指定的软驱已启动的情况下，如果启动所需的剩余滴答数小于2
			mon_timer[nr] = 2;                                    //225/ 则重置其启动所需的剩余滴答数为2
		current_DOR = mask;                                           //226/ 更新当前数字输出寄存器变量
	}                                                                     //227/ 
	sti();                                                                //228/ 开中断
	return mon_timer[nr];                                                 //229/ 返回参数nr指定的软驱启动所需的剩余滴答数
}                                                                             //230/ 
                                                                              //231/ 
void floppy_on(unsigned int nr)                                               //232/ [b;]使当前任务进入睡眠等待nr指定的软驱马达启动所需的一段时间，启动好后返回
{                                                                             //233/ 
	cli();                                                                //234/ 关中断
	while (ticks_to_floppy_on(nr))                                        //235/ 将nr指定的软驱马达停转所需维持的滴答数设为10000滴答，判断软驱是否被选定，如果未被选定，则判断nr指定软驱马达是否开启，若未开启，则开启马达，启动软盘控制器，允许DMA和中断请求，并设置好启动所需的剩余滴答数；若已开启，如果启动所需的剩余滴答数小于2，则重置其启动所需的剩余滴答数为2。返回nr指定软驱马达启动所需的剩余滴答数
		sleep_on(nr+wait_motor);                                      //236/ 如果返回的启动所需的滴答数不为0，表示还没有启动好，则把当前任务置为不可中断的睡眠状态，让当前任务的tmp指向*(nr+wait_motor)指向的旧睡眠队列头，而让*(nr+wait_motor)指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up唤醒了*(nr+wait_motor)指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
	sti();                                                                //237/ 开中断
}                                                                             //238/ 
                                                                              //239/ 
void floppy_off(unsigned int nr)                                              //240/ [b;]置nr指定的软驱马达停转之前剩余需维持的时间为300滴答
{                                                                             //241/ 
	moff_timer[nr]=3*HZ;                                                  //242/ 置nr指定的软驱马达停转之前剩余需维持的时间为300滴答，即3秒
}                                                                             //243/ 
                                                                              //244/ 
void do_floppy_timer(void)                                                    //245/ [r;]软盘有关
{                                                                             //246/ 
	int i;                                                                //247/ 
	unsigned char mask = 0x10;                                            //248/ 
                                                                              //249/ 
	for (i=0 ; i<4 ; i++,mask <<= 1) {                                    //250/ 
		if (!(mask & current_DOR))                                    //251/ 
			continue;                                             //252/ 
		if (mon_timer[i]) {                                           //253/ 
			if (!--mon_timer[i])                                  //254/ 
				wake_up(i+wait_motor);                        //255/ 
		} else if (!moff_timer[i]) {                                  //256/ 
			current_DOR &= ~mask;                                 //257/ 
			outb(current_DOR,FD_DOR);                             //258/ 
		} else                                                        //259/ 
			moff_timer[i]--;                                      //260/ 
	}                                                                     //261/ 
}                                                                             //262/ 
                                                                              //263/ 
#define TIME_REQUESTS 64                                                      //264/ 系统中最多只可有64个定时器(仅供内核使用)
                                                                              //265/ 
static struct timer_list {                                                    //266/ 
	long jiffies;                                                         //267/ 定时滴答数
	void (*fn)();                                                         //268/ 定时处理程序
	struct timer_list * next;                                             //269/ 指向队列中下一个定时器的指针
} timer_list[TIME_REQUESTS], * next_timer = NULL;                             //270/ 定义定时器队列结构的数组timer_list，next_timer为定时器队列头指针
                                                                              //271/ 
void add_timer(long jiffies, void (*fn)(void))                                //272/ [b;]为内核添加定时器，当参数jiffies指定的滴答数减到0时，执行参数fn指定的函数
{                                                                             //273/ 
	struct timer_list * p;                                                //274/ 
                                                                              //275/ 
	if (!fn)                                                              //276/ 如果定时处理程序指针为空，直接退出
		return;                                                       //277/ 
	cli();                                                                //278/ 关中断
	if (jiffies <= 0)                                                     //279/ 如果参数jiffies滴答值小于等于0，立刻调用其处理程序，
		(fn)();                                                       //280/ 并且不将定时器加入链表
	else {                                                                //281/ 
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)   //282/ 否则从64个定时器槽找出一个空闲槽
			if (!p->fn)                                           //283/ 
				break;                                        //284/ 
		if (p >= timer_list + TIME_REQUESTS)                          //285/ 如果64个定时器槽都用完了，直接死机
			panic("No more time requests free");                  //286/ 
		p->fn = fn;                                                   //287/ 如果找到了空槽，则在此槽中新建一个定时器，并加入队列中
		p->jiffies = jiffies;                                         //288/ 
		p->next = next_timer;                                         //289/ 
		next_timer = p;                                               //290/ 保证next_timer指向定时器队列头
		while (p->next && p->next->jiffies < p->jiffies) {            //291/ 如果定时器队列中不只一个定时器，并且新加入的定时器(timer1)，其jiffies大于之前加入的一个定时器(timer2)的jiffies，则调整timer1的jiffies大小及二者位置，再将调整后的time1与timer3比较，以此类推(bug：一个没有考虑周全的问题——如果定时器队列中不只一个定时器，并且新加入的定时器(timer1)，其jiffies小于或等于之前加入的一个定时器(timer2)的jiffies，则应该调整timer2的jiffies减去timer1的jiffies值)
			p->jiffies -= p->next->jiffies;                       //292/ 
			fn = p->fn;                                           //293/ 
			p->fn = p->next->fn;                                  //294/ 
			p->next->fn = fn;                                     //295/ 
			jiffies = p->jiffies;                                 //296/ 
			p->jiffies = p->next->jiffies;                        //297/ 
			p->next->jiffies = jiffies;                           //298/ 
			p = p->next;                                          //299/ 
		}                                                             //300/ 
	}                                                                     //301/ 
	sti();                                                                //302/ 开中断
}                                                                             //303/ 
                                                                              //304/ 
void do_timer(long cpl)                                                       //305/ [b;]时钟中断C函数处理程序，在system_call.s中的_timer_interrupt中被调用
{                                                                             //306/ 
	extern int beepcount;                                                 //307/ 扬声器发声时间(滴答数)
	extern void sysbeepstop(void);                                        //308/ 关闭扬声器
                                                                              //309/ 
	if (beepcount)                                                        //310/ 扬声器发声时间到了之后关闭扬声器
		if (!--beepcount)                                             //311/ 
			sysbeepstop();                                        //312/ 
                                                                              //313/ 
	if (cpl)                                                              //314/ 如果cpl大于0，则增加内核代码运行时间utime
		current->utime++;                                             //315/ 
	else                                                                  //316/ 如果cpl等于0，则增加用户代码运行时间stime
		current->stime++;                                             //317/ 
                                                                              //318/ 
	if (next_timer) {                                                     //319/ 如果有定时器存在
		next_timer->jiffies--;                                        //320/ 将定时器队列头的定时滴答数jiffies减一
		while (next_timer && next_timer->jiffies <= 0) {              //321/ 如果定时器队列头的定时时间到了，则调用其定时处理程序，
			void (*fn)(void);                                     //322/ 并将该定时器从队列中抹掉
			                                                      //323/ 
			fn = next_timer->fn;                                  //324/ 
			next_timer->fn = NULL;                                //325/ 
			next_timer = next_timer->next;                        //326/ 
			(fn)();                                               //327/ 
		}                                                             //328/ 
	}                                                                     //329/ 
	if (current_DOR & 0xf0)                                               //330/ [r;]暂未
		do_floppy_timer();                                            //331/ [r;]暂未
	if ((--current->counter)>0) return;                                   //332/ 将当前任务的可运行时间counter减去一个滴答，如果时间还没用完，直接退出
	current->counter=0;                                                   //333/ 否则将当前任务的counter置0
	if (!cpl) return;                                                     //334/ 如果任务处于内核态，由于不允许抢占，所以直接退出
	schedule();                                                           //335/ 重新调度
}                                                                             //336/ 
                                                                              //337/ 
int sys_alarm(long seconds)                                                   //338/ [b;]系统调用-设置报警定时时间值(秒)，当seconds大于0时返回原定时时刻还剩余的时间(秒)
{                                                                             //339/ 
	int old = current->alarm;                                             //340/ 
                                                                              //341/ 
	if (old)                                                              //342/ 
		old = (old - jiffies) / HZ;                                   //343/ 
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;                  //344/ 
	return (old);                                                         //345/ 
}                                                                             //346/ 
                                                                              //347/ 
int sys_getpid(void)                                                          //348/ [b;]系统调用-取当前进程号pid
{                                                                             //349/ 
	return current->pid;                                                  //350/ 
}                                                                             //351/ 
                                                                              //352/ 
int sys_getppid(void)                                                         //353/ [b;]系统调用-取当前进程的父进程号ppid
{                                                                             //354/ 
	return current->father;                                               //355/ 
}                                                                             //356/ 
                                                                              //357/ 
int sys_getuid(void)                                                          //358/ [b;]系统调用-取当前进程用户号uid
{                                                                             //359/ 
	return current->uid;                                                  //360/ 
}                                                                             //361/ 
                                                                              //362/ 
int sys_geteuid(void)                                                         //363/ [b;]系统调用-取当前进程有效的用户号euid
{                                                                             //364/ 
	return current->euid;                                                 //365/ 
}                                                                             //366/ 
                                                                              //367/ 
int sys_getgid(void)                                                          //368/ [b;]系统调用-取当前进程的组号gid
{                                                                             //369/ 
	return current->gid;                                                  //370/ 
}                                                                             //371/ 
                                                                              //372/ 
int sys_getegid(void)                                                         //373/ [b;]系统调用-取当前进程有效的组号egid
{                                                                             //374/ 
	return current->egid;                                                 //375/ 
}                                                                             //376/ 
                                                                              //377/ 
int sys_nice(long increment)                                                  //378/ [b;]系统调用-降低当前进程的优先权值priority，会影响counter的赋值
{                                                                             //379/ 
	if (current->priority-increment>0)                                    //380/ 
		current->priority -= increment;                               //381/ 
	return 0;                                                             //382/ 
}                                                                             //383/ 
                                                                              //384/ 
void sched_init(void)                                                         //385/ [b;]内核调度程序的初始化子程序
{                                                                             //386/ 
	int i;                                                                //387/ 
	struct desc_struct * p;                                               //388/ 定义一个描述符表结构指针p
                                                                              //389/ 
	if (sizeof(struct sigaction) != 16)                                   //390/ 无效
		panic("Struct sigaction MUST be 16 bytes");                   //391/ 无效
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));              //392/ 将参数2给定的任务0的TSS段基地址addr和固定的值(段界限=104、G=0、P=1、DPL=0、B(忙位)=0)写入参数1指定的地址处的8个字节
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));              //393/ 将参数2给定的任务0的LDT段基地址addr和固定的值(段界限=104、G=0、P=1、DPL=0)写入参数1指定的地址处的8个字节
	p = gdt+2+FIRST_TSS_ENTRY;                                            //394/ 394-401行用于将除任务0之外的任务数组task[i]以及各个任务的TSS和LDT描述符项都清空
	for(i=1;i<NR_TASKS;i++) {                                             //395/ 
		task[i] = NULL;                                               //396/ 
		p->a=p->b=0;                                                  //397/ 
		p++;                                                          //398/ 
		p->a=p->b=0;                                                  //399/ 
		p++;                                                          //400/ 
	}                                                                     //401/ 
/* Clear NT, so that we won't have troubles with that later on */             //402/ 
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");                  //403/ 复位中断嵌套标志NT
	ltr(0);                                                               //404/ 把任务0的TSS段选择符加载到任务寄存器TR中
	lldt(0);                                                              //405/ 把任务0的LDT段选择符加载到局部描述赋表寄存器LDTR中
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */   //406/ 设置8253定时器的控制字:通道0 模式3 先传低字节后传高字节 二进制
	outb_p(LATCH & 0xff , 0x40);	/* LSB */                             //407/ 设置8253定时器通道0的初始计数值的低字节
	outb(LATCH >> 8 , 0x40);	/* MSB */                             //408/ 设置8253定时器通道0的初始计数值的高字节
	set_intr_gate(0x20,&timer_interrupt);                                 //409/ 在idt表中的第32(0x20)(从0算起)个描述符的位置放置一个中断门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为&timer_interrupt;P=1;DPL=0;TYPE=14->1110)
	outb(inb_p(0x21)&~0x01,0x21);                                         //410/ 开启8259A主芯片的第一个中断IR0
	set_system_gate(0x80,&system_call);                                   //411/ 在idt表中的第128(0x80)(从0算起)个描述符的位置放置一个陷阱门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为&system_call;P=1;DPL=3;TYPE=15->1111)
}                                                                             //412/ 

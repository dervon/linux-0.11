/*                                                                         //  1/ 
 *  linux/init/main.c                                                      //  2/ 
 *                                                                         //  3/ 
 *  (C) 1991  Linus Torvalds                                               //  4/ 
 */                                                                        //  5/ 
                                                                           //  6/ 
#define __LIBRARY__                                                        //  7/ 定义该变量为了将unistd.h中的系统调用号和内嵌汇编_system0()等包含进来
#include <unistd.h>                                                        //  8/ 
#include <time.h>                                                          //  9/ 
                                                                           // 10/ 
/*                                                                         // 11/ 
 * we need this inline - forking from kernel space will result             // 12/ 
 * in NO COPY ON WRITE (!!!), until an execve is executed. This            // 13/ 
 * is no problem, but for the stack. This is handled by not letting        // 14/ 
 * main() use the stack at all after fork(). Thus, no function             // 15/ 
 * calls - which means inline code for fork too, as otherwise we           // 16/ 
 * would use the stack upon exit from 'fork()'.                            // 17/ 
 *                                                                         // 18/ 
 * Actually only pause and fork are needed inline, so that there           // 19/ 
 * won't be any messing with the stack from main(), but we define          // 20/ 
 * some others too.                                                        // 21/ 
 */                                                                        // 22/ 
static inline _syscall0(int,fork)                                          // 23/ [b;]static inline通常是用于内联集成，不会为函数自身产生汇编代码。除非
static inline _syscall0(int,pause)                                         // 24/ [r;]编译过程使用-fkeep-inline-functions选项。或者遇到特殊情况——内联
static inline _syscall1(int,setup,void *,BIOS)                             // 25/ 函数定义之前的调用语句或者有引用内联函数地址的语句
static inline _syscall0(int,sync)                                          // 26/ 
                                                                           // 27/ 
#include <linux/tty.h>                                                     // 28/ 
#include <linux/sched.h>                                                   // 29/ 
#include <linux/head.h>                                                    // 30/ 
#include <asm/system.h>                                                    // 31/ 
#include <asm/io.h>                                                        // 32/ 
                                                                           // 33/ 
#include <stddef.h>                                                        // 34/ 
#include <stdarg.h>                                                        // 35/ 
#include <unistd.h>                                                        // 36/ 
#include <fcntl.h>                                                         // 37/ 
#include <sys/types.h>                                                     // 38/ 
                                                                           // 39/ 
#include <linux/fs.h>                                                      // 40/ 
                                                                           // 41/ 
static char printbuf[1024];                                                // 42/ 静态字符串数组，用作内核显示信息的缓存
                                                                           // 43/ 
extern int vsprintf();                                                     // 44/ 
extern void init(void);                                                    // 45/ 
extern void blk_dev_init(void);                                            // 46/ 
extern void chr_dev_init(void);                                            // 47/ 
extern void hd_init(void);                                                 // 48/ 
extern void floppy_init(void);                                             // 49/ 
extern void mem_init(long start, long end);                                // 50/ 
extern long rd_init(long mem_start, int length);                           // 51/ 
extern long kernel_mktime(struct tm * tm);                                 // 52/ 
extern long startup_time;                                                  // 53/ 
                                                                           // 54/ 
/*                                                                         // 55/ 
 * This is set up by the setup-routine at boot-time                        // 56/ 
 */                                                                        // 57/ 
#define EXT_MEM_K (*(unsigned short *)0x90002)                             // 58/ 0x90002处存放的是系统从1MB开始的扩展内存数值(KB)
#define DRIVE_INFO (*(struct drive_info *)0x90080)                         // 59/ 0x90080处存放的是第一个硬盘的参数表
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)                         // 60/ 0x901FC处存放的是根文件系统所在的设备号
                                                                           // 61/ 
/*                                                                         // 62/ 
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly       // 63/ 
 * and this seems to work. I anybody has more info on the real-time        // 64/ 
 * clock I'd be interested. Most of this was trial and error, and some     // 65/ 
 * bios-listing reading. Urghh.                                            // 66/ 
 */                                                                        // 67/ 
                                                                           // 68/ 
#define CMOS_READ(addr) ({ \                                               // 69/ 0x70是CMOS RAM的索引端口，其最高位是控制NMI中断的开关，为1表示阻断所有NMI信号
outb_p(0x80|addr,0x70); \                                                  // 70/ 将0x80|addr通过al寄存器写入0x70端口中并等待一会
inb_p(0x71); \                                                             // 71/ 从0x71端口读出值到al寄存器中，等待一会然后将值返回(作为宏CMOS_READ的值返回)
})                                                                         // 72/ 0x71是CMOS RAM的数据端口
                                                                           // 73/ 
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)                 // 74/ 将一个字节的BCD码转换成一个字节的二进制数值
                                                                           // 75/ 
static void time_init(void)                                                // 76/ [b;]算出从1970.1.1:0:0:0开始到此时(系统刚开机时)经过的秒数赋给startup_time 
{                                                                          // 77/ 
	struct tm time;                                                    // 78/ 
                                                                           // 79/ 
	do {                                                               // 80/ 
		time.tm_sec = CMOS_READ(0);                                // 81/ 取出CMOS RAM中从1900.1.1:0:0:0开始到此时经过的秒数
		time.tm_min = CMOS_READ(2);                                // 82/ 取出CMOS RAM中从1900.1.1:0:0:0开始到此时经过的分钟
		time.tm_hour = CMOS_READ(4);                               // 83/ 取出CMOS RAM中从1900.1.1:0:0:0开始到此时经过的小时
		time.tm_mday = CMOS_READ(7);                               // 84/ 取出CMOS RAM中从1900.1.1:0:0:0开始到此时经过的日数
		time.tm_mon = CMOS_READ(8);                                // 85/ 取出CMOS RAM中从1900.1.1:0:0:0开始到此时经过的月数
		time.tm_year = CMOS_READ(9);                               // 86/ 取出CMOS RAM中从1900.1.1:0:0:0开始到此时经过的年数
	} while (time.tm_sec != CMOS_READ(0));                             // 87/ 
	BCD_TO_BIN(time.tm_sec);                                           // 88/ 88-93行将取出的BCD码都转换成二进制数值
	BCD_TO_BIN(time.tm_min);                                           // 89/ 
	BCD_TO_BIN(time.tm_hour);                                          // 90/ 
	BCD_TO_BIN(time.tm_mday);                                          // 91/ 
	BCD_TO_BIN(time.tm_mon);                                           // 92/ 
	BCD_TO_BIN(time.tm_year);                                          // 93/ 
	time.tm_mon--;                                                     // 94/ 因为tm结构中的tm_mon是用0-11来表表示月份的
	startup_time = kernel_mktime(&time);                               // 95/ 算出从1970.1.1:0:0:0开始到此时(系统刚开机时)经过的秒数赋给startup_time
}                                                                          // 96/ 
                                                                           // 97/ 
static long memory_end = 0;                                                // 98/ 物理内存大小(字节数)
static long buffer_memory_end = 0;                                         // 99/ 存放高速缓冲区都末端地址
static long main_memory_start = 0;                                         //100/ 存放主内存(将用于分页)开始的位置
                                                                           //101/ 
struct drive_info { char dummy[32]; } drive_info;                          //102/ 用于存放硬盘参数表信息，每个参数表16B，可以放两个硬盘的信息
                                                                           //103/ 
void main(void)		/* This really IS void, no error here. */          //104/ [b;]内核初始化主程序。初始化结束后将以任务0(idle任务即空闲任务)的身份运行
{			/* The startup routine assumes (well, ...) this */ //105/ 
/*                                                                         //106/ 
 * Interrupts are still disabled. Do necessary setups, then                //107/ 
 * enable them                                                             //108/ 
 */                                                                        //109/ 
 	ROOT_DEV = ORIG_ROOT_DEV;                                          //110/ 将根设备号赋给ROOT_DEV(定义在fs/super.c中)
 	drive_info = DRIVE_INFO;                                           //111/ 将两个硬盘的参数表信息(32B)赋给drive_info
	memory_end = (1<<20) + (EXT_MEM_K<<10);                            //112/ 物理内存大小 = 1MB + 扩展内存(1MB之后的内存值，单位KB) * 1KB
	memory_end &= 0xfffff000;                                          //113/ 将最后不到4KB(1页)的内存值忽略
	if (memory_end > 16*1024*1024)                                     //114/ 如果内存超过16MB，则只用16MB
		memory_end = 16*1024*1024;                                 //115/ 
	if (memory_end > 12*1024*1024)                                     //116/ 如果内存超过12MB，则设置高速缓冲区末端 = 4MB
		buffer_memory_end = 4*1024*1024;                           //117/ 
	else if (memory_end > 6*1024*1024)                                 //118/ 如果内存超过6MB，则设置高速缓冲区末端 = 2MB
		buffer_memory_end = 2*1024*1024;                           //119/ 
	else                                                               //120/ 如果内存不超过6MB，则设置高速缓冲区末端 = 1MB
		buffer_memory_end = 1*1024*1024;                           //121/ 
	main_memory_start = buffer_memory_end;                             //122/ 主内存(将用于分页)开始的位置 = 高速缓冲区末端
#ifdef RAMDISK                                                             //123/ 
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024);     //124/ 如果Makefile文件中定义了内存虚拟盘符号RAMDISK，则初始化虚拟盘，并从
#endif                                                                     //125/ 主内存中划分一部分空间给虚拟盘，将main_memory_start增大
	mem_init(main_memory_start,memory_end);                            //126/ 
	trap_init();                                                       //127/ 在中断描述符表IDT[0-47]中建立一些陷阱门描述符
	blk_dev_init();                                                    //128/ 初始化请求数组，将所有(共NR_REQUEST = 32个)请求项置为空闲项
	chr_dev_init();                                                    //129/ 字符设备初始化函数，空着，为以后扩展使用
	tty_init();                                                        //130/ tty终端初始化——初始化串口终端和控制台终端
	time_init();                                                       //131/ 算出从1970.1.1:0:0:0开始到此时(系统刚开机时)经过的秒数赋给startup_time 
	sched_init();                                                      //132/ 内核调度程序的初始化子程序
	buffer_init(buffer_memory_end);                                    //133/ 从高速缓冲区开始位置start_buffer到末端buffer_memory_end处分别同时设置(初始化)缓冲头结构和对应的缓冲块，直到缓冲区中所有内存被分配完毕
	hd_init();                                                         //134/ 硬盘系统初始化
	floppy_init();                                                     //135/ 软盘系统初始化
	sti();                                                             //136/ 开中断，将处理器标志寄存器的中断标志位置1
	move_to_user_mode();                                               //137/ 
	if (!fork()) {		/* we count on this going ok */            //138/ 138-148行建立子进程，主进程执行pause()进入睡眠状态，子进程执行init()
		init();                                                    //139/ 
	}                                                                  //140/ 
/*                                                                         //141/ 
 *   NOTE!!   For any other task 'pause()' would mean we have to get a     //142/ 
 * signal to awaken, but task0 is the sole exception (see 'schedule()')    //143/ 
 * as task 0 gets activated at every idle moment (when no other tasks      //144/ 
 * can run). For task0 'pause()' just means we go check if some other      //145/ 
 * task can run, and if not we return here.                                //146/ 
 */                                                                        //147/ 
	for(;;) pause();                                                   //148/ 
}                                                                          //149/ 
                                                                           //150/ 
static int printf(const char *fmt, ...)                                    //151/ [b;]
{                                                                          //152/ 
	va_list args;                                                      //153/ 
	int i;                                                             //154/ 
                                                                           //155/ 
	va_start(args, fmt);                                               //156/ 
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));                 //157/ 
	va_end(args);                                                      //158/ 
	return i;                                                          //159/ 
}                                                                          //160/ 
                                                                           //161/ 
static char * argv_rc[] = { "/bin/sh", NULL };                             //162/ 
static char * envp_rc[] = { "HOME=/", NULL };                              //163/ 
                                                                           //164/ 
static char * argv[] = { "-/bin/sh",NULL };                                //165/ 
static char * envp[] = { "HOME=/usr/root", NULL };                         //166/ 
                                                                           //167/ 
void init(void)                                                            //168/ [b;]
{                                                                          //169/ 
	int pid,i;                                                         //170/ 
                                                                           //171/ 
	setup((void *) &drive_info);                                       //172/ 
	(void) open("/dev/tty0",O_RDWR,0);                                 //173/ 
	(void) dup(0);                                                     //174/ 
	(void) dup(0);                                                     //175/ 
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,        //176/ 
		NR_BUFFERS*BLOCK_SIZE);                                    //177/ 
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);     //178/ 
	if (!(pid=fork())) {                                               //179/ 
		close(0);                                                  //180/ 
		if (open("/etc/rc",O_RDONLY,0))                            //181/ 
			_exit(1);                                          //182/ 
		execve("/bin/sh",argv_rc,envp_rc);                         //183/ 
		_exit(2);                                                  //184/ 
	}                                                                  //185/ 
	if (pid>0)                                                         //186/ 
		while (pid != wait(&i))                                    //187/ 
			/* nothing */;                                     //188/ 
	while (1) {                                                        //189/ 
		if ((pid=fork())<0) {                                      //190/ 
			printf("Fork failed in init\r\n");                 //191/ 
			continue;                                          //192/ 
		}                                                          //193/ 
		if (!pid) {                                                //194/ 
			close(0);close(1);close(2);                        //195/ 
			setsid();                                          //196/ 
			(void) open("/dev/tty0",O_RDWR,0);                 //197/ 
			(void) dup(0);                                     //198/ 
			(void) dup(0);                                     //199/ 
			_exit(execve("/bin/sh",argv,envp));                //200/ 
		}                                                          //201/ 
		while (1)                                                  //202/ 
			if (pid == wait(&i))                               //203/ 
				break;                                     //204/ 
		printf("\n\rchild %d died with code %04x\n\r",pid,i);      //205/ 
		sync();                                                    //206/ 
	}                                                                  //207/ 
	_exit(0);	/* NOTE! _exit, not exit() */                      //208/ 
}                                                                          //209/ 

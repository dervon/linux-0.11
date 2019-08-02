/*                                                                               //  1/ 
 *  linux/kernel/floppy.c                                                        //  2/ 
 *                                                                               //  3/ 
 *  (C) 1991  Linus Torvalds                                                     //  4/ 
 */                                                                              //  5/ 
                                                                                 //  6/ 
/*                                                                               //  7/ 
 * 02.12.91 - Changed to static variables to indicate need for reset             //  8/ 
 * and recalibrate. This makes some things easier (output_byte reset             //  9/ 
 * checking etc), and means less interrupt jumping in case of errors,            // 10/ 
 * so the code is hopefully easier to understand.                                // 11/ 
 */                                                                              // 12/ 
                                                                                 // 13/ 
/*                                                                               // 14/ 
 * This file is certainly a mess. I've tried my best to get it working,          // 15/ 
 * but I don't like programming floppies, and I have only one anyway.            // 16/ 
 * Urgel. I should check for more errors, and do more graceful error             // 17/ 
 * recovery. Seems there are problems with several drives. I've tried to         // 18/ 
 * correct them. No promises.                                                    // 19/ 
 */                                                                              // 20/ 
                                                                                 // 21/ 
/*                                                                               // 22/ 
 * As with hd.c, all routines within this file can (and will) be called          // 23/ 
 * by interrupts, so extreme caution is needed. A hardware interrupt             // 24/ 
 * handler may not sleep, or a kernel panic will happen. Thus I cannot           // 25/ 
 * call "floppy-on" directly, but have to set a special timer interrupt          // 26/ 
 * etc.                                                                          // 27/ 
 *                                                                               // 28/ 
 * Also, I'm not certain this works on more than 1 floppy. Bugs may              // 29/ 
 * abund.                                                                        // 30/ 
 */                                                                              // 31/ 
                                                                                 // 32/ 
#include <linux/sched.h>                                                         // 33/ 
#include <linux/fs.h>                                                            // 34/ 
#include <linux/kernel.h>                                                        // 35/ 
#include <linux/fdreg.h>                                                         // 36/ 
#include <asm/system.h>                                                          // 37/ 
#include <asm/io.h>                                                              // 38/ 
#include <asm/segment.h>                                                         // 39/ 
                                                                                 // 40/ 
#define MAJOR_NR 2                                                               // 41/ 定义主设备号为软盘的设备号2，用于给blk.h作出判断，确定一些符号和宏
#include "blk.h"                                                                 // 42/ 
                                                                                 // 43/ 
static int recalibrate = 0;                                                      // 44/ 重新校正标志：1表示需要重新校正磁头位置(磁头归零道)
static int reset = 0;                                                            // 45/ 复位标志：1表示需要进行复位操作
static int seek = 0;                                                             // 46/ 寻道标志：1表示需要执行寻道操作
                                                                                 // 47/ 
extern unsigned char current_DOR;                                                // 48/ 用于写入当前软盘控制器中的数字输出寄存器中的值
                                                                                 // 49/ 
#define immoutb_p(val,port) \                                                    // 50/ 把值val输出到端口port中
__asm__("outb %0,%1\n\tjmp 1f\n1:\tjmp 1f\n1:"::"a" ((char) (val)),"i" (port))   // 51/ 
                                                                                 // 52/ 
#define TYPE(x) ((x)>>2)                                                         // 53/ 利用此设备号x = TYPE * 4 + DRIVE计算出软驱类型(2--1.2MB，7--1.44MB)
#define DRIVE(x) ((x)&0x03)                                                      // 54/ 利用此设备号x = TYPE * 4 + DRIVE计算出软驱序号(0--3对应A--D)
/*                                                                               // 55/ 
 * Note that MAX_ERRORS=8 doesn't imply that we retry every bad read             // 56/ 
 * max 8 times - some types of errors increase the errorcount by 2,              // 57/ 
 * so we might actually retry only 5-6 times before giving up.                   // 58/ 
 */                                                                              // 59/ 
#define MAX_ERRORS 8                                                             // 60/ 并不一定表示每次读错误尝试最多8次
                                                                                 // 61/ 
/*                                                                               // 62/ 
 * globals used by 'result()'                                                    // 63/ 
 */                                                                              // 64/ 
#define MAX_REPLIES 7                                                            // 65/ 软盘控制器最多返回7字节的结果信息
static unsigned char reply_buffer[MAX_REPLIES];                                  // 66/ 存放软盘控制器返回的应答结果信息
#define ST0 (reply_buffer[0])                                                    // 67/ 结果状态字节0
#define ST1 (reply_buffer[1])                                                    // 68/ 结果状态字节1
#define ST2 (reply_buffer[2])                                                    // 69/ 结果状态字节2
#define ST3 (reply_buffer[3])                                                    // 70/ 结果状态字节3
                                                                                 // 71/ 
/*                                                                               // 72/ 
 * This struct defines the different floppy types. Unlike minix                  // 73/ 
 * linux doesn't have a "search for right type"-type, as the code                // 74/ 
 * for that is convoluted and weird. I've got enough problems with               // 75/ 
 * this driver as it is.                                                         // 76/ 
 *                                                                               // 77/ 
 * The 'stretch' tells if the tracks need to be boubled for some                 // 78/ 
 * types (ie 360kB diskette in 1.2MB drive etc). Others should                   // 79/ 
 * be self-explanatory.                                                          // 80/ 
 */                                                                              // 81/ 
static struct floppy_struct {                                                    // 82/ 82-93行定义了软盘类型参数结构数组
	unsigned int size, sect, head, track, stretch;                           // 83/ 扇区数、每磁道扇区数、磁头数、磁道数、对磁盘是否要特殊处理(标志)
	unsigned char gap,rate,spec1;                                            // 84/ 扇区间隙长度(字节数)、数据传输速率、参数(高4位是步进速率，低4位是磁头卸载时间)
} floppy_type[] = {                                                              // 85/ 软盘的类型TYPE被用作该数组的索引，详见431行
	{    0, 0,0, 0,0,0x00,0x00,0x00 },	/* no testing */                 // 86/ 
	{  720, 9,2,40,0,0x2A,0x02,0xDF },	/* 360kB PC diskettes */         // 87/ 
	{ 2400,15,2,80,0,0x1B,0x00,0xDF },	/* 1.2 MB AT-diskettes */        // 88/ 
	{  720, 9,2,40,1,0x2A,0x02,0xDF },	/* 360kB in 720kB drive */       // 89/ 
	{ 1440, 9,2,80,0,0x2A,0x02,0xDF },	/* 3.5" 720kB diskette */        // 90/ 
	{  720, 9,2,40,1,0x23,0x01,0xDF },	/* 360kB in 1.2MB drive */       // 91/ 
	{ 1440, 9,2,80,0,0x23,0x01,0xDF },	/* 720kB in 1.2MB drive */       // 92/ 
	{ 2880,18,2,80,0,0x1B,0x00,0xCF },	/* 1.44MB diskette */            // 93/ 
};                                                                               // 94/ 
/*                                                                               // 95/ 
 * Rate is 0 for 500kb/s, 2 for 300kbps, 1 for 250kbps                           // 96/ 
 * Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc),            // 97/ 
 * H is head unload time (1=16ms, 2=32ms, etc)                                   // 98/ 
 *                                                                               // 99/ 
 * Spec2 is (HLD<<1 | ND), where HLD is head load time (1=2ms, 2=4 ms etc)       //100/ 
 * and ND is set means no DMA. Hardcoded to 6 (HLD=6ms, use DMA).                //101/ 
 */                                                                              //102/ 
                                                                                 //103/ 
extern void floppy_interrupt(void);                                              //104/ 软驱中断处理过程标号，存在于system_call.s中
extern char tmp_floppy_area[1024];                                               //105/ 处于物理内存中地址_tmp_floppy_area:0x0000 5000处的1KB大小的临时软盘缓冲区
                                                                                 //106/ 
/*                                                                               //107/ 
 * These are global variables, as that's the easiest way to give                 //108/ 
 * information to interrupts. They are the data used for the current             //109/ 
 * request.                                                                      //110/ 
 */                                                                              //111/ 112-123行用于定义一些全局变量
static int cur_spec1 = -1;                                                       //112/ 当前软盘参数(高4位是步进速率，低4位是磁头卸载时间)
static int cur_rate = -1;                                                        //113/ 当前软盘转速(即数据传输速率)rate
static struct floppy_struct * floppy = floppy_type;                              //114/ 软盘类型结构数组指针
static unsigned char current_drive = 0;                                          //115/ 当前驱动器号
static unsigned char sector = 0;                                                 //116/ 当前扇区号
static unsigned char head = 0;                                                   //117/ 当前磁头号
static unsigned char track = 0;                                                  //118/ 当前磁道号
static unsigned char seek_track = 0;                                             //119/ 寻道磁道号，即寻道号
static unsigned char current_track = 255;                                        //120/ 当前磁头所在磁道号
static unsigned char command = 0;                                                //121/ 软盘控制器命令
unsigned char selected = 0;                                                      //122/ 软驱已选定标志
struct task_struct * wait_on_floppy_select = NULL;                               //123/ 等待选定软驱的任务队列
                                                                                 //124/ 
void floppy_deselect(unsigned int nr)                                            //125/ [b;]取消选定参数nr(00-11)指定的软驱，复位软驱已选定标志，唤醒wait_on_floppy_select指向的任务
{                                                                                //126/ 
	if (nr != (current_DOR & 3))                                             //127/ 如果参数nr指定的软驱未被选定
		printk("floppy_deselect: drive not selected\n\r");               //128/ 则打印信息——“软驱未选中”
	selected = 0;                                                            //129/ 复位软驱已选定标志
	wake_up(&wait_on_floppy_select);                                         //130/ 唤醒wait_on_floppy_select指向的任务，wait_on_floppy_select是任务等待队列头指针
}                                                                                //131/ 
                                                                                 //132/ 
/*                                                                               //133/ 
 * floppy-change is never called from an interrupt, so we can relax a bit        //134/ 
 * here, sleep etc. Note that floppy-on tries to set current_DOR to point        //135/ 
 * to the desired drive, but it will probably not survive the sleep if           //136/ 
 * several floppies are used at the same time: thus the loop.                    //137/ 
 */                                                                              //138/ 
int floppy_change(unsigned int nr)                                               //139/ [b;]检测nr指定软驱中软盘的更改情况，返回1表示已更换，返回0表示未更换
{                                                                                //140/ 
repeat:                                                                          //141/ 
	floppy_on(nr);                                                           //142/ 使当前任务进入睡眠等待nr指定的软驱马达启动所需的一段时间，启动好后返回
	while ((current_DOR & 3) != nr && selected)                              //143/ 如果当前选择的软驱不是参数nr指定的软驱，并且已经选定了其他软驱
		interruptible_sleep_on(&wait_on_floppy_select);                  //144/ 则把当前任务置为可中断的睡眠状态，等待其他软驱被取消选定
	if ((current_DOR & 3) != nr)                                             //145/ 如果当前没有软驱被选中或者其他软驱被取消选定而使当前任务被唤醒，当前软驱仍然不是nr指定的软驱
		goto repeat;                                                     //146/ 则跳回去重新循环等待
	if (inb(FD_DIR) & 0x80) {                                                //147/ 读出软盘控制器中数字输入寄存器的内容，判断位7，如果是1，即盘片已更换
		floppy_off(nr);                                                  //148/ 则置nr指定的软驱马达停转之前剩余需维持的时间为300滴答，并返回1
		return 1;                                                        //149/ 
	}                                                                        //150/ 
	floppy_off(nr);                                                          //151/ 如果是0，即盘片未更换，则置nr指定的软驱马达停转之前剩余需维持的时间为300滴答，并返回0
	return 0;                                                                //152/ 
}                                                                                //153/ 
                                                                                 //154/ 
#define copy_buffer(from,to) \                                                   //155/ [b;]从内存地址from处复制1KB数据到地址to处
__asm__("cld ; rep ; movsl" \                                                    //156/ 
	::"c" (BLOCK_SIZE/4),"S" ((long)(from)),"D" ((long)(to)) \               //157/ 
	:"cx","di","si")                                                         //158/ 
                                                                                 //159/ 
static void setup_DMA(void)                                                      //160/ [b;]对软盘控制器使用的DMA通道2进行设置
{                                                                                //161/ 
	long addr = (long) CURRENT->buffer;                                      //162/ 当前请求项缓冲块所处内存地址
                                                                                 //163/ 
	cli();                                                                   //164/ 关中断
	if (addr >= 0x100000) {                                                  //165/ 如果当前请求项缓冲块处于1MB以上的位置
		addr = (long) tmp_floppy_area;                                   //166/ 则将DMA缓冲区设在临时软盘缓冲区，因为DMA芯片8237只能在1MB地址范围内寻址
		if (command == FD_WRITE)                                         //167/ 如果软盘控制器命令是软盘写扇区数据命令
			copy_buffer(CURRENT->buffer,tmp_floppy_area);            //168/ 则从内存地址CURRENT->buffer处复制1KB数据到临时软盘缓冲区tmp_floppy_area处
	}                                                                        //169/ 
/* mask DMA 2 */                                                                 //170/ 
	immoutb_p(4|2,10);                                                       //171/ 屏蔽DMA通道2——把值4|2输出到DMA单屏蔽寄存器端口10中
/* output command byte. I don't know why, but everyone (minix, */                //172/ 
/* sanches & canton) output this twice, first to 12 then to 11 */                //173/ 
 	__asm__("outb %%al,$12\n\tjmp 1f\n1:\tjmp 1f\n1:\t"                      //174/ 174-176行，如果软盘控制器命令command是读扇区数据命令，则将0x46(请求模式、地址递增、自动预置、DMA读传输、选择DMA通道2)写入DMA清除先后触发器端口12中，然后延时等待一会，之后将0x46写入DMA方式寄存器端口11(可能只为达到写操作的效果，置字节先后触发器为默认状态)中，最后再等待一会；如果软盘控制器命令command不是读扇区数据命令，则将0x4A(请求模式、地址递增、自动预置、DMA写传输、选择DMA通道2)写入前面指定的两个寄存器中
	"outb %%al,$11\n\tjmp 1f\n1:\tjmp 1f\n1:"::                              //175/ 
	"a" ((char) ((command == FD_READ)?DMA_READ:DMA_WRITE)));                 //176/ 
/* 8 low bits of addr */                                                         //177/ 177-184行用于将DMA的地址寄存器和页面寄存器指定为addr地址值
	immoutb_p(addr,4);                                                       //178/ 设置DMA使用的内存页面(一个页面64KB大小)中的偏移地址的低8位——把值addr输出到DMA地址寄存器端口4中
	addr >>= 8;                                                              //179/ 
/* bits 8-15 of addr */                                                          //180/ 
	immoutb_p(addr,4);                                                       //181/ 设置DMA使用的内存页面(一个页面64KB大小)中的偏移地址的高8位——把值addr输出到DMA地址寄存器端口4中
	addr >>= 8;                                                              //182/ 
/* bits 16-19 of addr */                                                         //183/ 
	immoutb_p(addr,0x81);                                                    //184/ 设置DMA使用的内存页面(一个页面64KB大小)的基地址——把值addr输出到DMA页面寄存器端口0x81中
/* low 8 bits of count-1 (1024-1=0x3ff) */                                       //185/ 185-188行设置计数值0x3ff，即DMA将传输1024字节
	immoutb_p(0xff,5);                                                       //186/ 设置DMA使用的计数值的低8位——把值0xff输出到DMA计数寄存器端口5中
/* high 8 bits of count-1 */                                                     //187/ 
	immoutb_p(3,5);                                                          //188/ 设置DMA使用的计数值的高8位——把值3输出到DMA计数寄存器端口5中
/* activate DMA 2 */                                                             //189/ 
	immoutb_p(0|2,10);                                                       //190/ 开启DMA通道2——把值0|2输出到DMA单屏蔽寄存器端口10中
	sti();                                                                   //191/ 开中断
}                                                                                //192/ 
                                                                                 //193/ 
static void output_byte(char byte)                                               //194/ [b;]向软盘控制器的数据寄存器输出一个字节的命令byte或参数byte
{                                                                                //195/ 
	int counter;                                                             //196/ 
	unsigned char status;                                                    //197/ 
                                                                                 //198/ 
	if (reset)                                                               //199/ 如果复位标志置位
		return;                                                          //200/ 则直接返回
	for(counter = 0 ; counter < 10000 ; counter++) {                         //201/ 循环读取软盘控制器的主状态寄存器值
		status = inb_p(FD_STATUS) & (STATUS_READY | STATUS_DIR);         //202/ 
		if (status == STATUS_READY) {                                    //203/ 如果传输方向是CPU->软盘控制器、软盘控制器的数据寄存器准备就绪
			outb(byte,FD_DATA);                                      //204/ 将参数byte写入软盘控制器中数据寄存器
			return;                                                  //205/ 返回
		}                                                                //206/ 
	}                                                                        //207/ 
	reset = 1;                                                               //208/ 如果循环了10000次还不能传输，则置位复位标志
	printk("Unable to send byte to FDC\n\r");                                //209/ 打印出错信息
}                                                                                //210/ 
                                                                                 //211/ 
static int result(void)                                                          //212/ [b;]读取软盘控制器执行的结果信息存放到reply_buffer数组中
{                                                                                //213/ 
	int i = 0, counter, status;                                              //214/ 
                                                                                 //215/ 
	if (reset)                                                               //216/ 如果复位标志置位
		return -1;                                                       //217/ 则直接返回
	for (counter = 0 ; counter < 10000 ; counter++) {                        //218/ 循环读取软盘控制器的主状态寄存器值
		status = inb_p(FD_STATUS)&(STATUS_DIR|STATUS_READY|STATUS_BUSY); //219/ 
		if (status == STATUS_READY)                                      //220/ 如果传输方向是CPU->软盘控制器、软盘控制器的数据寄存器准备就绪、软盘控制器不忙
			return i;                                                //221/ 则表示没有数据可取，返回已读取的字节数i
		if (status == (STATUS_DIR|STATUS_READY|STATUS_BUSY)) {           //222/ 如果传输方向是软盘控制器->CPU、软盘控制器的数据寄存器准备就绪、软盘控制器忙
			if (i >= MAX_REPLIES)                                    //223/ 则表示有数据可取，如果已读取的字节数已达到了MAX_REPLIES=7个字节的限制
				break;                                           //224/ 则不再读取
			reply_buffer[i++] = inb_p(FD_DATA);                      //225/ 否则就将软盘控制器中数据寄存器的值读出来存放到reply_buffer数组中
		}                                                                //226/ 
	}                                                                        //227/ 
	reset = 1;                                                               //228/ 如果循环了10000次还不能传输，则置位复位标志
	printk("Getstatus times out\n\r");                                       //229/ 打印出错信息
	return -1;                                                               //230/ 
}                                                                                //231/ 
                                                                                 //232/ 
static void bad_flp_intr(void)                                                   //233/ [b;]根据软盘读写出错次数来确定需要采取的进一步行动
{                                                                                //234/ 
	CURRENT->errors++;                                                       //235/ 将当前请求项出错次数增1
	if (CURRENT->errors > MAX_ERRORS) {                                      //236/ 如果当前请求项出错次数大于最大允许出错次数
		floppy_deselect(current_drive);                                  //237/ 则取消选定当前驱动器号current_drive(00-11)指定的当前软驱，复位软驱已选定标志，唤醒wait_on_floppy_select指向的任务
		end_request(0);                                                  //238/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值0，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
	}                                                                        //239/ 
	if (CURRENT->errors > MAX_ERRORS/2)                                      //240/ 如果当前请求项出错次数大于最大允许出错次数的一半
		reset = 1;                                                       //241/ 则置位复位标志
	else                                                                     //242/ 否则
		recalibrate = 1;                                                 //243/ 置位重新校正标志
}	                                                                         //244/ 
                                                                                 //245/ 
/*                                                                               //246/ 
 * Ok, this interrupt is called after a DMA read/write has succeeded,            //247/ 
 * so we check the results, and copy any buffers.                                //248/ 
 */                                                                              //249/ 
static void rw_interrupt(void)                                                   //250/ [b;]软盘读写操作中断调用函数
{                                                                                //251/ 
	if (result() != 7 || (ST0 & 0xf8) || (ST1 & 0xbf) || (ST2 & 0x73)) {     //252/ 读取软盘控制器执行的结果信息存放到reply_buffer数组中，如果返回结果字节数不等于7或者状态字节0、1、2中存在出错标志
		if (ST1 & 0x02) {                                                //253/ 如果出错标志是写保护
			printk("Drive %d is write protected\n\r",current_drive); //254/ 则打印写保护信息
			floppy_deselect(current_drive);                          //255/ 则取消选定当前驱动器号current_drive(00-11)指定的当前软驱，复位软驱已选定标志，唤醒wait_on_floppy_select指向的任务
			end_request(0);                                          //256/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值0，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
		} else                                                           //257/ 如果出错标志不是写保护
			bad_flp_intr();                                          //258/ 则根据软盘读写出错次数来确定需要采取的进一步行动
		do_fd_request();                                                 //259/ 如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
		return;                                                          //260/ 返回
	}                                                                        //261/ 
	if (command == FD_READ && (unsigned long)(CURRENT->buffer) >= 0x100000)  //262/ 如果软盘控制器命令是读扇区数据命令，并且当前请求项的缓冲块地址在1MB以上
		copy_buffer(tmp_floppy_area,CURRENT->buffer);                    //263/ 则从临时软盘缓冲区tmp_floppy_area处复制1KB数据到内存地址CURRENT->buffer处
	floppy_deselect(current_drive);                                          //264/ 取消选定当前驱动器号current_drive(00-11)指定的当前软驱，复位软驱已选定标志，唤醒wait_on_floppy_select指向的任务
	end_request(1);                                                          //265/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值1，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
	do_fd_request();                                                         //266/ 如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
}                                                                                //267/ 
                                                                                 //268/ 
inline void setup_rw_floppy(void)                                                //269/ [b;]设置DMA通道2并向软盘控制器输出命令和参数，若复位标志被置位，则执行软盘读写请求项操作函数do_fd_request
{                                                                                //270/ 
	setup_DMA();                                                             //271/ 对软盘控制器使用的DMA通道2进行设置(初始化)
	do_floppy = rw_interrupt;                                                //272/ 设置在中断处理程序中调用的函数为 软盘读写操作中断调用函数rw_interrupt
	output_byte(command);                                                    //273/ 向软盘控制器的数据寄存器输出软盘控制器命令command
	output_byte(head<<2 | current_drive);                                    //274/ 向软盘控制器的数据寄存器输出当前磁头号head + 当前驱动器号current_drive
	output_byte(track);                                                      //275/ 向软盘控制器的数据寄存器输出当前磁道号track
	output_byte(head);                                                       //276/ 向软盘控制器的数据寄存器输出当前磁头号head
	output_byte(sector);                                                     //277/ 向软盘控制器的数据寄存器输出起始扇区号sector
	output_byte(2);		/* sector size = 512 */                          //278/ 可能是向软盘控制器的数据寄存器输出扇区字节数的倍数(倍数 * 基数 = 每扇区字节数)
	output_byte(floppy->sect);                                               //279/ 向软盘控制器的数据寄存器输出软盘的每磁道扇区数
	output_byte(floppy->gap);                                                //280/ 向软盘控制器的数据寄存器输出软盘的扇区间隙长度(字节数)
	output_byte(0xFF);	/* sector size (0xff when n!=0 ?) */             //281/ 可能是向软盘控制器的数据寄存器输出扇区字节数的基数
	if (reset)                                                               //282/ 如果复位标志被置位
		do_fd_request();                                                 //283/ 则如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
}                                                                                //284/ 
                                                                                 //285/ 
/*                                                                               //286/ 
 * This is the routine called after every seek (or recalibrate) interrupt        //287/ 
 * from the floppy controller. Note that the "unexpected interrupt" routine      //288/ 
 * also does a recalibrate, but doesn't come here.                               //289/ 
 */                                                                              //290/ 
static void seek_interrupt(void)                                                 //291/ [b;]寻道处理结束后中断过程中调用的C函数
{                                                                                //292/ 
/* sense drive status */                                                         //293/ 
	output_byte(FD_SENSEI);                                                  //294/ 向软盘控制器的数据寄存器输出检测中断状态命令
	if (result() != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track) {        //295/ 读取软盘控制器执行的结果信息存放到reply_buffer数组中，如果返回结果字节数不等于2 或者 状态字节0不为寻道结束 或者 返回的当前磁头所在的磁道号不等于寻道号
		bad_flp_intr();                                                  //296/ 根据软盘读写出错次数来确定需要采取的进一步行动
		do_fd_request();                                                 //297/ 如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
		return;                                                          //298/ 返回
	}                                                                        //299/ 
	current_track = ST1;                                                     //300/ 将返回的当前磁头所在的磁道号赋给current_track
	setup_rw_floppy();                                                       //301/ 设置DMA通道2并向软盘控制器输出命令和参数，若复位标志被置位，则执行软盘读写请求项操作函数do_fd_request
}                                                                                //302/ 
                                                                                 //303/ 
/*                                                                               //304/ 
 * This routine is called when everything should be correctly set up             //305/ 
 * for the transfer (ie floppy motor is on and the correct floppy is             //306/ 
 * selected).                                                                    //307/ 
 */                                                                              //308/ 
static void transfer(void)                                                       //309/ [r;]读写数据传输函数
{                                                                                //310/ 
	if (cur_spec1 != floppy->spec1) {                                        //311/ 如果当前软盘参数(高4位是步进速率，低4位是磁头卸载时间)不等于当前请求项CURRENT中的设备号中的软盘类型对应的驱动器的参数
		cur_spec1 = floppy->spec1;                                       //312/ 则将当前请求项CURRENT中的设备号中的软盘类型对应的驱动器的参数赋给cur_spec1
		output_byte(FD_SPECIFY);                                         //313/ 向软盘控制器的数据寄存器输出设定驱动器参数命令
		output_byte(cur_spec1);		/* hut etc */                    //314/ 向软盘控制器的数据寄存器输出当前软盘参数(高4位是步进速率，低4位是磁头卸载时间)
		output_byte(6);			/* Head load time =6ms, DMA */   //315/ 向软盘控制器的数据寄存器输出6=0000011 0B(即磁头加载时间为3*2ms=6ms、DMA方式)
	}                                                                        //316/ 
	if (cur_rate != floppy->rate)                                            //317/ 如果当前软盘转速rate不等于当前请求项CURRENT中的设备号中的软盘类型对应的数据传输速率
		outb_p(cur_rate = floppy->rate,FD_DCR);                          //318/ 则则将当前请求项CURRENT中的设备号中的软盘类型对应的数据传输速率赋给cur_rate，并将该数据传输速率写入软盘控制器中的磁盘控制寄存器
	if (reset) {                                                             //319/ 如果复位标志已置位
		do_fd_request();                                                 //320/ 则如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
		return;                                                          //321/ 返回
	}                                                                        //322/ 
	if (!seek) {                                                             //323/ 如果寻道标志未置位
		setup_rw_floppy();                                               //324/ 则设置DMA通道2并向软盘控制器输出命令和参数，若复位标志被置位，则执行软盘读写请求项操作函数do_fd_request
		return;                                                          //325/ 返回
	}                                                                        //326/ 
	do_floppy = seek_interrupt;                                              //327/ 设置在中断处理程序中调用的函数为 寻道处理结束后中断过程中调用的C函数seek_interrupt
	if (seek_track) {                                                        //328/ 如果寻道号不为0
		output_byte(FD_SEEK);                                            //329/ 则向软盘控制器的数据寄存器输出磁头寻道命令
		output_byte(head<<2 | current_drive);                            //330/ 向软盘控制器的数据寄存器输出 当前磁头号+当前驱动器号
		output_byte(seek_track);                                         //331/ 向软盘控制器的数据寄存器输出寻道号
	} else {                                                                 //332/ 否则
		output_byte(FD_RECALIBRATE);                                     //333/ 向软盘控制器的数据寄存器输出重新校正命令
		output_byte(head<<2 | current_drive);                            //334/ 向软盘控制器的数据寄存器输出 当前磁头号+当前驱动器号
	}                                                                        //335/ 
	if (reset)                                                               //336/ 如果复位标志已置位
		do_fd_request();                                                 //337/ 则如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
}                                                                                //338/ 
                                                                                 //339/ 
/*                                                                               //340/ 
 * Special case - used after a unexpected interrupt (or reset)                   //341/ 
 */                                                                              //342/ 
static void recal_interrupt(void)                                                //343/ [b;]软驱重新校正中断调用函数
{                                                                                //344/ 
	output_byte(FD_SENSEI);                                                  //345/ 向软盘控制器的数据寄存器输出检测中断状态命令
	if (result()!=2 || (ST0 & 0xE0) == 0x60)                                 //346/ 读取软盘控制器执行的结果信息存放到reply_buffer数组中，如果返回结果字节数不等于2 或 返回结果指示命令异常结束
		reset = 1;                                                       //347/ 则置位复位标志
	else                                                                     //348/ 否则
		recalibrate = 0;                                                 //349/ 置位重新校正标志
	do_fd_request();                                                         //350/ 如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
}                                                                                //351/ 
                                                                                 //352/ 
void unexpected_floppy_interrupt(void)                                           //353/ [b;]意外软盘中断请求引发的软盘中断处理程序中调用的函数
{                                                                                //354/ 
	output_byte(FD_SENSEI);                                                  //355/ 向软盘控制器的数据寄存器输出检测中断状态命令
	if (result()!=2 || (ST0 & 0xE0) == 0x60)                                 //356/ 读取软盘控制器执行的结果信息存放到reply_buffer数组中，如果返回结果字节数不等于2 或 返回结果指示命令异常结束
		reset = 1;                                                       //357/ 则置位复位标志
	else                                                                     //358/ 否则
		recalibrate = 1;                                                 //359/ 置位重新校正标志
}                                                                                //360/ 
                                                                                 //361/ 
static void recalibrate_floppy(void)                                             //362/ [b;]重新校正软驱
{                                                                                //363/ 
	recalibrate = 0;                                                         //364/ 复位重新校正标志
	current_track = 0;                                                       //365/ 当前磁道号归零
	do_floppy = recal_interrupt;                                             //366/ 设置在中断处理程序中调用的函数为 软驱重新校正中断调用函数recal_interrupt
	output_byte(FD_RECALIBRATE);                                             //367/ 向软盘控制器的数据寄存器输出重新校正命令
	output_byte(head<<2 | current_drive);                                    //368/ 向软盘控制器的数据寄存器输出 当前磁头号+当前驱动器号
	if (reset)                                                               //369/ 如果复位标志已置位
		do_fd_request();                                                 //370/ 则如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
}                                                                                //371/ 
                                                                                 //372/ 
static void reset_interrupt(void)                                                //373/ [b;]软盘控制器复位中断调用函数
{                                                                                //374/ 
	output_byte(FD_SENSEI);                                                  //375/ 向软盘控制器的数据寄存器输出检测中断状态命令
	(void) result();                                                         //376/ 读取软盘控制器执行的结果信息存放到reply_buffer数组中
	output_byte(FD_SPECIFY);                                                 //377/ 向软盘控制器的数据寄存器输出设定驱动器参数命令
	output_byte(cur_spec1);		/* hut etc */                            //378/ 向软盘控制器的数据寄存器输出当前软盘参数(高4位是步进速率，低4位是磁头卸载时间)
	output_byte(6);			/* Head load time =6ms, DMA */           //379/ 向软盘控制器的数据寄存器输出6=0000011 0B(即磁头加载时间为3*2ms=6ms、DMA方式)
	do_fd_request();                                                         //380/ 如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
}                                                                                //381/ 
                                                                                 //382/ 
/*                                                                               //383/ 
 * reset is done by pulling bit 2 of DOR low for a while.                        //384/ 
 */                                                                              //385/ 
static void reset_floppy(void)                                                   //386/ [b;]复位软盘控制器
{                                                                                //387/ 
	int i;                                                                   //388/ 
                                                                                 //389/ 
	reset = 0;                                                               //390/ 将复位标志清0
	cur_spec1 = -1;                                                          //391/ 使当前软盘参数变量无效
	cur_rate = -1;                                                           //392/ 使当前软盘转速变量无效
	recalibrate = 1;                                                         //393/ 置位重新校正标志
	printk("Reset-floppy called\n\r");                                       //394/ 打印信息——“执行软盘复位”
	cli();                                                                   //395/ 关中断
	do_floppy = reset_interrupt;                                             //396/ 设置在中断处理程序中调用的函数为 软盘控制器复位中断调用函数reset_interrupt
	outb_p(current_DOR & ~0x04,FD_DOR);                                      //397/ 对软盘控制器执行复位操作——将(current_DOR & ~0x04)值放到al寄存器中写入FD_DOR指定的端口，并等待一会
	for (i=0 ; i<100 ; i++)                                                  //398/ 延迟等待一会
		__asm__("nop");                                                  //399/ 
	outb(current_DOR,FD_DOR);                                                //400/ 再启动软盘控制器——将current_DOR值放到al寄存器中写入FD_DOR指定的端口
	sti();                                                                   //401/ 开中断
}                                                                                //402/ 
                                                                                 //403/ 
static void floppy_on_interrupt(void)                                            //404/ [b;]软驱启动定时中断调用函数
{                                                                                //405/ 
/* We cannot do a floppy-select, as that might sleep. We just force it */        //406/ 
	selected = 1;                                                            //407/ 置位软驱已选定标志
	if (current_drive != (current_DOR & 3)) {                                //408/ 如果当前驱动器号与软盘控制器中的数字输出寄存器中指定的驱动器号不同
		current_DOR &= 0xFC;                                             //409/ 409-411行用于重新设置软盘控制器中的数字输出寄存器中指定的驱动器号为当期驱动器号current_drive
		current_DOR |= current_drive;                                    //410/ 
		outb(current_DOR,FD_DOR);                                        //411/ 
		add_timer(2,&transfer);                                          //412/ 延迟2个滴答后，执行[r;]r
	} else                                                                   //413/ 否则
		transfer();                                                      //414/ 则[r;]r
}                                                                                //415/ 
                                                                                 //416/ 
void do_fd_request(void)                                                         //417/ [b;]如果复位标志是置位的，则复位软盘控制器，然后返回；如果重新校正标志是置位的，则向重新校正软驱，然后返回；如果复位标志和重新校正标志都没置位，则执行软盘当前请求项的读写操作
{                                                                                //418/ 
	unsigned int block;                                                      //419/ 
                                                                                 //420/ 
	seek = 0;                                                                //421/ 复位寻道标志
	if (reset) {                                                             //422/ 如果复位标志已置位
		reset_floppy();                                                  //423/ 则复位软盘控制器
		return;                                                          //424/ 返回
	}                                                                        //425/ 
	if (recalibrate) {                                                       //426/ 如果重新校正标志已置位
		recalibrate_floppy();                                            //427/ 则重新校正软驱
		return;                                                          //428/ 返回
	}                                                                        //429/ 
	INIT_REQUEST;                                                            //430/ 初始化请求项宏——判断当前请求项合法性，若已没有请求项则退出，若当前请求项不合法则死机
	floppy = (MINOR(CURRENT->dev)>>2) + floppy_type;                         //431/ 将请求项指定的设备号中的软盘类型取出，用作软盘类型参数结构数组floppy_type[]的索引来取得指定软盘的参数块
	if (current_drive != CURRENT_DEV)                                        //432/ 如果当前驱动器号不等于当前请求项CURRENT中的设备号对应的软盘(驱动器)号--(0-3)
		seek = 1;                                                        //433/ 则置位寻道标志
	current_drive = CURRENT_DEV;                                             //434/ 将当前请求项CURRENT中的设备号指定的驱动器号赋给current_drive
	block = CURRENT->sector;                                                 //435/ 将当前请求项中的起始扇区号(从磁盘/分区开头算起)赋给block
	if (block+2 > floppy->size) {                                            //436/ 如果起始扇区号(从磁盘/分区开头算起)block超出了范围(因为每次读写是以1块，即2个扇区为单位)
		end_request(0);                                                  //437/ 则结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值0，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
		goto repeat;                                                     //438/ 跳转回INIT_REQUEST中的起始repeat处
	}                                                                        //439/ 
	sector = block % floppy->sect;                                           //440/ 计算出起始扇区号(从磁盘/分区开头算起)block对应的当前磁道上的扇区号赋给sector
	block /= floppy->sect;                                                   //441/ 
	head = block % floppy->head;                                             //442/ 计算出起始扇区号(从磁盘/分区开头算起)block对应的当前磁头号赋给head
	track = block / floppy->head;                                            //443/ 计算出起始扇区号(从磁盘/分区开头算起)block对应的当前磁道号赋给track
	seek_track = track << floppy->stretch;                                   //444/ 相应于软驱中软盘类型进行调整，得出寻道号赋给seek_track
	if (seek_track != current_track)                                         //445/ 如果寻道号与当前磁头所在磁道号不同
		seek = 1;                                                        //446/ 则置位寻道标志，要求执行寻道操作
	sector++;                                                                //447/ 修整当前磁道上的扇区号sector，因为磁盘上实际扇区计数是从1算起的
	if (CURRENT->cmd == READ)                                                //448/ 如果当前请求项是读操作
		command = FD_READ;                                               //449/ 则设置软盘控制器命令为读扇区数据命令
	else if (CURRENT->cmd == WRITE)                                          //450/ 而如果当前请求项是写操作
		command = FD_WRITE;                                              //451/ 则设置软盘控制器命令为写扇区数据命令
	else                                                                     //452/ 否则
		panic("do_fd_request: unknown command");                         //453/ 直接死机
	add_timer(ticks_to_floppy_on(current_drive),&floppy_on_interrupt);       //454/ 为内核添加定时器，当(ticks_to_floppy_on(current_drive))指定的滴答数减到0时，执行软驱启动定时中断调用函数floppy_on_interrupt
}                                                                                //455/ 
                                                                                 //456/ 
void floppy_init(void)                                                           //457/ [b;]软盘系统初始化
{                                                                                //458/ 
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;                           //459/ 找到块设备表(数组)中软盘对应的项，将该项中的请求操作的函数指针字段设置为DEVICE_REQUEST(即do_fd_request)
	set_trap_gate(0x26,&floppy_interrupt);                                   //460/ 在idt表中的第0x26=38(从0算起)个描述符的位置放置一个陷阱门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为&floppy_interrupt;P=1;DPL=0;TYPE=15->1111)
	outb(inb_p(0x21)&~0x40,0x21);                                            //461/ 复位8259主片的IR6屏蔽位，使得软盘中断可以传递到系统
}                                                                                //462/ 

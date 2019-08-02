#ifndef _BLK_H                                                         //  1/ 
#define _BLK_H                                                         //  2/ 
                                                                       //  3/ 
#define NR_BLK_DEV	7                                              //  4/ 块设备的数量
/*                                                                     //  5/ 
 * NR_REQUEST is the number of entries in the request-queue.           //  6/ 
 * NOTE that writes may use only the low 2/3 of these: reads           //  7/ 
 * take precedence.                                                    //  8/ 
 *                                                                     //  9/ 
 * 32 seems to be a reasonable number: enough to get some benefit      // 10/ 
 * from the elevator-mechanism, but not so much as to lock a lot of    // 11/ 
 * buffers when they are in the queue. 64 seems to be too many (easily // 12/ 
 * long pauses in reading when heavy writing/syncing is going on)      // 13/ 
 */                                                                    // 14/ 
#define NR_REQUEST	32                                             // 15/ 请求队列中所包含的项数。写操作仅使用这些项中低端的2/3项;读操作优先处理，即读操作总在写操作之前处理
                                                                       // 16/ 
/*                                                                     // 17/ 
 * Ok, this is an expanded form so that we can use the same            // 18/ 
 * request for paging requests when that is implemented. In            // 19/ 
 * paging, 'bh' is NULL, and 'waiting' is used to wait for             // 20/ 
 * read/write completion.                                              // 21/ 
 */                                                                    // 22/ 
struct request {                                                       // 23/ 请求列表中请求项的结构
	int dev;		/* -1 if no request */                 // 24/ 发请求的设备的设备号(主+次)，dev为-1表示队列中此请求项没有被使用
	int cmd;		/* READ or WRITE */                    // 25/ 0:读命令 or 1:写命令
	int errors;                                                    // 26/ 操作时产生的错误次数
	unsigned long sector;                                          // 27/ 起始扇区(1块 = 2扇区)
	unsigned long nr_sectors;                                      // 28/ 读/写扇区数
	char * buffer;                                                 // 29/ 数据缓冲区
	struct task_struct * waiting;                                  // 30/ 任务等待操作执行完成的地方
	struct buffer_head * bh;                                       // 31/ 缓冲区头指针
	struct request * next;                                         // 32/ 指向下一请求项
};                                                                     // 33/ 
                                                                       // 34/ 
/*                                                                     // 35/ 
 * This is used in the elevator algorithm: Note that                   // 36/ 
 * reads always go before writes. This is natural: reads               // 37/ 
 * are much more time-critical than writes.                            // 38/ 
 */                                                                    // 39/ 
#define IN_ORDER(s1,s2) \                                              // 40/ 根据电梯算法，如果s1指向的请求项在s2指向的请求项之前进行，则返回1；否则返回0。电梯算法的判断准则，1.读操作的请求项总是在写操作之前进行；2.操作命令相同的情况下，设备号(主+次)小请求项的在设备号(主+次)大的之前进行；3.操作命令和设备号(主+次)都相同的情况下，起始扇区小的请求项在起始扇区大的之前进行
((s1)->cmd<(s2)->cmd || (s1)->cmd==(s2)->cmd && \                      // 41/ 
((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \                 // 42/ 
(s1)->sector < (s2)->sector)))                                         // 43/ 
                                                                       // 44/ 
struct blk_dev_struct {                                                // 45/ 块设备结构
	void (*request_fn)(void);                                      // 46/ 请求操作的函数指针
	struct request * current_request;                              // 47/ 当前正在处理的请求信息结构
};                                                                     // 48/ 
                                                                       // 49/ 
extern struct blk_dev_struct blk_dev[NR_BLK_DEV];                      // 50/ 块设备表(数组)，每种块设备占用一项
extern struct request request[NR_REQUEST];                             // 51/ 请求项队列数组
extern struct task_struct * wait_for_request;                          // 52/ 等待空闲请求项的进程队列头指针
                                                                       // 53/ 
#ifdef MAJOR_NR                                                        // 54/ 主设备号
                                                                       // 55/ 
/*                                                                     // 56/ 
 * Add entries as needed. Currently the only block devices             // 57/ 
 * supported are hard-disks and floppies.                              // 58/ 
 */                                                                    // 59/ 
                                                                       // 60/ 
#if (MAJOR_NR == 1)                                                    // 61/ 若主设备号为1，表示块设备是内存虚拟盘
/* ram disk */                                                         // 62/ 
#define DEVICE_NAME "ramdisk"                                          // 63/ 设备名称-ramdisk
#define DEVICE_REQUEST do_rd_request                                   // 64/ 设备请求函数-do_rd_request()
#define DEVICE_NR(device) ((device) & 7)                               // 65/ 次设备号-(0-7)
#define DEVICE_ON(device)                                              // 66/ 开启设备宏。虚拟盘无需开启或关闭
#define DEVICE_OFF(device)                                             // 67/ 关闭设备宏
                                                                       // 68/ 
#elif (MAJOR_NR == 2)                                                  // 69/ 若主设备号为2，表示块设备是软盘
/* floppy */                                                           // 70/ 
#define DEVICE_NAME "floppy"                                           // 71/ 设备名称-floppy
#define DEVICE_INTR do_floppy                                          // 72/ 设备中断处理程序-do_floppy()
#define DEVICE_REQUEST do_fd_request                                   // 73/ 设备请求函数-do_fd_request()
#define DEVICE_NR(device) ((device) & 3)                               // 74/ 次设备号-(0-3)
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))                 // 75/ 开启设备宏
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))               // 76/ 关闭设备宏
                                                                       // 77/ 
#elif (MAJOR_NR == 3)                                                  // 78/ 若主设备号为3，表示块设备是硬盘
/* harddisk */                                                         // 79/ 
#define DEVICE_NAME "harddisk"                                         // 80/ 设备名称-harddisk
#define DEVICE_INTR do_hd                                              // 81/ 设备中断处理程序-do_hd()
#define DEVICE_REQUEST do_hd_request                                   // 82/ 设备请求函数-do_hd_request()
#define DEVICE_NR(device) (MINOR(device)/5)                            // 83/ 通过设备号device得出硬盘(驱动器)号(0或1)
#define DEVICE_ON(device)                                              // 84/ 开启设备宏。硬盘一直在工作，无需开启或关闭
#define DEVICE_OFF(device)                                             // 85/ 关闭设备宏。硬盘一直在工作，无需开启或关闭
                                                                       // 86/ 
#elif                                                                  // 87/ 
/* unknown blk device */                                               // 88/ 
#error "unknown blk device"                                            // 89/ 当预处理器预处理到#error命令时将停止编译并输出错误消息-"unknown blk device"
                                                                       // 90/ 
#endif                                                                 // 91/ 
                                                                       // 92/ 
#define CURRENT (blk_dev[MAJOR_NR].current_request)                    // 93/ 指定主设备号的当前请求项指针
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)                            // 94/ 当前请求项CURRENT中的设备号对应的硬盘(驱动器)号--(0或1) 或 软盘(驱动器)号--(0-3)
                                                                       // 95/ 
#ifdef DEVICE_INTR                                                     // 96/ 如果定义了设备中断处理函数符号宏
void (*DEVICE_INTR)(void) = NULL;                                      // 97/ 则将其定义的函数名声明为一个函数指针，并默认为NULL(比如用来定义了 设备中断处理程序-do_hd())
#endif                                                                 // 98/ 
static void (DEVICE_REQUEST)(void);                                    // 99/ 将符号常数DEVICE_REGUEST声明为一个不带参数并无返回值的静态函数指针(比如用来定义了 设备请求函数-do_hd_request())
                                                                       //100/ 
extern inline void unlock_buffer(struct buffer_head * bh)              //101/ [b;]解锁指定的缓冲区(块)，并唤醒等待该缓冲区的进程
{                                                                      //102/ 
	if (!bh->b_lock)                                               //103/ 如果指定的缓冲块bh没有上锁
		printk(DEVICE_NAME ": free buffer being unlocked\n");  //104/     则显示警告信息。
	bh->b_lock=0;                                                  //105/ 如果上了锁就将其解锁
	wake_up(&bh->b_wait);                                          //106/ 唤醒等待该缓冲区的进程
}                                                                      //107/ 
                                                                       //108/ 
extern inline void end_request(int uptodate)                           //109/ [b;]结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值uptodate，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
{                                                                      //110/ 
	DEVICE_OFF(CURRENT->dev);                                      //111/ 关闭设备
	if (CURRENT->bh) {                                             //112/ 如果当前请求项指定的缓冲头结构不为空
		CURRENT->bh->b_uptodate = uptodate;                    //113/     设置该缓冲块的数据更新标志为参数值uptodate
		unlock_buffer(CURRENT->bh);                            //114/     解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程
	}                                                              //115/ 
	if (!uptodate) {                                               //116/ 若参数uptodate指定的更新标志为0，表示当前请求项的操作失败
		printk(DEVICE_NAME " I/O error\n\r");                  //117/     则显示出错信息
		printk("dev %04x, block %d\n\r",CURRENT->dev,          //118/ 
			CURRENT->bh->b_blocknr);                       //119/ 
	}                                                              //120/ 
	wake_up(&CURRENT->waiting);                                    //121/ 唤醒CURRENT->waiting指向的任务
	wake_up(&wait_for_request);                                    //122/ 唤醒wait_for_request指向的任务
	CURRENT->dev = -1;                                             //123/ 释放当前请求项
	CURRENT = CURRENT->next;                                       //124/ 从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
}                                                                      //125/ 
                                                                       //126/ 
#define INIT_REQUEST \                                                 //127/ [b;]初始化请求项宏——判断当前请求项合法性，若已没有请求项则退出，若当前请求项不合法则死机
repeat: \                                                              //128/ 
	if (!CURRENT) \                                                //129/ 如果当前请求结构指针为null则返回
		return; \                                              //130/ 
	if (MAJOR(CURRENT->dev) != MAJOR_NR) \                         //131/ 如果当前请求对应的设备的主设备号与当前指定的主设备号不匹配则死机
		panic(DEVICE_NAME ": request list destroyed"); \       //132/ 
	if (CURRENT->bh) { \                                           //133/ 
		if (!CURRENT->bh->b_lock) \                            //134/ 如果当前请求项指定的缓冲块没被锁定则死机
			panic(DEVICE_NAME ": block not locked"); \     //135/ 
	}                                                              //136/ 
                                                                       //137/ 
#endif                                                                 //138/ 
                                                                       //139/ 
#endif                                                                 //140/ 

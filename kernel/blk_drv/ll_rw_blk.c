/*                                                                            //  1/ 
 *  linux/kernel/blk_dev/ll_rw.c                                              //  2/ 
 *                                                                            //  3/ 
 * (C) 1991 Linus Torvalds                                                    //  4/ 
 */                                                                           //  5/ 
                                                                              //  6/ 
/*                                                                            //  7/ 
 * This handles all read/write requests to block devices                      //  8/ 
 */                                                                           //  9/ 
#include <errno.h>                                                            // 10/ 
#include <linux/sched.h>                                                      // 11/ 
#include <linux/kernel.h>                                                     // 12/ 
#include <asm/system.h>                                                       // 13/ 
                                                                              // 14/ 
#include "blk.h"                                                              // 15/ 
                                                                              // 16/ 
/*                                                                            // 17/ 
 * The request-struct contains all necessary data                             // 18/ 
 * to load a nr of sectors into memory                                        // 19/ 
 */                                                                           // 20/ 
struct request request[NR_REQUEST];                                           // 21/ 请求项数组，共有NR_REQUEST = 32个请求项
                                                                              // 22/ 
/*                                                                            // 23/ 
 * used to wait on when there are no free requests                            // 24/ 
 */                                                                           // 25/ 
struct task_struct * wait_for_request = NULL;                                 // 26/ 是用于在请求数组没有空闲项时进程的临时等待处
                                                                              // 27/ 
/* blk_dev_struct is:                                                         // 28/ 
 *	do_request-address                                                    // 29/ 
 *	next-request                                                          // 30/ 
 */                                                                           // 31/ 
struct blk_dev_struct blk_dev[NR_BLK_DEV] = {                                 // 32/ 块设备表(数组)，共NR_BLK_DEV = 7项，用块设备的主设备号作为索引
	{ NULL, NULL },		/* no_dev */                                  // 33/ 
	{ NULL, NULL },		/* dev mem */                                 // 34/ 
	{ NULL, NULL },		/* dev fd */                                  // 35/ 
	{ NULL, NULL },		/* dev hd */                                  // 36/ 
	{ NULL, NULL },		/* dev ttyx */                                // 37/ 
	{ NULL, NULL },		/* dev tty */                                 // 38/ 
	{ NULL, NULL }		/* dev lp */                                  // 39/ 
};                                                                            // 40/ 
                                                                              // 41/ 
static inline void lock_buffer(struct buffer_head * bh)                       // 42/ [b;]锁定bh指定的缓冲块，若已被锁定，则当前任务先进入不可中断的睡眠状态，醒来后再锁定
{                                                                             // 43/ 
	cli();                                                                // 44/ 关中断
	while (bh->b_lock)                                                    // 45/ 如果指定的缓冲块已被锁定
		sleep_on(&bh->b_wait);                                        // 46/ 则把当前任务置为不可中断的睡眠状态，让当前任务的tmp指向bh->b_wait指向的旧睡眠队列头，而让bh->b_wait指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up唤醒了bh->b_wait指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
	bh->b_lock=1;                                                         // 47/ 立即锁定该缓冲区
	sti();                                                                // 48/ 开中断
}                                                                             // 49/ 
                                                                              // 50/ 
static inline void unlock_buffer(struct buffer_head * bh)                     // 51/ [b;]释放(解锁)bh指定的锁定的缓冲块，并唤醒bh->b_wait指向的任务
{                                                                             // 52/ 
	if (!bh->b_lock)                                                      // 53/ 如果指定的缓冲块未被锁定
		printk("ll_rw_block.c: buffer not locked\n\r");               // 54/ 则打印出错信息
	bh->b_lock = 0;                                                       // 55/ 清锁定标志
	wake_up(&bh->b_wait);                                                 // 56/ 唤醒bh->b_wait指向的任务，bh->b_wait是任务等待队列头指针
}                                                                             // 57/ 
                                                                              // 58/ 
/*                                                                            // 59/ 
 * add-request adds a request to the linked list.                             // 60/ 
 * It disables interrupts so that it can muck with the                        // 61/ 
 * request-lists in peace.                                                    // 62/ 
 */                                                                           // 63/ 
static void add_request(struct blk_dev_struct * dev, struct request * req)    // 64/ [b;]将已经设置好的请求项req添加到dev块设备项指定设备的请求项链表中
{                                                                             // 65/ 
	struct request * tmp;                                                 // 66/ 新建一个临时请求项指针
                                                                              // 67/ 
	req->next = NULL;                                                     // 68/ 对请求项req的某些字段进行一些初始设置。置空请求项req中的下一请求项指针
	cli();                                                                // 69/ 关中断
	if (req->bh)                                                          // 70/ 如果请求项的缓冲块头指针指向的缓冲头结构存在
		req->bh->b_dirt = 0;                                          // 71/ 则将该缓冲头结构中的“脏”标志复位
	if (!(tmp = dev->current_request)) {                                  // 72/ 将参数dev指定设备的当前请求项指针赋给tmp，如果为空，表示目前该设备没有请求项
		dev->current_request = req;                                   // 73/ 则直接将其当前请求项设为参数req指定的请求项
		sti();                                                        // 74/ 开中断
		(dev->request_fn)();                                          // 75/ 执行请求函数，对于硬盘就是do_hd_request()
		return;                                                       // 76/ 
	}                                                                     // 77/ 
	for ( ; tmp->next ; tmp=tmp->next)                                    // 78/ 如果参数dev指定设备的当前请求项不为空，则遍寻整个请求队列，利用电梯算法将req指定的请求项插入到最佳位置(电梯算法的作用是让磁盘磁头的移动距离最小，从而改善硬盘访问时间)
		if ((IN_ORDER(tmp,req) ||                                     // 79/ 如果tmp指向的请求项在req指向的请求项之前进行，则返回1；否则返回0
		    !IN_ORDER(tmp,tmp->next)) &&                              // 80/ [r;]此行的条件似乎多余
		    IN_ORDER(req,tmp->next))                                  // 81/ 
			break;                                                // 82/ 找出了合适的位置为“tmp先于req先于tmp->next”，跳出循环，不再继续查找
	req->next=tmp->next;                                                  // 83/ 将req指向的请求项插入tmp->next指向的请求项之前
	tmp->next=req;                                                        // 84/ 将req指向的请求项插入tmp指向的请求项之后
	sti();                                                                // 85/ 开中断
}                                                                             // 86/ 
                                                                              // 87/ 
static void make_request(int major,int rw, struct buffer_head * bh)           // 88/ [b;]创建请求项并插入请求队列中(major:主设备号;rw:操作命令;bh:存放数据的缓冲头指针)
{                                                                             // 89/ 
	struct request * req;                                                 // 90/ 
	int rw_ahead;                                                         // 91/ 
                                                                              // 92/ 
/* WRITEA/READA is special case - it is not really needed, so if the */       // 93/ 
/* buffer is locked, we just forget about it, else it's a normal read */      // 94/ 
	if (rw_ahead = (rw == READA || rw == WRITEA)) {                       // 95/ 如果操作命令是READA或者WRITEA
		if (bh->b_lock)                                               // 96/ 如果当bh指向的缓存块被锁住时
			return;                                               // 97/ 直接放弃预读/写请求
		if (rw == READA)                                              // 98/ 如果bh指向的缓存块没被锁住，则READA或者WRITEA按普通的READ或者WRITE处理
			rw = READ;                                            // 99/ 
		else                                                          //100/ 
			rw = WRITE;                                           //101/ 
	}                                                                     //102/ 
	if (rw!=READ && rw!=WRITE)                                            //103/ 如果参数不是READA、WRITEA、READ、WRITE中的一个，直接死机
		panic("Bad block dev command, must be R/W/RA/WA");            //104/ 
	lock_buffer(bh);                                                      //105/ 锁定bh指定的缓冲块，若已被锁定，则当前任务先进入不可中断的睡眠状态，醒来后再锁定——开始准备创建请求项
	if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate)) { //106/ 在两种情况下缓冲块的数据与设备上一致，直接退出，不需创建请求项:1.命令是写，但缓冲块的数据被读入后并未修改；2.命令是读，但缓冲块中的数据已经是更新过的.
		unlock_buffer(bh);                                            //107/ 释放(解锁)bh指定的锁定的缓冲块，并唤醒bh->b_wait指向的任务，bh->b_wait是任务等待队列头指针
		return;                                                       //108/ 
	}                                                                     //109/ 
repeat:                                                                       //110/ 
/* we don't allow the write-requests to fill up the queue completely:         //111/ 
 * we want some room for reads: they take precedence. The last third          //112/ 
 * of the requests are only for reads.                                        //113/ 
 */                                                                           //114/ 
	if (rw == READ)                                                       //115/ 如果是读操作，从请求项数组的末端开始往前查找空闲项
		req = request+NR_REQUEST;                                     //116/ 
	else                                                                  //117/ 如果是写操作，从请求项数组的2/3处开始往前查找空闲项
		req = request+((NR_REQUEST*2)/3);                             //118/ 
/* find an empty request */                                                   //119/ 
	while (--req >= request)                                              //120/ 
		if (req->dev<0)                                               //121/ 如果找到某一请求项的dev为-1，表示其为空闲项，直接跳出循环
			break;                                                //122/ 
/* if none found, sleep on new requests: check for rw_ahead */                //123/ 
	if (req < request) {                                                  //124/ 如果已搜索到头，即没有找到空闲项
		if (rw_ahead) {                                               //125/ 如果是提起读/写
			unlock_buffer(bh);                                    //126/ 则直接解锁bh指定的缓冲块退出
			return;                                               //127/ 
		}                                                             //128/ 
		sleep_on(&wait_for_request);                                  //129/ 如果是普通的读/写，则直接进入不可中断的睡眠等待
		goto repeat;                                                  //130/ 醒来后重新搜索空闲项
	}                                                                     //131/ 
/* fill up the request-info, and add it to the queue */                       //132/ 开始创建请求项，向空闲请求项中填写请求信息
	req->dev = bh->b_dev;                                                 //133/ 设备号(主+次)
	req->cmd = rw;                                                        //134/ 操作命令
	req->errors=0;                                                        //135/ 操作时产生的错误次数
	req->sector = bh->b_blocknr<<1;                                       //136/ 起始扇区(1块 = 2扇区)
	req->nr_sectors = 2;                                                  //137/ 读/写扇区数
	req->buffer = bh->b_data;                                             //138/ 数据缓冲区
	req->waiting = NULL;                                                  //139/ 任务等待操作执行完成的地方
	req->bh = bh;                                                         //140/ 缓冲块头指针
	req->next = NULL;                                                     //141/ 指向下一请求项
	add_request(major+blk_dev,req);                                       //142/ 将已经设置好的请求项req添加到major+blk_dev块设备项指定设备的请求项链表中
}                                                                             //143/ 
                                                                              //144/ 
void ll_rw_block(int rw, struct buffer_head * bh)                             //145/ [b;]判断bh指向的缓冲头结构对应的设备的主设备号和请求操作函数是否有效，如果有效则创建请求项并插入请求队列中
{                                                                             //146/ 
	unsigned int major;                                                   //147/ 
                                                                              //148/ 
	if ((major=MAJOR(bh->b_dev)) >= NR_BLK_DEV ||                         //149/ 如果设备的主设备号超过了范围(>=7)或该设备的请求操作函数不存在
	!(blk_dev[major].request_fn)) {                                       //150/ 
		printk("Trying to read nonexistent block-device\n\r");        //151/ 则显示出错信息，并返回
		return;                                                       //152/ 
	}                                                                     //153/ 
	make_request(major,rw,bh);                                            //154/ 创建请求项并插入请求队列中(major:主设备号;rw:操作命令;bh:存放数据的缓冲头指针)
}                                                                             //155/ 
                                                                              //156/ 
void blk_dev_init(void)                                                       //157/ [b;]初始化请求数组，将所有(共NR_REQUEST = 32个)请求项置为空闲项
{                                                                             //158/ 
	int i;                                                                //159/ 
                                                                              //160/ 
	for (i=0 ; i<NR_REQUEST ; i++) {                                      //161/ 
		request[i].dev = -1;                                          //162/ 
		request[i].next = NULL;                                       //163/ 
	}                                                                     //164/ 
}                                                                             //165/ 

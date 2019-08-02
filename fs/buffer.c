/*                                                                              //  1/ 
 *  linux/fs/buffer.c                                                           //  2/ 
 *                                                                              //  3/ 
 *  (C) 1991  Linus Torvalds                                                    //  4/ 
 */                                                                             //  5/ 
                                                                                //  6/ 
/*                                                                              //  7/ 
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have      //  8/ 
 * been avoided by NEVER letting a interrupt change a buffer (except for the    //  9/ 
 * data, of course), but instead letting the caller do it. NOTE! As interrupts  // 10/ 
 * can wake up a caller, some cli-sti sequences are needed to check for         // 11/ 
 * sleep-on-calls. These should be extremely quick, though (I hope).            // 12/ 
 */                                                                             // 13/ 
                                                                                // 14/ 
/*                                                                              // 15/ 
 * NOTE! There is one discordant note here: checking floppies for               // 16/ 
 * disk change. This is where it fits best, I think, as it should               // 17/ 
 * invalidate changed floppy-disk-caches.                                       // 18/ 
 */                                                                             // 19/ 
                                                                                // 20/ 
#include <stdarg.h>                                                             // 21/ 
                                                                                // 22/ 
#include <linux/config.h>                                                       // 23/ 
#include <linux/sched.h>                                                        // 24/ 
#include <linux/kernel.h>                                                       // 25/ 
#include <asm/system.h>                                                         // 26/ 
#include <asm/io.h>                                                             // 27/ 
                                                                                // 28/ 
extern int end;                                                                 // 29/ 由ld生成，用于表明内核代码的末端，即指明内核模块结束位置后的第一个地址
struct buffer_head * start_buffer = (struct buffer_head *) &end;                // 30/ 这里用来表明高速缓冲区开始于内核代码末端位置，即内核模块结束位置后的第一个地址。[r;]end之前是不是不应该加&
struct buffer_head * hash_table[NR_HASH];                                       // 31/ 定义高速缓冲块的缓冲头结构的哈希表结构，长度为307项
static struct buffer_head * free_list;                                          // 32/ 定义高速缓冲块的空闲缓冲块链表头指针
static struct task_struct * buffer_wait = NULL;                                 // 33/ 等待空闲缓冲块而睡眠的任务队列
int NR_BUFFERS = 0;                                                             // 34/ 高速缓冲区中划分出来的高速缓冲块的总数目
                                                                                // 35/ 
static inline void wait_on_buffer(struct buffer_head * bh)                      // 36/ [b;]如果bh指定的缓冲块已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
{                                                                               // 37/ 
	cli();                                                                  // 38/ 关中断
	while (bh->b_lock)                                                      // 39/ 如果高速缓冲块已被上锁则进程进入睡眠，等待其解锁
		sleep_on(&bh->b_wait);                                          // 40/ 把当前任务置为不可中断的睡眠状态，让当前任务的tmp指向bh->b_wait指向的旧睡眠队列头，而让bh->b_wait指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up唤醒了*p指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
	sti();                                                                  // 41/ 开中断
}                                                                               // 42/ 
                                                                                // 43/ 
int sys_sync(void)                                                              // 44/ [b;]同步设备和内存高速缓冲中的数据
{                                                                               // 45/ 
	int i;                                                                  // 46/ 
	struct buffer_head * bh;                                                // 47/ 
                                                                                // 48/ 
	sync_inodes();		/* write out inodes into buffers */             // 49/ 把内存i节点表中的所有i节点与设备上的i节点作同步操作
	bh = start_buffer;                                                      // 50/ 将bh指向高速缓冲区的开始处
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {                                   // 51/ 遍寻NR_BUFFERS个缓冲头结构
		wait_on_buffer(bh);                                             // 52/ 如果bh指定的缓冲块已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
		if (bh->b_dirt)                                                 // 53/ 如果缓冲托bh指定的缓冲块中数据已修改
			ll_rw_block(WRITE,bh);                                  // 54/ 判断bh指向的缓冲头结构对应的设备的主设备号和请求操作函数是否有效，如果有效则创建请求项并插入请求队列中，使得设备与高速缓冲区数据同步
	}                                                                       // 55/ 
	return 0;                                                               // 56/ 
}                                                                               // 57/ 
                                                                                // 58/ 
int sync_dev(int dev)                                                           // 59/ [b;]对设备号dev指定的设备进行高速缓冲中数据与设备上数据的同步操作(之所以采用两遍同步是为了提高内核执行效率)
{                                                                               // 60/ 
	int i;                                                                  // 61/ 
	struct buffer_head * bh;                                                // 62/ 
                                                                                // 63/ 
	bh = start_buffer;                                                      // 64/ 
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {                                   // 65/ 遍寻高速缓冲区中的所有缓冲块
		if (bh->b_dev != dev)                                           // 66/ 如果当前缓冲块不是设备号dev指定的设备的缓冲块
			continue;                                               // 67/ 则继续检查下一个
		wait_on_buffer(bh);                                             // 68/ 如果是设备号dev指定的设备的缓冲块，如果已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
		if (bh->b_dev == dev && bh->b_dirt)                             // 69/ 经过睡眠后，再次判断，如果该缓冲块是设备号dev指定的设备的缓冲块，且已被修改
			ll_rw_block(WRITE,bh);                                  // 70/ 则判断bh指向的缓冲头结构对应的设备的主设备号和请求操作函数是否有效，如果有效则创建请求项并插入请求队列中
	}                                                                       // 71/ 
	sync_inodes();                                                          // 72/ 把内存i节点表中的所有i节点与设备上的i节点作同步操作
	bh = start_buffer;                                                      // 73/ 73-80行执行与上面同样的同步操作，用于将由于i节点同步操作而变‘脏’的缓冲块与设备中数据同步
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {                                   // 74/ 
		if (bh->b_dev != dev)                                           // 75/ 
			continue;                                               // 76/ 
		wait_on_buffer(bh);                                             // 77/ 
		if (bh->b_dev == dev && bh->b_dirt)                             // 78/ 
			ll_rw_block(WRITE,bh);                                  // 79/ 
	}                                                                       // 80/ 
	return 0;                                                               // 81/ 
}                                                                               // 82/ 
                                                                                // 83/ 
void inline invalidate_buffers(int dev)                                         // 84/ [b;]对设备号dev指定的设备在高速缓冲中缓冲块复位其更新(有效)标志和已修改标志，使指定设备在高速缓冲区中的数据无效
{                                                                               // 85/ 
	int i;                                                                  // 86/ 
	struct buffer_head * bh;                                                // 87/ 
                                                                                // 88/ 
	bh = start_buffer;                                                      // 89/ 
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {                                   // 90/ 遍寻高速缓冲区中的所有缓冲块
		if (bh->b_dev != dev)                                           // 91/ 如果当前缓冲块不是设备号dev指定的设备的缓冲块
			continue;                                               // 92/ 则继续检查下一个
		wait_on_buffer(bh);                                             // 93/ 如果是设备号dev指定的设备的缓冲块，如果已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
		if (bh->b_dev == dev)                                           // 94/ 经过睡眠后，再次判断，如果该缓冲块是设备号dev指定的设备的缓冲块
			bh->b_uptodate = bh->b_dirt = 0;                        // 95/ 则复位该缓冲块的更新(有效)标志和已修改标志
	}                                                                       // 96/ 
}                                                                               // 97/ 
                                                                                // 98/ 
/*                                                                              // 99/ 
 * This routine checks whether a floppy has been changed, and                   //100/ 
 * invalidates all buffer-cache-entries in that case. This                      //101/ 
 * is a relatively slow routine, so we have to try to minimize using            //102/ 
 * it. Thus it is called only upon a 'mount' or 'open'. This                    //103/ 
 * is the best way of combining speed and utility, I think.                     //104/ 
 * People changing diskettes in the middle of an operation deserve              //105/ 
 * to loose :-)                                                                 //106/ 
 *                                                                              //107/ 
 * NOTE! Although currently this is only for floppies, the idea is              //108/ 
 * that any additional removable block-device will use this routine,            //109/ 
 * and that mount/open needn't know that floppies/whatever are                  //110/ 
 * special.                                                                     //111/ 
 */                                                                             //112/ 
void check_disk_change(int dev)                                                 //113/ [b;]检查设备号dev指定的设备是否更换，如果已更换则释放该设备对应的内存中超级块结构、释放该设备在内存i节点表中的所有i节点，对该设备在高速缓冲中对应的缓冲块复位其更新(有效)标志和已修改标志，使该设备在高速缓冲区中的数据无效
{                                                                               //114/ 
	int i;                                                                  //115/ 
                                                                                //116/ 
	if (MAJOR(dev) != 2)                                                    //117/ 先检测一些设备号dev指定的设备是不是软盘。因为当时仅支持软盘可移动介质
		return;                                                         //118/ 如果不是软盘则退出
	if (!floppy_change(dev & 0x03))                                         //119/ 检测软驱号(dev & 0x03)指定软驱中软盘的更改情况，返回1表示已更换，返回0表示未更换
		return;                                                         //120/ 如果未更换则退出
	for (i=0 ; i<NR_SUPER ; i++)                                            //121/ 遍寻NR_SUPER=8个内存中超级块结构
		if (super_block[i].s_dev == dev)                                //122/ 如果内存中超级块super_block[i]就是参数dev指定的设备的超级块
			put_super(super_block[i].s_dev);                        //123/ 则释放设备号super_block[i].s_dev指定的设备对应的内存中超级块结构(置其中的s_dev为0)，并释放该设备i节点位图和逻辑块位图所占用的高速缓冲块
	invalidate_inodes(dev);                                                 //124/ 释放设备号dev指定的设备在内存i节点表中的所有i节点
	invalidate_buffers(dev);                                                //125/ 对设备号dev指定的设备在高速缓冲中缓冲块复位其更新(有效)标志和已修改标志，使指定设备在高速缓冲区中的数据无效
}                                                                               //126/ 
                                                                                //127/ 
#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)                    //128/ 使用设备号dev和逻辑块号，根据缓冲头的哈希表结构给出的算法，算出哈希表中对应项的序号
#define hash(dev,block) hash_table[_hashfn(dev,block)]                          //129/ 使用设备号dev和逻辑块号，返回缓冲头的哈希表结构中对应的项(指向一个缓冲头队列的第一项)
                                                                                //130/ 
static inline void remove_from_queues(struct buffer_head * bh)                  //131/ [b;]从HASH表中的一个队列和空闲链表队列中移走bh指定的缓冲头
{                                                                               //132/ 
/* remove from hash-queue */                                                    //133/ 从HASH队列中移除bh指定的缓冲头
	if (bh->b_next)                                                         //134/ 
		bh->b_next->b_prev = bh->b_prev;                                //135/ 
	if (bh->b_prev)                                                         //136/ 
		bh->b_prev->b_next = bh->b_next;                                //137/ 
	if (hash(bh->b_dev,bh->b_blocknr) == bh)                                //138/ 如果bh指向的缓冲头是HASH表中队列的头一个
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;                     //139/ 则使HASH表中的对应项指向本队列的下一个缓冲区
/* remove from free list */                                                     //140/ 从空闲链表中移除bh指定的缓冲头
	if (!(bh->b_prev_free) || !(bh->b_next_free))                           //141/ 因为空闲链表是双向循环链表，所以如果bh指向的缓冲头的前一项或下一项为空
		panic("Free block list corrupted");                             //142/ 则直接死机
	bh->b_prev_free->b_next_free = bh->b_next_free;                         //143/ 
	bh->b_next_free->b_prev_free = bh->b_prev_free;                         //144/ 
	if (free_list == bh)                                                    //145/ 
		free_list = bh->b_next_free;                                    //146/ 
}                                                                               //147/ 
                                                                                //148/ 
static inline void insert_into_queues(struct buffer_head * bh)                  //149/ [b;]将bh指向的缓冲头插入空闲链表尾部，同时放入HASH队列中
{                                                                               //150/ 
/* put at end of free list */                                                   //151/ 将bh指向的缓冲头放入空闲链表末尾处
	bh->b_next_free = free_list;                                            //152/ 
	bh->b_prev_free = free_list->b_prev_free;                               //153/ 
	free_list->b_prev_free->b_next_free = bh;                               //154/ 
	free_list->b_prev_free = bh;                                            //155/ 
/* put the buffer in new hash-queue if it has a device */                       //156/ 如果bh指向的缓冲头被一个设备使用着，将其插入到HASH表中对应的缓冲头队列的开头
	bh->b_prev = NULL;                                                      //157/ 
	bh->b_next = NULL;                                                      //158/ 
	if (!bh->b_dev)                                                         //159/ 
		return;                                                         //160/ 
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);                             //161/ 
	hash(bh->b_dev,bh->b_blocknr) = bh;                                     //162/ 
	bh->b_next->b_prev = bh;                                                //163/ 
}                                                                               //164/ 
                                                                                //165/ 
static struct buffer_head * find_buffer(int dev, int block)                     //166/ [b;]利用HASH表查询设备号dev指定的设备的逻辑块号为block的缓冲块是否已在高速缓冲区中，如果在就返回指向其对应缓冲头结构的指针
{		                                                                //167/ 
	struct buffer_head * tmp;                                               //168/ 
                                                                                //169/ 
	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)           //170/ 使用设备号dev和逻辑块号，找出缓冲头的哈希表结构中对应的项(指向一个缓冲头队列的第一项)，然后遍寻该缓冲头队列
		if (tmp->b_dev==dev && tmp->b_blocknr==block)                   //171/ 如果该缓冲头队列中存在参数指定的缓冲头结构
			return tmp;                                             //172/ 返回指向该缓冲头结构的指针
	return NULL;                                                            //173/ 
}                                                                               //174/ 
                                                                                //175/ 
/*                                                                              //176/ 
 * Why like this, I hear you say... The reason is race-conditions.              //177/ 
 * As we don't lock buffers (unless we are readint them, that is),              //178/ 
 * something might happen to it while we sleep (ie a read-error                 //179/ 
 * will force it bad). This shouldn't really happen currently, but              //180/ 
 * the code is ready.                                                           //181/ 
 */                                                                             //182/ 
struct buffer_head * get_hash_table(int dev, int block)                         //183/ [b;]利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号block指定的缓冲块，如果找到就增加其引用计数并返回指向其对应缓冲头结构的指针
{                                                                               //184/ 
	struct buffer_head * bh;                                                //185/ 
                                                                                //186/ 
	for (;;) {                                                              //187/ 
		if (!(bh=find_buffer(dev,block)))                               //188/ 利用HASH表查询设备号dev指定的设备的逻辑块号为block的缓冲块是否已在高速缓冲区中，如果在就返回指向其对应缓冲头结构的指针给bh
			return NULL;                                            //189/ 
		bh->b_count++;                                                  //190/ 将该缓冲块的引用计数加一
		wait_on_buffer(bh);                                             //191/ 如果bh指定的缓冲块已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
		if (bh->b_dev == dev && bh->b_blocknr == block)                 //192/ 因为没有上锁，所以经过了睡眠状态，有必要再次验证缓冲块的正确性
			return bh;                                              //193/ 正确，返回指向该缓冲头的指针
		bh->b_count--;                                                  //194/ 如果出错，则撤销对应的引用计数
	}                                                                       //195/ 
}                                                                               //196/ 
                                                                                //197/ 
/*                                                                              //198/ 
 * Ok, this is getblk, and it isn't very clear, again to hinder                 //199/ 
 * race-conditions. Most of the code is seldom used, (ie repeating),            //200/ 
 * so it should be much more efficient than it looks.                           //201/ 
 *                                                                              //202/ 
 * The algoritm is changed: hopefully better, and an elusive bug removed.       //203/ 
 */                                                                             //204/ 
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)                            //205/ 通过bh指向的缓冲头中的修改标志和锁定标志得出的该缓冲头结构的总权值(修改标志占的比重要比锁定标志大)
struct buffer_head * getblk(int dev,int block)                                  //206/ [b;]利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号block指定的缓冲块，如果找到就增加其引用计数并返回指向其对应缓冲头结构的指针；如果没找到就在高速缓冲的空闲数据块链表中找到一个空闲项，建立一个对应的新项(b_uptodate=0)，返回相应的缓冲头指针
{                                                                               //207/ 
	struct buffer_head * tmp, * bh;                                         //208/ 
                                                                                //209/ 
repeat:                                                                         //210/ 
	if (bh = get_hash_table(dev,block))                                     //211/ 利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号block指定的缓冲块，如果找到就增加其引用计数并返回指向其对应缓冲头结构的指针给bh
		return bh;                                                      //212/ 
	tmp = free_list;                                                        //213/ 使tmp指向空闲链表的第一项
	do {                                                                    //214/ 
		if (tmp->b_count)                                               //215/ 如果该项的引用计数不为0，表示该缓冲块正在被使用
			continue;                                               //216/ 则继续扫描下一项
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {                          //217/ 遍寻空闲链表，找出其中缓冲头结构的权值最小的一个缓冲头，将指向其的指针赋给bh
			bh = tmp;                                               //218/ 
			if (!BADNESS(tmp))                                      //219/ 如果某一项的权值为0(即该缓冲块既没有被修改也没有被锁定)，则直接将其赋给bh，并退出循环
				break;                                          //220/ 
		}                                                               //221/ 
/* and repeat until we find something good */                                   //222/ 
	} while ((tmp = tmp->b_next_free) != free_list);                        //223/ 遍寻空闲链表
	if (!bh) {                                                              //224/ 如果空闲链表中所有缓冲块都被使用
		sleep_on(&buffer_wait);                                         //225/ 则当前任务进入不可中断的睡眠等待，醒来后回到开头重新查找
		goto repeat;                                                    //226/ 
	}                                                                       //227/ 
	wait_on_buffer(bh);                                                     //228/ 如果找到一个未被使用的缓冲块，如果已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
	if (bh->b_count)                                                        //229/ 经过了睡眠，醒来后再次判断该缓冲块有没有被其他任务使用
		goto repeat;                                                    //230/ 如果被占用了，则回到开头重新查找
	while (bh->b_dirt) {                                                    //231/ 如果该缓冲区已被修改
		sync_dev(bh->b_dev);                                            //232/ 对设备号bh->b_dev指定的设备进行高速缓冲数据与设备上数据的同步操作
		wait_on_buffer(bh);                                             //233/ 如果bh指定的缓冲块已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
		if (bh->b_count)                                                //234/ 经过了睡眠，醒来后再次判断该缓冲块有没有被其他任务使用
			goto repeat;                                            //235/ 如果被占用了，则回到开头重新查找
	}                                                                       //236/ 
/* NOTE!! While we slept waiting for this block, somebody else might */         //237/ 
/* already have added "this" block to the cache. check it */                    //238/ 
	if (find_buffer(dev,block))                                             //239/ 利用HASH表查询设备号dev指定的设备的逻辑块号为block的缓冲块是否乘睡眠之际已被加入高速缓冲区中，如果在其中就回到开头重新查找
		goto repeat;                                                    //240/ 
/* OK, FINALLY we know that this buffer is the only one of it's kind, */        //241/ 
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */          //242/ 最终我们找到指定参数的唯一一块没有被使用(b_count=0)、未被上锁(b_lock=0)并且是干净(b_dirt=0)的缓冲块，让我们占领此缓冲块
	bh->b_count=1;                                                          //243/ 置引用计数为1
	bh->b_dirt=0;                                                           //244/ 复位修改标志
	bh->b_uptodate=0;                                                       //245/ 复位有效标志
	remove_from_queues(bh);                                                 //246/ 从HASH表中的一个队列和空闲链表队列中移走bh指定的缓冲头
	bh->b_dev=dev;                                                          //247/ 设置设备号，让缓冲块用于指定设备
	bh->b_blocknr=block;                                                    //248/ 设置逻辑块号，让缓冲块用于指定设备的指定块
	insert_into_queues(bh);                                                 //249/ 将bh指向的缓冲头插入空闲链表尾部，同时放入HASH队列中
	return bh;                                                              //250/ 返回该缓冲头指针
}                                                                               //251/ 
                                                                                //252/ 
void brelse(struct buffer_head * buf)                                           //253/ [b;]等待不为空的buf指定的缓冲块解锁，将buf指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
{                                                                               //254/ 
	if (!buf)                                                               //255/ 如果缓冲头指针无效
		return;                                                         //256/ 则返回
	wait_on_buffer(buf);                                                    //257/ 如果buf指定的缓冲块已上锁的话，则把当前任务置为不可中断的睡眠状态，等待buf指定的高速缓冲块解锁
	if (!(buf->b_count--))                                                  //258/ 将buf指定的缓冲块引用计数递减1
		panic("Trying to free free buffer");                            //259/ 
	wake_up(&buffer_wait);                                                  //260/ 唤醒buffer_wait指向的任务，buffer_wait是任务等待队列头指针
}                                                                               //261/ 
                                                                                //262/ 
/*                                                                              //263/ 
 * bread() reads a specified block and returns the buffer that contains         //264/ 
 * it. It returns NULL if the block was unreadable.                             //265/ 
 */                                                                             //266/ 
struct buffer_head * bread(int dev,int block)                                   //267/ [b;]从设备号dev指定的设备上读取逻辑块号block指定的数据块到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针
{                                                                               //268/ 
	struct buffer_head * bh;                                                //269/ 
                                                                                //270/ 
	if (!(bh=getblk(dev,block)))                                            //271/ 利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号block指定的缓冲块，如果找到就增加其引用计数并返回指向其对应缓冲头结构的指针赋给bh；如果没找到就在高速缓冲的空闲数据块链表中找到一个空闲项，建立一个对应的新项(b_uptodate=0)，返回相应的缓冲头指针赋给bh
		panic("bread: getblk returned NULL\n");                         //272/ 
	if (bh->b_uptodate)                                                     //273/ 判断bh指定缓冲块中数据是否有效
		return bh;                                                      //274/ 如果有效则直接返回使用
	ll_rw_block(READ,bh);                                                   //275/ 如果无效则判断bh指向的缓冲头结构对应的设备的主设备号和请求操作函数是否有效，如果有效则创建请求项并插入请求队列中
	wait_on_buffer(bh);                                                     //276/ 等待指定的数据被写入——如果bh指定的缓冲块已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
	if (bh->b_uptodate)                                                     //277/ 判断bh指定缓冲块中数据是否有效
		return bh;                                                      //278/ 如果有效则直接返回使用
	brelse(bh);                                                             //279/ 否则说明读设备操作失败，则等待不为空的buf指定的缓冲块解锁，将buf指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	return NULL;                                                            //280/ 
}                                                                               //281/ 
                                                                                //282/ 
#define COPYBLK(from,to) \                                                      //283/ [b;]从ds:from地址复制一块(1024字节)数据到es:to位置
__asm__("cld\n\t" \                                                             //284/ 清方向位，按递增方式复制
	"rep\n\t" \                                                             //285/ 
	"movsl\n\t" \                                                           //286/ 
	::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \                              //287/ BLOCK_SIZE = 1024
	:"cx","di","si")                                                        //288/ 
                                                                                //289/ 
/*                                                                              //290/ 
 * bread_page reads four buffers into memory at the desired address. It's       //291/ 
 * a function of its own, as there is some speed to be got by reading them      //292/ 
 * all at the same time, not waiting for one to be read, and then another       //293/ 
 * etc.                                                                         //294/ 
 */                                                                             //295/ 
void bread_page(unsigned long address,int dev,int b[4])                         //296/ [b;]将设备号dev和逻辑块号b[]对应的1-4个(不一定就是4个，因为有些可能无效)高速缓冲块数据(如果缓冲块中数据无效则产生读设备请求从设备上读取相应数据块内容到高速缓冲块中)顺序复制到主内存中的地址address处
{                                                                               //297/ 
	struct buffer_head * bh[4];                                             //298/ 
	int i;                                                                  //299/ 
                                                                                //300/ 
	for (i=0 ; i<4 ; i++)                                                   //301/ 
		if (b[i]) {                                                     //302/ 如果逻辑块号b[i]有效
			if (bh[i] = getblk(dev,b[i]))                           //303/ 利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号b[i]指定的缓冲块，如果找到就增加其引用计数并返回指向其对应缓冲头结构的指针；如果没找到就在高速缓冲的空闲数据块链表中找到一个空闲项，建立一个对应的新项(b_uptodate=0)，返回相应的缓冲头指针赋给bh[i]
				if (!bh[i]->b_uptodate)                         //304/ 如果bh[i]指定的缓冲块未更新
					ll_rw_block(READ,bh[i]);                //305/ 则判断bh[i]指向的缓冲头结构对应的设备的主设备号和请求操作函数是否有效，如果有效则创建请求项并插入请求队列中
		} else                                                          //306/ 否则
			bh[i] = NULL;                                           //307/ 将bh[i]置空
	for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)                             //308/ 将1-4个(不一定就是4个，因为有些可能无效)缓冲块上的内容顺序复制到address指定的地址处
		if (bh[i]) {                                                    //309/ 如果bh[i]不为空
			wait_on_buffer(bh[i]);                                  //310/ 如果bh[i]指定的缓冲块已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh[i]指定的高速缓冲块解锁，即等待数据被读入高速缓冲块中
			if (bh[i]->b_uptodate)                                  //311/ 如果bh[i]指定的缓冲块数据有效
				COPYBLK((unsigned long) bh[i]->b_data,address); //312/ 则从ds:bh[i]->b_data地址复制一块(1024字节)数据到es:address位置
			brelse(bh[i]);                                          //313/ 等待不为空的bh[i]指定的缓冲块解锁，将bh[i]指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
		}                                                               //314/ 
}                                                                               //315/ 
                                                                                //316/ 
/*                                                                              //317/ 
 * Ok, breada can be used as bread, but additionally to mark other              //318/ 
 * blocks for reading as well. End the argument list with a negative            //319/ 
 * number.                                                                      //320/ 
 */                                                                             //321/ 
struct buffer_head * breada(int dev,int first, ...)                             //322/ [b;]从设备号dev指定的设备读取指定的一些块到高速缓冲区，返回参数first对应的缓冲头指针用于立即使用，其他块只是预读进高速缓冲区，暂时不用
{                                                                               //323/ 
	va_list args;                                                           //324/ 
	struct buffer_head * bh, *tmp;                                          //325/ 
                                                                                //326/ 
	va_start(args,first);                                                   //327/ 使指针args指向传给函数的可变参数表...中的第一个参数
	if (!(bh=getblk(dev,first)))                                            //328/ 利用HASH表在高速缓冲区中寻找设备号dev和参数中逻辑块号first指定的缓冲块，如果找到就增加其引用计数并返回指向其对应缓冲头结构的指针；如果没找到就在高速缓冲的空闲数据块链表中找到一个空闲项，建立一个对应的新项(b_uptodate=0)，返回相应的缓冲头指针赋给bh
		panic("bread: getblk returned NULL\n");                         //329/ 
	if (!bh->b_uptodate)                                                    //330/ 如果bh指定的缓冲块未更新
		ll_rw_block(READ,bh);                                           //331/ 则判断bh指向的缓冲头结构对应的设备的主设备号和请求操作函数是否有效，如果有效则创建请求项并插入请求队列中
	while ((first=va_arg(args,int))>=0) {                                   //332/ 332-339行顺序对可变参数...中的逻辑块号进行类似上面的操作，但是因为只是读进高速缓冲区并不是马上被使用，所以需要将其引用计数递减释放掉(因为在getblk()中增加了其引用计数值)
		tmp=getblk(dev,first);                                          //333/ 
		if (tmp) {                                                      //334/ 
			if (!tmp->b_uptodate)                                   //335/ 
				ll_rw_block(READA,bh);                          //336/ 存在一个bug——"bh应被替换为tmp"
			tmp->b_count--;                                         //337/ 
		}                                                               //338/ 
	}                                                                       //339/ 
	va_end(args);                                                           //340/ 
	wait_on_buffer(bh);                                                     //341/ 如果bh指定的缓冲块(与参数first指定的逻辑块号对应的缓冲块)已上锁的话，则把当前任务置为不可中断的睡眠状态，等待bh指定的高速缓冲块解锁
	if (bh->b_uptodate)                                                     //342/ 如果bh指定的缓冲块(与参数first指定的逻辑块号对应的缓冲块)已更新
		return bh;                                                      //343/ 则返回该缓冲头指针
	brelse(bh);                                                             //344/ 否则等待不为空的bh指定的缓冲块(与参数first指定的逻辑块号对应的缓冲块)解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	return (NULL);                                                          //345/ 
}                                                                               //346/ 
                                                                                //347/ 
void buffer_init(long buffer_end)                                               //348/ [b;]从高速缓冲区开始位置start_buffer到末端buffer_end处分别同时设置(初始化)缓冲头结构和对应的缓冲块，直到缓冲区中所有内存被分配完毕
{                                                                               //349/ 
	struct buffer_head * h = start_buffer;                                  //350/ start_buffer指向内核代码末端，即高速缓冲区的内存始段指针，将其赋给h
	void * b;                                                               //351/ 
	int i;                                                                  //352/ 
                                                                                //353/ 
	if (buffer_end == 1<<20)                                                //354/ 354-357行根据内存容量将高速缓冲区末端的内存地址值赋给b(因为640KB-1MB分配给了显存和ROM-BIOS)
		b = (void *) (640*1024);                                        //355/ 
	else                                                                    //356/ 
		b = (void *) buffer_end;                                        //357/ 
	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {                       //358/ 保证剩余的空间足够分配出一个缓冲头结构和对应的一个缓冲块
		h->b_dev = 0;                                                   //359/ 
		h->b_dirt = 0;                                                  //360/ 
		h->b_count = 0;                                                 //361/ 
		h->b_lock = 0;                                                  //362/ 
		h->b_uptodate = 0;                                              //363/ 
		h->b_wait = NULL;                                               //364/ 
		h->b_next = NULL;                                               //365/ 
		h->b_prev = NULL;                                               //366/ 
		h->b_data = (char *) b;                                         //367/ 
		h->b_prev_free = h-1;                                           //368/ 
		h->b_next_free = h+1;                                           //369/ 
		h++;                                                            //370/ 使h指向下一新缓冲头位置
		NR_BUFFERS++;                                                   //371/ 缓冲块的数量累加
		if (b == (void *) 0x100000)                                     //372/ 若b递减到等于1MB，则跳过384KB(因为640KB-1MB分配给了显存和ROM-BIOS)
			b = (void *) 0xA0000;                                   //373/ 
	}                                                                       //374/ 
	h--;                                                                    //375/ 让h指向最后一个有效缓冲头
	free_list = start_buffer;                                               //376/ 让空闲链表表指向头一个缓冲块
	free_list->b_prev_free = h;                                             //377/ 链表头的b_prev_free指向前一项(即最后一项)
	h->b_next_free = free_list;                                             //378/ h的下一项指针指向第一项，形成一个环链
	for (i=0;i<NR_HASH;i++)                                                 //379/ 初始化HASH表，置表中所有(NR_HASH = 307个)指针为NULL
		hash_table[i]=NULL;                                             //380/ 
}	                                                                        //381/ 

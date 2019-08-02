/*                                                                          //  1/ 
 *  linux/fs/inode.c                                                        //  2/ 
 *                                                                          //  3/ 
 *  (C) 1991  Linus Torvalds                                                //  4/ 
 */                                                                         //  5/ 
                                                                            //  6/ 
#include <string.h>                                                         //  7/ 
#include <sys/stat.h>                                                       //  8/ 
                                                                            //  9/ 
#include <linux/sched.h>                                                    // 10/ 
#include <linux/kernel.h>                                                   // 11/ 
#include <linux/mm.h>                                                       // 12/ 
#include <asm/system.h>                                                     // 13/ 
                                                                            // 14/ 
struct m_inode inode_table[NR_INODE]={{0,},};                               // 15/ 定义内存中i节点结构表(数组)，总共NR_INODE=32个i节点
                                                                            // 16/ 
static void read_inode(struct m_inode * inode);                             // 17/ 
static void write_inode(struct m_inode * inode);                            // 18/ 
                                                                            // 19/ 
static inline void wait_on_inode(struct m_inode * inode)                    // 20/ [b;]判断参数inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
{                                                                           // 21/ 
	cli();                                                              // 22/ 
	while (inode->i_lock)                                               // 23/ 
		sleep_on(&inode->i_wait);                                   // 24/ 把当前任务置为不可中断的睡眠状态，让当前任务的tmp指向inode->i_wait指向的旧睡眠队列头，而让inode->i_wait指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up唤醒了inode->i_wait指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
	sti();                                                              // 25/ 
}                                                                           // 26/ 
                                                                            // 27/ 
static inline void lock_inode(struct m_inode * inode)                       // 28/ [b;]锁定inode指定的内存i节点，若已被锁定，则当前任务先进入不可中断的睡眠状态，醒来后再锁定
{                                                                           // 29/ 
	cli();                                                              // 30/ 
	while (inode->i_lock)                                               // 31/ 
		sleep_on(&inode->i_wait);                                   // 32/ 
	inode->i_lock=1;                                                    // 33/ 
	sti();                                                              // 34/ 
}                                                                           // 35/ 
                                                                            // 36/ 
static inline void unlock_inode(struct m_inode * inode)                     // 37/ [b;]解锁inode指定的内存i节点，并唤醒inode->i_wait指向的任务
{                                                                           // 38/ 
	inode->i_lock=0;                                                    // 39/ 
	wake_up(&inode->i_wait);                                            // 40/ 唤醒inode->i_wait指向的任务，inode->i_wait是任务等待队列头指针
}                                                                           // 41/ 
                                                                            // 42/ 
void invalidate_inodes(int dev)                                             // 43/ [b;]释放设备号dev指定的设备在内存i节点表中的所有i节点
{                                                                           // 44/ 
	int i;                                                              // 45/ 
	struct m_inode * inode;                                             // 46/ 
                                                                            // 47/ 
	inode = 0+inode_table;                                              // 48/ 
	for(i=0 ; i<NR_INODE ; i++,inode++) {                               // 49/ 遍寻内存中i节点表中的所有(NR_INODE = 32)i节点
		wait_on_inode(inode);                                       // 50/ 判断参数inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
		if (inode->i_dev == dev) {                                  // 51/ 如果该内存i节点是设备号dev指定的设备的i节点
			if (inode->i_count)                                 // 52/ 如果该内存i节点的引用计数不为0，即还被使用着
				printk("inode in use on removed disk\n\r"); // 53/ 则打印警告信息
			inode->i_dev = inode->i_dirt = 0;                   // 54/ 释放该内存i节点(置节点所在的设备号为0，置修改标志为0)
		}                                                           // 55/ 
	}                                                                   // 56/ 
}                                                                           // 57/ 
                                                                            // 58/ 
void sync_inodes(void)                                                      // 59/ [b;]把内存i节点表中的所有i节点与设备上的i节点作同步操作
{                                                                           // 60/ 
	int i;                                                              // 61/ 
	struct m_inode * inode;                                             // 62/ 
                                                                            // 63/ 
	inode = 0+inode_table;                                              // 64/ 使inode指向内存中i节点结构表的开头
	for(i=0 ; i<NR_INODE ; i++,inode++) {                               // 65/ 遍寻内存中i节点表的所有NR_INODE个i节点
		wait_on_inode(inode);                                       // 66/ 判断参数inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
		if (inode->i_dirt && !inode->i_pipe)                        // 67/ 解锁后如果i节点已修改且不是管道节点
			write_inode(inode);                                 // 68/ 则将参数inode指定的内存i节点信息中的一部分(块设备i节点信息)写入缓冲区中相应的缓冲块中，待缓冲区刷新时会写入盘中
	}                                                                   // 69/ 
}                                                                           // 70/ 
                                                                            // 71/ 
static int _bmap(struct m_inode * inode,int block,int create)               // 72/ [b;]
{                                                                           // 73/ 
	struct buffer_head * bh;                                            // 74/ 
	int i;                                                              // 75/ 
                                                                            // 76/ 
	if (block<0)                                                        // 77/ 
		panic("_bmap: block<0");                                    // 78/ 
	if (block >= 7+512+512*512)                                         // 79/ 
		panic("_bmap: block>big");                                  // 80/ 
	if (block<7) {                                                      // 81/ 
		if (create && !inode->i_zone[block])                        // 82/ 
			if (inode->i_zone[block]=new_block(inode->i_dev)) { // 83/ 
				inode->i_ctime=CURRENT_TIME;                // 84/ 
				inode->i_dirt=1;                            // 85/ 
			}                                                   // 86/ 
		return inode->i_zone[block];                                // 87/ 
	}                                                                   // 88/ 
	block -= 7;                                                         // 89/ 
	if (block<512) {                                                    // 90/ 
		if (create && !inode->i_zone[7])                            // 91/ 
			if (inode->i_zone[7]=new_block(inode->i_dev)) {     // 92/ 
				inode->i_dirt=1;                            // 93/ 
				inode->i_ctime=CURRENT_TIME;                // 94/ 
			}                                                   // 95/ 
		if (!inode->i_zone[7])                                      // 96/ 
			return 0;                                           // 97/ 
		if (!(bh = bread(inode->i_dev,inode->i_zone[7])))           // 98/ 
			return 0;                                           // 99/ 
		i = ((unsigned short *) (bh->b_data))[block];               //100/ 
		if (create && !i)                                           //101/ 
			if (i=new_block(inode->i_dev)) {                    //102/ 
				((unsigned short *) (bh->b_data))[block]=i; //103/ 
				bh->b_dirt=1;                               //104/ 
			}                                                   //105/ 
		brelse(bh);                                                 //106/ 
		return i;                                                   //107/ 
	}                                                                   //108/ 
	block -= 512;                                                       //109/ 
	if (create && !inode->i_zone[8])                                    //110/ 
		if (inode->i_zone[8]=new_block(inode->i_dev)) {             //111/ 
			inode->i_dirt=1;                                    //112/ 
			inode->i_ctime=CURRENT_TIME;                        //113/ 
		}                                                           //114/ 
	if (!inode->i_zone[8])                                              //115/ 
		return 0;                                                   //116/ 
	if (!(bh=bread(inode->i_dev,inode->i_zone[8])))                     //117/ 
		return 0;                                                   //118/ 
	i = ((unsigned short *)bh->b_data)[block>>9];                       //119/ 
	if (create && !i)                                                   //120/ 
		if (i=new_block(inode->i_dev)) {                            //121/ 
			((unsigned short *) (bh->b_data))[block>>9]=i;      //122/ 
			bh->b_dirt=1;                                       //123/ 
		}                                                           //124/ 
	brelse(bh);                                                         //125/ 
	if (!i)                                                             //126/ 
		return 0;                                                   //127/ 
	if (!(bh=bread(inode->i_dev,i)))                                    //128/ 
		return 0;                                                   //129/ 
	i = ((unsigned short *)bh->b_data)[block&511];                      //130/ 
	if (create && !i)                                                   //131/ 
		if (i=new_block(inode->i_dev)) {                            //132/ 
			((unsigned short *) (bh->b_data))[block&511]=i;     //133/ 
			bh->b_dirt=1;                                       //134/ 
		}                                                           //135/ 
	brelse(bh);                                                         //136/ 
	return i;                                                           //137/ 
}                                                                           //138/ 
                                                                            //139/ 
int bmap(struct m_inode * inode,int block)                                  //140/ [b;]
{                                                                           //141/ 
	return _bmap(inode,block,0);                                        //142/ 
}                                                                           //143/ 
                                                                            //144/ 
int create_block(struct m_inode * inode, int block)                         //145/ [b;]
{                                                                           //146/ 
	return _bmap(inode,block,1);                                        //147/ 
}                                                                           //148/ 
		                                                            //149/ 
void iput(struct m_inode * inode)                                           //150/ [b;]将参数inode指向的内存中i节点释放
{                                                                           //151/ 
	if (!inode)                                                         //152/ 判断inode的有效性
		return;                                                     //153/ 
	wait_on_inode(inode);                                               //154/ 判断inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
	if (!inode->i_count)                                                //155/ 如果inode指定的内存i节点的引用计数为0，说明内核中其他代码有问题
		panic("iput: trying to free free inode");                   //156/ 直接死机
	if (inode->i_pipe) {                                                //157/ 如果inode指向的内存中i节点是管道i节点
		wake_up(&inode->i_wait);                                    //158/ 唤醒inode->i_wait指向的任务，inode->i_wait是任务等待队列头指针
		if (--inode->i_count)                                       //159/ 引用次数减1，如果还有引用
			return;                                             //160/ 则返回
		free_page(inode->i_size);                                   //161/ 释放管道缓冲区——释放物理地址inode->i_size指向的位置所在处的1页面内存
		inode->i_count=0;                                           //162/ 复位该内存中i节点的引用计数值
		inode->i_dirt=0;                                            //163/ 复位该内存中i节点的已修改标志
		inode->i_pipe=0;                                            //164/ 复位该内存中i节点的管道标志
		return;                                                     //165/ 返回
	}                                                                   //166/ 
	if (!inode->i_dev) {                                                //167/ 如果inode指向的内存中i节点对应的设备的设备号为0
		inode->i_count--;                                           //168/ 则将该内存中i节点的引用计数值减1
		return;                                                     //169/ 返回
	}                                                                   //170/ 
	if (S_ISBLK(inode->i_mode)) {                                       //171/ 如果inode指向的内存中i节点是块设备文件的i节点
		sync_dev(inode->i_zone[0]);                                 //172/ 对设备号inode->i_zone[0]指定的设备进行高速缓冲中数据与设备上数据的同步操作
		wait_on_inode(inode);                                       //173/ 判断inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
	}                                                                   //174/ 
repeat:                                                                     //175/ 
	if (inode->i_count>1) {                                             //176/ 如果inode指向的内存中i节点的引用计数大于1
		inode->i_count--;                                           //177/ 则计数递减1
		return;                                                     //178/ 返回
	}                                                                   //179/ 
	if (!inode->i_nlinks) {                                             //180/ 如果inode指向的内存中i节点的链接数为0，说明文件被删除
		truncate(inode);                                            //181/ 将inode指向的内存中i节点对应的文件所占用的所有逻辑块(包括所有的直接块、一次和二次间接块)都释放，将该文件的长度置零，置位该i节点的已修改标志，将该i节点的文件修改时间和i节点改变时间设置为当前时间
		free_inode(inode);                                          //182/ 如果inode指定的内存中i节点未被使用，则将该内存中i节点释放(内容清零)，如果已使用，则复位inode指定的内存中i节点对应的i节点位图中的偏移位，并置该i节点位图所在的高速缓冲块的已修改标志，然后再将该内存中i节点释放(内容清零)
		return;                                                     //183/ 返回
	}                                                                   //184/ 
	if (inode->i_dirt) {                                                //185/ 如果inode指定的内存中i节点已被修改
		write_inode(inode);	/* we can sleep - so do again */    //186/ 则将参数inode指定的内存i节点信息中的一部分(块设备i节点信息)写入缓冲区中相应的缓冲块中，待缓冲区刷新时会写入盘中
		wait_on_inode(inode);                                       //187/ 判断参数inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
		goto repeat;                                                //188/ 因为睡眠过，所以其他进程可能又修改了该i节点，所以回到repeat再次重复
	}                                                                   //189/ 
	inode->i_count--;                                                   //190/ 将inode指向的内存中i节点的引用计数递减1，此时应该等于0，表示已释放
	return;                                                             //191/ 返回
}                                                                           //192/ 
                                                                            //193/ 
struct m_inode * get_empty_inode(void)                                      //194/ [b;]从内存中i节点表中获取一个空闲内存中i节点，将该内存中i节点内容清0，引用计数置1，返回指向该内存中i节点指针
{                                                                           //195/ 
	struct m_inode * inode;                                             //196/ 
	static struct m_inode * last_inode = inode_table;                   //197/ 使last_inode指向内存中i节点表的第1项
	int i;                                                              //198/ 
                                                                            //199/ 
	do {                                                                //200/ 
		inode = NULL;                                               //201/ 
		for (i = NR_INODE; i ; i--) {                               //202/ 202-210行遍寻内存中i节点表的NR_INODE=32个内存中i节点
			if (++last_inode >= inode_table + NR_INODE)         //203/ 如果last_inode指到了内存中i节点表的末尾
				last_inode = inode_table;                   //204/ 则将last_inode调整到内存中i节点表的第1项
			if (!last_inode->i_count) {                         //205/ 如果last_inode指向内存中i节点空闲，即引用计数为0
				inode = last_inode;                         //206/ 则将inode指向该空闲内存中i节点
				if (!inode->i_dirt && !inode->i_lock)       //207/ 如果该空闲内存中i节点的已修改标志和锁定标志都为0
					break;                              //208/ 则选定此空闲内存中i节点，并退出for循环
			}                                                   //209/ 
		}                                                           //210/ 
		if (!inode) {                                               //211/ 如果在内存中i节点表中未找到合适的内存中i节点
			for (i=0 ; i<NR_INODE ; i++)                        //212/ 则打印相关的调试信息，并死机
				printk("%04x: %6d\t",inode_table[i].i_dev,  //213/ 
					inode_table[i].i_num);              //214/ 
			panic("No free inodes in mem");                     //215/ 
		}                                                           //216/ 
		wait_on_inode(inode);                                       //217/ 判断inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
		while (inode->i_dirt) {                                     //218/ 因为要睡眠，所以循环判断inode指定的内存i节点是否已被修改
			write_inode(inode);                                 //219/ 将inode指定的内存i节点信息中的一部分(块设备i节点信息)写入缓冲区中相应的缓冲块中，待缓冲区刷新时会写入盘中
			wait_on_inode(inode);                               //220/ 判断inode指定的内存i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
		}                                                           //221/ 
	} while (inode->i_count);                                           //222/ 如果睡眠醒来后，该内存中i节点又被其他占用，即引用计数不为0，则重新再去寻找合适的内存i节点
	memset(inode,0,sizeof(*inode));                                     //223/ 用数字0填满inode指向的内存中i节点的内容
	inode->i_count = 1;                                                 //224/ 置该内存中i节点的引用计数为1
	return inode;                                                       //225/ 返回指向该内存中i节点指针
}                                                                           //226/ 
                                                                            //227/ 
struct m_inode * get_pipe_inode(void)                                       //228/ [b;]
{                                                                           //229/ 
	struct m_inode * inode;                                             //230/ 
                                                                            //231/ 
	if (!(inode = get_empty_inode()))                                   //232/ 
		return NULL;                                                //233/ 
	if (!(inode->i_size=get_free_page())) {                             //234/ 
		inode->i_count = 0;                                         //235/ 
		return NULL;                                                //236/ 
	}                                                                   //237/ 
	inode->i_count = 2;	/* sum of readers/writers */                //238/ 
	PIPE_HEAD(*inode) = PIPE_TAIL(*inode) = 0;                          //239/ 
	inode->i_pipe = 1;                                                  //240/ 
	return inode;                                                       //241/ 
}                                                                           //242/ 
                                                                            //243/ 
struct m_inode * iget(int dev,int nr)                                       //244/ [b;]从设备号dev指定的设备上读取i节点号nr指定的i节点到内存中i节点表中，并返回指向该内存中i节点的指针
{                                                                           //245/ 
	struct m_inode * inode, * empty;                                    //246/ 
                                                                            //247/ 
	if (!dev)                                                           //248/ 判断设备号是否有效
		panic("iget with dev==0");                                  //249/ 
	empty = get_empty_inode();                                          //250/ 临时申请一个空闲内存中i节点——从内存中i节点表中获取一个空闲内存中i节点，将该内存中i节点内容清0，引用计数置1，返回指向该内存中i节点指针赋给empty
	inode = inode_table;                                                //251/ 将inode指向内存中i节点表的第1项
	while (inode < NR_INODE+inode_table) {                              //252/ 遍寻内存中i节点表中的NR_INODE=32个内存中i节点
		if (inode->i_dev != dev || inode->i_num != nr) {            //253/ 253-256行用于找出设备号dev指定的设备上i节点号nr指定的i节点对应的内存中i节点指针赋给inode
			inode++;                                            //254/ 
			continue;                                           //255/ 
		}                                                           //256/ 
		wait_on_inode(inode);                                       //257/ 判断inode指定的内存中i节点是否被锁定，若已被锁定，则将当前任务置为不可中断的睡眠等待其解锁
		if (inode->i_dev != dev || inode->i_num != nr) {            //258/ 解锁之后再进行一次判断，如果不符合条件就回到while循环开头重新再在内存i节点表中查找
			inode = inode_table;                                //259/ 
			continue;                                           //260/ 
		}                                                           //261/ 
		inode->i_count++;                                           //262/ 将inode指定的内存中i节点的引用计数增1
		if (inode->i_mount) {                                       //263/ 如果inode指定的内存中i节点上挂载了其他的文件系统
			int i;                                              //264/ 
                                                                            //265/ 
			for (i = 0 ; i<NR_SUPER ; i++)                      //266/ 266-268行用于在内存中超级块表中搜索挂载在此内存中i节点的文件系统对应的内存中超级块
				if (super_block[i].s_imount==inode)         //267/ 
					break;                              //268/ 
			if (i >= NR_SUPER) {                                //269/ 如果没找到对应的内存中超级块
				printk("Mounted inode hasn't got sb\n");    //270/ 则打印出错信息
				if (empty)                                  //271/ 
					iput(empty);                        //272/ 将empty指向的内存中i节点释放
				return inode;                               //273/ 返回找到的i节点指针
			}                                                   //274/ 
			iput(inode);                                        //275/ 将inode指向的内存中i节点释放
			dev = super_block[i].s_dev;                         //276/ 从安装在inode指向的内存中i节点上的文件系统超级块中取设备号赋给dev
			nr = ROOT_INO;                                      //277/ 将节点号1赋给nr
			inode = inode_table;                                //278/ 将inode指向内存中i节点表的第1项，再重新查找
			continue;                                           //279/ 
		}                                                           //280/ 
		if (empty)                                                  //281/ 
			iput(empty);                                        //282/ 将empty指向的内存中i节点释放
		return inode;                                               //283/ 返回找到的i节点指针
	}                                                                   //284/ 
	if (!empty)                                                         //285/ 如果临时申请一个空闲内存中i节点无效
		return (NULL);                                              //286/ 返回空指针
	inode=empty;                                                        //287/ 287-289行用于在临时申请一个空闲内存中i节点中建立参数指定的i节点
	inode->i_dev = dev;                                                 //288/ 
	inode->i_num = nr;                                                  //289/ 
	read_inode(inode);                                                  //290/ 从设备上读取(inode指定的内存中i节点当中的块设备上i节点)部分的内容到inode指定的内存中i节点中
	return inode;                                                       //291/ 返回建立的i节点指针
}                                                                           //292/ 
                                                                            //293/ 
static void read_inode(struct m_inode * inode)                              //294/ [b;]从设备上读取(inode指定的内存中i节点当中的块设备上i节点)部分的内容到inode指定的内存中i节点中
{                                                                           //295/ 
	struct super_block * sb;                                            //296/ 
	struct buffer_head * bh;                                            //297/ 
	int block;                                                          //298/ 
                                                                            //299/ 
	lock_inode(inode);                                                  //300/ 锁定inode指定的内存i节点，若已被锁定，则当前任务先进入不可中断的睡眠状态，醒来后再锁定
	if (!(sb=get_super(inode->i_dev)))                                  //301/ 取设备号dev指定的设备对应的内存中超级块结构，返回指向该内存中超级块的指针赋给sb，如果sb为空指针
		panic("trying to read inode without dev");                  //302/ 则直接死机
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +                 //303/ 计算出inode指定的内存中i节点所在的设备逻辑块号赋给block
		(inode->i_num-1)/INODES_PER_BLOCK;                          //304/ 
	if (!(bh=bread(inode->i_dev,block)))                                //305/ 从设备号(inode->i_dev)指定的设备上读取逻辑块号block指定的数据块(即inode指定的内存中i节点所在的设备逻辑块)到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针赋给bh，如果bh为空指针
		panic("unable to read i-node block");                       //306/ 则直接死机
	*(struct d_inode *)inode =                                          //307/ 将从设备上读到高速缓冲块中的(inode指定的内存中i节点当中的块设备上i节点)部分的内容复制到inode指定的内存中i节点中
		((struct d_inode *)bh->b_data)                              //308/ 
			[(inode->i_num-1)%INODES_PER_BLOCK];                //309/ 
	brelse(bh);                                                         //310/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	unlock_inode(inode);                                                //311/ 解锁inode指定的内存i节点，并唤醒inode->i_wait指向的任务
}                                                                           //312/ 
                                                                            //313/ 
static void write_inode(struct m_inode * inode)                             //314/ [b;]将参数inode指定的内存i节点信息中的一部分(块设备i节点信息)写入缓冲区中相应的缓冲块中，待缓冲区刷新时会写入盘中
{                                                                           //315/ 
	struct super_block * sb;                                            //316/ 
	struct buffer_head * bh;                                            //317/ 
	int block;                                                          //318/ 
                                                                            //319/ 
	lock_inode(inode);                                                  //320/ 锁定inode指定的内存i节点，若已被锁定，则当前任务先进入不可中断的睡眠状态，醒来后再锁定
	if (!inode->i_dirt || !inode->i_dev) {                              //321/ 如果该i节点被修改或该i节点的设备号等于零
		unlock_inode(inode);                                        //322/ 则解锁该i节点，并退出
		return;                                                     //323/ 
	}                                                                   //324/ 
	if (!(sb=get_super(inode->i_dev)))                                  //325/ 取设备号inode->i_dev指定的设备对应的内存中超级块结构，将指向该内存中超级块的指针赋给sb
		panic("trying to write inode without device");              //326/ 如果没取到，直接死机
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +                 //327/ 计算参数inode指定的内存i节点所在的设备的逻辑块号赋给block = (启动块 + 超级块) + i节点位图占用的块 + 逻辑块位图占用的块 + (i节点号-1)/每块含有的i节点数
		(inode->i_num-1)/INODES_PER_BLOCK;                          //328/ 
	if (!(bh=bread(inode->i_dev,block)))                                //329/ 从设备号inode->i_dev指定的设备上读取逻辑块号block指定的数据块到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针赋给bh
		panic("unable to read i-node block");                       //330/ 
	((struct d_inode *)bh->b_data)                                      //331/ 
		[(inode->i_num-1)%INODES_PER_BLOCK] =                       //332/ 
			*(struct d_inode *)inode;                           //333/ 将参数inode指定的内存中i节点信息中的一部分(块设备i节点信息)取出，复制到bh指定的缓冲块中对应该i节点的位置处
	bh->b_dirt=1;                                                       //334/ 置缓冲块已修改标志
	inode->i_dirt=0;                                                    //335/ 复位i节点修改标志，i节点已经与缓冲区中的一致，待缓冲区刷新时会写入盘中
	brelse(bh);                                                         //336/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	unlock_inode(inode);                                                //337/ 解锁inode指定的内存i节点，并唤醒inode->i_wait指向的任务
}                                                                           //338/ 

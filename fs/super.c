/*                                                                              //  1/ 
 *  linux/fs/super.c                                                            //  2/ 
 *                                                                              //  3/ 
 *  (C) 1991  Linus Torvalds                                                    //  4/ 
 */                                                                             //  5/ 
                                                                                //  6/ 
/*                                                                              //  7/ 
 * super.c contains code to handle the super-block tables.                      //  8/ 
 */                                                                             //  9/ 
#include <linux/config.h>                                                       // 10/ 
#include <linux/sched.h>                                                        // 11/ 
#include <linux/kernel.h>                                                       // 12/ 
#include <asm/system.h>                                                         // 13/ 
                                                                                // 14/ 
#include <errno.h>                                                              // 15/ 
#include <sys/stat.h>                                                           // 16/ 
                                                                                // 17/ 
int sync_dev(int dev);                                                          // 18/ 
void wait_for_keypress(void);                                                   // 19/ 
                                                                                // 20/ 
/* set_bit uses setb, as gas doesn't recognize setc */                          // 21/ 
#define set_bit(bitnr,addr) ({ \                                                // 22/ 将地址addr指向的数据中偏移bitnr的那一位的位值(0或1)返回
register int __res __asm__("ax"); \                                             // 23/ 
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \ // 24/ 
__res; })                                                                       // 25/ 
                                                                                // 26/ 
struct super_block super_block[NR_SUPER];                                       // 27/ 定义内存中超级块结构表(数组)(NR_SUPER = 8)
/* this is initialized in init/main.c */                                        // 28/ 
int ROOT_DEV = 0;                                                               // 29/ 根文件系统所在设备的设备号
                                                                                // 30/ 
static void lock_super(struct super_block * sb)                                 // 31/ [b;]锁定sb指向的内存中超级块，若已被锁定，则当前任务先进入不可中断的睡眠状态，醒来后再锁定
{                                                                               // 32/ 
	cli();                                                                  // 33/ 
	while (sb->s_lock)                                                      // 34/ 
		sleep_on(&(sb->s_wait));                                        // 35/ 
	sb->s_lock = 1;                                                         // 36/ 
	sti();                                                                  // 37/ 
}                                                                               // 38/ 
                                                                                // 39/ 
static void free_super(struct super_block * sb)                                 // 40/ [b;]解锁sb指向的内存中超级块，并唤醒sb->s_wait指向的任务
{                                                                               // 41/ 
	cli();                                                                  // 42/ 
	sb->s_lock = 0;                                                         // 43/ 
	wake_up(&(sb->s_wait));                                                 // 44/ 
	sti();                                                                  // 45/ 
}                                                                               // 46/ 
                                                                                // 47/ 
static void wait_on_super(struct super_block * sb)                              // 48/ [b;]判断sb指向的超级块是否被锁，如果被上锁，则当前任务进入不可中断的睡眠，等待sb指向的超级块解锁
{                                                                               // 49/ 
	cli();                                                                  // 50/ 
	while (sb->s_lock)                                                      // 51/ 
		sleep_on(&(sb->s_wait));                                        // 52/ 
	sti();                                                                  // 53/ 
}                                                                               // 54/ 
                                                                                // 55/ 
struct super_block * get_super(int dev)                                         // 56/ [b;]取设备号dev指定的设备对应的内存中超级块结构，返回指向该内存中超级块的指针
{                                                                               // 57/ 
	struct super_block * s;                                                 // 58/ 
                                                                                // 59/ 
	if (!dev)                                                               // 60/ 如果设备号为0
		return NULL;                                                    // 61/ 直接返回空
	s = 0+super_block;                                                      // 62/ 让s指向内存超级块数组的起始处
	while (s < NR_SUPER+super_block)                                        // 63/ 遍寻NR_SUPER=8个内存中超级块结构
		if (s->s_dev == dev) {                                          // 64/ 如果s指向的内存中超级块就是参数dev指定的设备的超级块
			wait_on_super(s);                                       // 65/ 判断s指向的内存中超级块是否被锁，如果被上锁，则当前任务进入不可中断的睡眠，等待s指向的内存中超级块解锁
			if (s->s_dev == dev)                                    // 66/ 再次判断s指向的内存中超级块是否就是参数dev指定的设备的超级块 [r;]此判断似乎多余
				return s;                                       // 67/ 如果是，则返回指向该内存中超级块的指针s
			s = 0+super_block;                                      // 68/ 否则就将s指回内存中超级块数组的起始处，重新再搜索一遍
		} else                                                          // 69/ 如果不是
			s++;                                                    // 70/ 则接着检查下一项
	return NULL;                                                            // 71/ 
}                                                                               // 72/ 
                                                                                // 73/ 
void put_super(int dev)                                                         // 74/ [b;]释放设备号dev指定的设备对应的内存中超级块结构(置其中的s_dev为0)，并释放该设备i节点位图和逻辑块位图所占用的高速缓冲块
{                                                                               // 75/ 
	struct super_block * sb;                                                // 76/ 
	struct m_inode * inode;                                                 // 77/ 
	int i;                                                                  // 78/ 
                                                                                // 79/ 
	if (dev == ROOT_DEV) {                                                  // 80/ 如果设备号dev和根文件系统所在设备的设备号相同
		printk("root diskette changed: prepare for armageddon\n\r");    // 81/ 则不允许释放，直接退出
		return;                                                         // 82/ 
	}                                                                       // 83/ 
	if (!(sb = get_super(dev)))                                             // 84/ 取设备号dev指定的设备对应的内存中超级块结构，返回指向该内存中超级块的指针赋给sb
		return;                                                         // 85/ 
	if (sb->s_imount) {                                                     // 86/ 如果该内存中超级块对应的某个i节点挂载有文件系统
		printk("Mounted disk changed - tssk, tssk\n\r");                // 87/ 则不允许释放，直接退出
		return;                                                         // 88/ 
	}                                                                       // 89/ 
	lock_super(sb);                                                         // 90/ 锁定sb指向的内存中超级块，若已被锁定，则当前任务先进入不可中断的睡眠状态，醒来后再锁定
	sb->s_dev = 0;                                                          // 91/ 置sb指向的内存中超级块空闲，即释放该超级块
	for(i=0;i<I_MAP_SLOTS;i++)                                              // 92/ 遍寻设备号dev指定的设备的内存中超级块结构中的i节点位图在高速缓冲块的指针数组(共I_MAP_SLOTS=8个元素)
		brelse(sb->s_imap[i]);                                          // 93/ 等待不为空的sb->s_imap[i]指定的缓冲块解锁，将sb->s_imap[i]指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	for(i=0;i<Z_MAP_SLOTS;i++)                                              // 94/ 遍寻设备号dev指定的设备的内存中超级块结构中的逻辑块位图在高速缓冲块的指针数组(共Z_MAP_SLOTS=8个元素)
		brelse(sb->s_zmap[i]);                                          // 95/ 等待不为空的sb->s_zmap[i]指定的缓冲块解锁，将sb->s_zmap[i]指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	free_super(sb);                                                         // 96/ 解锁sb指向的内存中超级块，并唤醒sb->s_wait指向的任务
	return;                                                                 // 97/ 
}                                                                               // 98/ 
                                                                                // 99/ 
static struct super_block * read_super(int dev)                                 //100/ [b;]如果设备号dev指定的设备上的文件系统超级块已经在超级块结构表中，则直接返回指向该超级块项的指针，否则就从该设备上读取超级块到缓冲块中，并复制到超级块结构表中，并返回指向该内存中超级块结构的指针
{                                                                               //101/ 
	struct super_block * s;                                                 //102/ 
	struct buffer_head * bh;                                                //103/ 
	int i,block;                                                            //104/ 
                                                                                //105/ 
	if (!dev)                                                               //106/ 如果设备号为0
		return NULL;                                                    //107/ 直接返回NULL
	check_disk_change(dev);                                                 //108/ 检查设备号dev指定的设备是否更换，如果已更换则释放该设备对应的内存中超级块结构、释放该设备在内存i节点表中的所有i节点，对该设备在高速缓冲中对应的缓冲块复位其更新(有效)标志和已修改标志，使该设备在高速缓冲区中的数据无效
	if (s = get_super(dev))                                                 //109/ 取设备号dev指定的设备对应的内存中超级块结构，返回指向该内存中超级块的指针赋给s
		return s;                                                       //110/ 如果该指针不为空，则返回s
	for (s = 0+super_block ;; s++) {                                        //111/ 111-116行遍寻NR_SUPER=8个内存中超级块结构，找出一个未使用的空项，将指向该项的指针赋给s
		if (s >= NR_SUPER+super_block)                                  //112/ 
			return NULL;                                            //113/ 
		if (!s->s_dev)                                                  //114/ 
			break;                                                  //115/ 
	}                                                                       //116/ 
	s->s_dev = dev;                                                         //117/ 117-122行将s指向的空闲的内存中超级块结构进行部分初始化，给设备号dev指定的设备使用
	s->s_isup = NULL;                                                       //118/ 
	s->s_imount = NULL;                                                     //119/ 
	s->s_time = 0;                                                          //120/ 
	s->s_rd_only = 0;                                                       //121/ 
	s->s_dirt = 0;                                                          //122/ 
	lock_super(s);                                                          //123/ 锁定该内存中超级块结构
	if (!(bh = bread(dev,1))) {                                             //124/ 从设备号dev指定的设备上读取逻辑块号1指定的数据块(即超级块)到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针赋给bh
		s->s_dev=0;                                                     //125/ 如果读取失败，就释放s指向的内存中超级块结构
		free_super(s);                                                  //126/ 解锁s指向的内存中超级块，并唤醒s->s_wait指向的任务
		return NULL;                                                    //127/ 返回空指针
	}                                                                       //128/ 
	*((struct d_super_block *) s) =                                         //129/ 将从设备号dev指定的设备上读取的超级块信息从缓冲块数据区复制到s指向的空闲的内存中超级块结构中
		*((struct d_super_block *) bh->b_data);                         //130/ 
	brelse(bh);                                                             //131/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	if (s->s_magic != SUPER_MAGIC) {                                        //132/ 如果该块设备中超级块结构中的文件系统幻数指定的文件系统类型不是MINIX1.0文件系统
		s->s_dev = 0;                                                   //133/ 释放s指向的内存中超级块结构
		free_super(s);                                                  //134/ 解锁s指向的内存中超级块，并唤醒s->s_wait指向的任务
		return NULL;                                                    //135/ 返回空指针
	}                                                                       //136/ 
	for (i=0;i<I_MAP_SLOTS;i++)                                             //137/ 遍寻设备号dev指定的设备的内存中超级块结构中的i节点位图在高速缓冲块的指针数组(共I_MAP_SLOTS=8个元素)
		s->s_imap[i] = NULL;                                            //138/ 将s指向的内存中超级块结构中的i节点位图在高速缓冲块的指针数组清空
	for (i=0;i<Z_MAP_SLOTS;i++)                                             //139/ 遍寻设备号dev指定的设备的内存中超级块结构中的逻辑块位图在高速缓冲块的指针数组(共Z_MAP_SLOTS=8个元素)
		s->s_zmap[i] = NULL;                                            //140/ 将s指向的内存中超级块结构中的逻辑块位图在高速缓冲块的指针数组清空
	block=2;                                                                //141/ i节点位图保存在设备上2号块开始的逻辑块中
	for (i=0 ; i < s->s_imap_blocks ; i++)                                  //142/ 142-146行将s指向的内存中超级块结构中的i节点位图在高速缓冲块的指针数组初始化，使它们指向对应的i节点位图所在的高速缓冲块
		if (s->s_imap[i]=bread(dev,block))                              //143/ 从设备号dev指定的设备上读取逻辑块号block指定的数据块(即i节点位图所在的数据块)到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针
			block++;                                                //144/ 
		else                                                            //145/ 
			break;                                                  //146/ 
	for (i=0 ; i < s->s_zmap_blocks ; i++)                                  //147/ 147-151行将s指向的内存中超级块结构中的逻辑块位图在高速缓冲块的指针数组初始化，使它们指向对应的逻辑块位图所在的高速缓冲块
		if (s->s_zmap[i]=bread(dev,block))                              //148/ 从设备号dev指定的设备上读取逻辑块号block指定的数据块(即逻辑块位图所在的数据块)到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针
			block++;                                                //149/ 
		else                                                            //150/ 
			break;                                                  //151/ 
	if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {                     //152/ 如果读出的i节点位图和逻辑块位图所占用的总逻辑块数不等于s指向的内存中超级块结构中i节点位图和逻辑块位图应该占有的总逻辑块数
		for(i=0;i<I_MAP_SLOTS;i++)                                      //153/ 153-156行释放i节点位图和逻辑块位图占用的高速缓冲块
			brelse(s->s_imap[i]);                                   //154/ 等待不为空的s->s_imap[i]指定的缓冲块解锁，将s->s_imap[i]指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
		for(i=0;i<Z_MAP_SLOTS;i++)                                      //155/ 
			brelse(s->s_zmap[i]);                                   //156/ 等待不为空的s->s_zmap[i]指定的缓冲块解锁，将s->s_zmap[i]指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
		s->s_dev=0;                                                     //157/ 释放s指向的内存中超级块结构
		free_super(s);                                                  //158/ 解锁s指向的内存中超级块，并唤醒s->s_wait指向的任务
		return NULL;                                                    //159/ 返回空指针
	}                                                                       //160/ 
	s->s_imap[0]->b_data[0] |= 1;                                           //161/ 将s指向的内存中超级块结构中i节点位图的最低位置1
	s->s_zmap[0]->b_data[0] |= 1;                                           //162/ 将s指向的内存中超级块结构中逻辑块位图的最低位置1
	free_super(s);                                                          //163/ 解锁s指向的内存中超级块，并唤醒s->s_wait指向的任务
	return s;                                                               //164/ 
}                                                                               //165/ 
                                                                                //166/ 
int sys_umount(char * dev_name)                                                 //167/ [b;]
{                                                                               //168/ 
	struct m_inode * inode;                                                 //169/ 
	struct super_block * sb;                                                //170/ 
	int dev;                                                                //171/ 
                                                                                //172/ 
	if (!(inode=namei(dev_name)))                                           //173/ 
		return -ENOENT;                                                 //174/ 
	dev = inode->i_zone[0];                                                 //175/ 
	if (!S_ISBLK(inode->i_mode)) {                                          //176/ 
		iput(inode);                                                    //177/ 
		return -ENOTBLK;                                                //178/ 
	}                                                                       //179/ 
	iput(inode);                                                            //180/ 
	if (dev==ROOT_DEV)                                                      //181/ 
		return -EBUSY;                                                  //182/ 
	if (!(sb=get_super(dev)) || !(sb->s_imount))                            //183/ 
		return -ENOENT;                                                 //184/ 
	if (!sb->s_imount->i_mount)                                             //185/ 
		printk("Mounted inode has i_mount=0\n");                        //186/ 
	for (inode=inode_table+0 ; inode<inode_table+NR_INODE ; inode++)        //187/ 
		if (inode->i_dev==dev && inode->i_count)                        //188/ 
				return -EBUSY;                                  //189/ 
	sb->s_imount->i_mount=0;                                                //190/ 
	iput(sb->s_imount);                                                     //191/ 
	sb->s_imount = NULL;                                                    //192/ 
	iput(sb->s_isup);                                                       //193/ 
	sb->s_isup = NULL;                                                      //194/ 
	put_super(dev);                                                         //195/ 
	sync_dev(dev);                                                          //196/ 
	return 0;                                                               //197/ 
}                                                                               //198/ 
                                                                                //199/ 
int sys_mount(char * dev_name, char * dir_name, int rw_flag)                    //200/ [b;]
{                                                                               //201/ 
	struct m_inode * dev_i, * dir_i;                                        //202/ 
	struct super_block * sb;                                                //203/ 
	int dev;                                                                //204/ 
                                                                                //205/ 
	if (!(dev_i=namei(dev_name)))                                           //206/ 
		return -ENOENT;                                                 //207/ 
	dev = dev_i->i_zone[0];                                                 //208/ 
	if (!S_ISBLK(dev_i->i_mode)) {                                          //209/ 
		iput(dev_i);                                                    //210/ 
		return -EPERM;                                                  //211/ 
	}                                                                       //212/ 
	iput(dev_i);                                                            //213/ 
	if (!(dir_i=namei(dir_name)))                                           //214/ 
		return -ENOENT;                                                 //215/ 
	if (dir_i->i_count != 1 || dir_i->i_num == ROOT_INO) {                  //216/ 
		iput(dir_i);                                                    //217/ 
		return -EBUSY;                                                  //218/ 
	}                                                                       //219/ 
	if (!S_ISDIR(dir_i->i_mode)) {                                          //220/ 
		iput(dir_i);                                                    //221/ 
		return -EPERM;                                                  //222/ 
	}                                                                       //223/ 
	if (!(sb=read_super(dev))) {                                            //224/ 
		iput(dir_i);                                                    //225/ 
		return -EBUSY;                                                  //226/ 
	}                                                                       //227/ 
	if (sb->s_imount) {                                                     //228/ 
		iput(dir_i);                                                    //229/ 
		return -EBUSY;                                                  //230/ 
	}                                                                       //231/ 
	if (dir_i->i_mount) {                                                   //232/ 
		iput(dir_i);                                                    //233/ 
		return -EPERM;                                                  //234/ 
	}                                                                       //235/ 
	sb->s_imount=dir_i;                                                     //236/ 
	dir_i->i_mount=1;                                                       //237/ 
	dir_i->i_dirt=1;		/* NOTE! we don't iput(dir_i) */        //238/ 
	return 0;			/* we do that in umount */              //239/ 
}                                                                               //240/ 
                                                                                //241/ 
void mount_root(void)                                                           //242/ [b;]安装根文件系统
{                                                                               //243/ 
	int i,free;                                                             //244/ 
	struct super_block * p;                                                 //245/ 
	struct m_inode * mi;                                                    //246/ 
                                                                                //247/ 
	if (32 != sizeof (struct d_inode))                                      //248/ 如果块设备上i节点结构不是32字节(此判断用于防止修改代码时出现不一致的情况)
		panic("bad i-node size");                                       //249/ 则直接死机
	for(i=0;i<NR_FILE;i++)                                                  //250/ 遍寻文件表(数组)的NR_FILE=64个表项
		file_table[i].f_count=0;                                        //251/ 将每个表项中的对应文件引用计数值清0，表示空闲
	if (MAJOR(ROOT_DEV) == 2) {                                             //252/ 如果根文件系统所在设备是软盘
		printk("Insert root floppy and press ENTER");                   //253/ 则打印提示信息--“请插入根文件系统盘，并按回车键”
		wait_for_keypress();                                            //254/ [r;]与字符设备有关
	}                                                                       //255/ 
	for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++) {           //256/ 遍寻NR_SUPER=8个内存中超级块结构，将它们初始化
		p->s_dev = 0;                                                   //257/ 将超级块中的超级块所在设备号清0
		p->s_lock = 0;                                                  //258/ 将超级块中的锁定标志清0
		p->s_wait = NULL;                                               //259/ 将超级块中的等待本超级块的进程指针清空
	}                                                                       //260/ 
	if (!(p=read_super(ROOT_DEV)))                                          //261/ 如果设备号ROOT_DEV指定的设备上的文件系统超级块已经在超级块结构表中，则直接返回指向该超级块项的指针赋给p，否则就从该设备上读取超级块到缓冲块中，并复制到超级块结构表中，并返回指向该内存中超级块结构的指针赋给p，如果p为空指针
		panic("Unable to mount root");                                  //262/ 则直接死机
	if (!(mi=iget(ROOT_DEV,ROOT_INO)))                                      //263/ 从设备号ROOT_DEV指定的设备上读取i节点号ROOT_INO=1指定的i节点(即根i节点)到内存中i节点表中，并返回指向该内存中i节点的指针赋给mi，如果mi为空指针
		panic("Unable to read root i-node");                            //264/ 则直接死机
	mi->i_count += 3 ;	/* NOTE! it is logically used 4 times, not 1 */ //265/ 将根i节点的引用计数递增3次(因为在iget中已设置为1，而266-268行引用了4次)
	p->s_isup = p->s_imount = mi;                                           //266/ 将p指向的内存中超级块结构中的(指向被安装文件系统的根目录的i节点)和(指向)字段都设置为根i节点指针mi
	current->pwd = mi;                                                      //267/ 将当前进程的(进程当前的工作目录i节点结构指针)字段设置为根i节点指针mi
	current->root = mi;                                                     //268/ 将当前进程的(进程自己的根目录i节点结构指针)字段设置为根i节点指针mi
	free=0;                                                                 //269/ 初始化空闲的总逻辑块数变量为0
	i=p->s_nzones;                                                          //270/ 
	while (-- i >= 0)                                                       //271/ 遍寻设备号ROOT_DEV指定的设备上的所有逻辑块对应的逻辑块位图中的位
		if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))                  //272/ 将地址(p->s_zmap[i>>13]->b_data)指向的数据中偏移(i&8191)的那一位的位值(0或1)返回，若返回值为0，表示对应的逻辑块空闲
			free++;                                                 //273/ 则将空闲的总逻辑块数变量free增1
	printk("%d/%d free blocks\n\r",free,p->s_nzones);                       //274/ 打印设备号ROOT_DEV指定的设备上的空闲的总逻辑块数
	free=0;                                                                 //275/ 初始化空闲的总i节点数变量为0
	i=p->s_ninodes+1;                                                       //276/ 
	while (-- i >= 0)                                                       //277/ 遍寻设备号ROOT_DEV指定的设备上的所有i节点对应的i节点位图中的位
		if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))                  //278/ 将地址(p->s_imap[i>>13]->b_data)指向的数据中偏移(i&8191)的那一位的位值(0或1)返回，若返回值为0，表示对应的i节点空闲
			free++;                                                 //279/ 则将空闲的总i节点数变量free增1
	printk("%d/%d free inodes\n\r",free,p->s_ninodes);                      //280/ 打印设备号ROOT_DEV指定的设备上的空闲的总i节点数
}                                                                               //281/ 

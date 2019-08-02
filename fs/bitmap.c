/*                                                                             //  1/ 
 *  linux/fs/bitmap.c                                                          //  2/ 
 *                                                                             //  3/ 
 *  (C) 1991  Linus Torvalds                                                   //  4/ 
 */                                                                            //  5/ 
                                                                               //  6/ 
/* bitmap.c contains the code that handles the inode and block bitmaps */      //  7/ 
#include <string.h>                                                            //  8/ 
                                                                               //  9/ 
#include <linux/sched.h>                                                       // 10/ 
#include <linux/kernel.h>                                                      // 11/ 
                                                                               // 12/ 
#define clear_block(addr) \                                                    // 13/ 
__asm__("cld\n\t" \                                                            // 14/ 
	"rep\n\t" \                                                            // 15/ 
	"stosl" \                                                              // 16/ 
	::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr)):"cx","di")            // 17/ 
                                                                               // 18/ 
#define set_bit(nr,addr) ({\                                                   // 19/ 
register int res __asm__("ax"); \                                              // 20/ 
__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \                              // 21/ 
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \                                  // 22/ 
res;})                                                                         // 23/ 
                                                                               // 24/ 
#define clear_bit(nr,addr) ({\                                                 // 25/ 复位指定地址addr开始的第nr位偏移处的比特位，返回原比特位值的反码
register int res __asm__("ax"); \                                              // 26/ 
__asm__ __volatile__("btrl %2,%3\n\tsetnb %%al": \                             // 27/ 
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \                                  // 28/ 
res;})                                                                         // 29/ 
                                                                               // 30/ 
#define find_first_zero(addr) ({ \                                             // 31/ 
int __res; \                                                                   // 32/ 
__asm__("cld\n" \                                                              // 33/ 
	"1:\tlodsl\n\t" \                                                      // 34/ 
	"notl %%eax\n\t" \                                                     // 35/ 
	"bsfl %%eax,%%edx\n\t" \                                               // 36/ 
	"je 2f\n\t" \                                                          // 37/ 
	"addl %%edx,%%ecx\n\t" \                                               // 38/ 
	"jmp 3f\n" \                                                           // 39/ 
	"2:\taddl $32,%%ecx\n\t" \                                             // 40/ 
	"cmpl $8192,%%ecx\n\t" \                                               // 41/ 
	"jl 1b\n" \                                                            // 42/ 
	"3:" \                                                                 // 43/ 
	:"=c" (__res):"c" (0),"S" (addr):"ax","dx","si"); \                    // 44/ 
__res;})                                                                       // 45/ 
                                                                               // 46/ 
void free_block(int dev, int block)                                            // 47/ [b;]利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号block指定的缓冲块，如果找到就将其释放(引用计数递减1)，唤醒buffer_wait指向的任务，如果引用计数递减1后为0，则复位该缓冲块的已修改标志和更新标志；复位逻辑块号block在逻辑块位图中的比特位，置位相应逻辑块位图所在缓冲块已修改标志
{                                                                              // 48/ 
	struct super_block * sb;                                               // 49/ 
	struct buffer_head * bh;                                               // 50/ 
                                                                               // 51/ 
	if (!(sb = get_super(dev)))                                            // 52/ 取设备号dev指定的设备对应的内存中超级块结构，返回指向该内存中超级块的指针赋给sb
		panic("trying to free block on nonexistent device");           // 53/ 
	if (block < sb->s_firstdatazone || block >= sb->s_nzones)              // 54/ 保证逻辑块号block指定的逻辑块是存在于设备上数据区的逻辑块
		panic("trying to free block not in datazone");                 // 55/ 
	bh = get_hash_table(dev,block);                                        // 56/ 利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号block指定的缓冲块，如果找到就增加其引用计数并返回指向其对应缓冲头结构的指针赋给bh
	if (bh) {                                                              // 57/ 如果bh不为空
		if (bh->b_count != 1) {                                        // 58/ 如果bh指定的缓冲块的引用计数不为1 [r;]存在一个bug——在引用计数大于1时，没有执行释放操作，见P539
			printk("trying to free block (%04x:%d), count=%d\n",   // 59/ 则不允许释放，打印调试信息
				dev,block,bh->b_count);                        // 60/ 
			return;                                                // 61/ 直接返回
		}                                                              // 62/ 
		bh->b_dirt=0;                                                  // 63/ 复位bh指定的缓冲块的已修改标志
		bh->b_uptodate=0;                                              // 64/ 复位bh指定的缓冲块的更新标志
		brelse(bh);                                                    // 65/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	}                                                                      // 66/ 
	block -= sb->s_firstdatazone - 1 ;                                     // 67/ 67-71行用于复位逻辑块号block在逻辑块位图中的比特位，如果对应比特位原本就是0，则出错停机
	if (clear_bit(block&8191,sb->s_zmap[block/8192]->b_data)) {            // 68/ 
		printk("block (%04x:%d) ",dev,block+sb->s_firstdatazone-1);    // 69/ 
		panic("free_block: bit already cleared");                      // 70/ 
	}                                                                      // 71/ 
	sb->s_zmap[block/8192]->b_dirt = 1;                                    // 72/ 置位相应逻辑块位图所在缓冲块已修改标志
}                                                                              // 73/ 
                                                                               // 74/ 
int new_block(int dev)                                                         // 75/ [b;]
{                                                                              // 76/ 
	struct buffer_head * bh;                                               // 77/ 
	struct super_block * sb;                                               // 78/ 
	int i,j;                                                               // 79/ 
                                                                               // 80/ 
	if (!(sb = get_super(dev)))                                            // 81/ 
		panic("trying to get new block from nonexistant device");      // 82/ 
	j = 8192;                                                              // 83/ 
	for (i=0 ; i<8 ; i++)                                                  // 84/ 
		if (bh=sb->s_zmap[i])                                          // 85/ 
			if ((j=find_first_zero(bh->b_data))<8192)              // 86/ 
				break;                                         // 87/ 
	if (i>=8 || !bh || j>=8192)                                            // 88/ 
		return 0;                                                      // 89/ 
	if (set_bit(j,bh->b_data))                                             // 90/ 
		panic("new_block: bit already set");                           // 91/ 
	bh->b_dirt = 1;                                                        // 92/ 
	j += i*8192 + sb->s_firstdatazone-1;                                   // 93/ 
	if (j >= sb->s_nzones)                                                 // 94/ 
		return 0;                                                      // 95/ 
	if (!(bh=getblk(dev,j)))                                               // 96/ 
		panic("new_block: cannot get block");                          // 97/ 
	if (bh->b_count != 1)                                                  // 98/ 
		panic("new block: count is != 1");                             // 99/ 
	clear_block(bh->b_data);                                               //100/ 
	bh->b_uptodate = 1;                                                    //101/ 
	bh->b_dirt = 1;                                                        //102/ 
	brelse(bh);                                                            //103/ 
	return j;                                                              //104/ 
}                                                                              //105/ 
                                                                               //106/ 
void free_inode(struct m_inode * inode)                                        //107/ [b;]如果inode指定的内存中i节点未被使用，则将该内存中i节点释放(内容清零)，如果已使用，则复位inode指定的内存中i节点对应的i节点位图中的偏移位，并置该i节点位图所在的高速缓冲块的已修改标志，然后再将该内存中i节点释放(内容清零)
{                                                                              //108/ 
	struct super_block * sb;                                               //109/ 
	struct buffer_head * bh;                                               //110/ 
                                                                               //111/ 
	if (!inode)                                                            //112/ 如果inode为空指针
		return;                                                        //113/ 则返回
	if (!inode->i_dev) {                                                   //114/ 如果inode指定的内存中i节点上的设备号字段为0，即该i节点未被使用
		memset(inode,0,sizeof(*inode));                                //115/ 则将该内存中i节点内容清零——用数字0填写inode指向的内存区域，共填写sizeof(*inode)个字节(此时可能ds、es都指向了同一个段)
		return;                                                        //116/ 返回
	}                                                                      //117/ 
	if (inode->i_count>1) {                                                //118/ 如果inode指定的内存中i节点的引用计数大于1，即还有其他程序引用它，说明内核有问题
		printk("trying to free inode with count=%d\n",inode->i_count); //119/ 则打印调试信息
		panic("free_inode");                                           //120/ 死机
	}                                                                      //121/ 
	if (inode->i_nlinks)                                                   //122/ 如果inode指定的内存中i节点的硬链接数不为0
		panic("trying to free inode with links");                      //123/ 则直接死机
	if (!(sb = get_super(inode->i_dev)))                                   //124/ 取设备号(inode->i_dev)指定的设备对应的内存中超级块结构，返回指向该内存中超级块的指针赋给sb，如果该指针为空
		panic("trying to free inode on nonexistent device");           //125/ 则直接死机
	if (inode->i_num < 1 || inode->i_num > sb->s_ninodes)                  //126/ 如果inode指定的内存中i节点的节点号小于1或者大于i节点总数
		panic("trying to free inode 0 or nonexistant inode");          //127/ 则直接死机
	if (!(bh=sb->s_imap[inode->i_num>>13]))                                //128/ 将inode指定的内存中i节点对应的i节点位图所在的高速缓冲块指针赋给bh，如果bh为空指针
		panic("nonexistent imap in superblock");                       //129/ 则直接死机
	if (clear_bit(inode->i_num&8191,bh->b_data))                           //130/ 复位inode指定的内存中i节点对应的i节点位图中的偏移位——复位指定地址(bh->b_data)开始的第(inode->i_num&8191)位偏移处的比特位，返回原比特位值的反码
		printk("free_inode: bit already cleared.\n\r");                //131/ 
	bh->b_dirt = 1;                                                        //132/ 置inode指定的内存中i节点对应的i节点位图所在的高速缓冲块的已修改标志
	memset(inode,0,sizeof(*inode));                                        //133/ 将inode指定的内存中i节点内容清零——用数字0填写inode指向的内存区域，共填写sizeof(*inode)个字节(此时可能ds、es都指向了同一个段)
}                                                                              //134/ 
                                                                               //135/ 
struct m_inode * new_inode(int dev)                                            //136/ [b;]
{                                                                              //137/ 
	struct m_inode * inode;                                                //138/ 
	struct super_block * sb;                                               //139/ 
	struct buffer_head * bh;                                               //140/ 
	int i,j;                                                               //141/ 
                                                                               //142/ 
	if (!(inode=get_empty_inode()))                                        //143/ 
		return NULL;                                                   //144/ 
	if (!(sb = get_super(dev)))                                            //145/ 
		panic("new_inode with unknown device");                        //146/ 
	j = 8192;                                                              //147/ 
	for (i=0 ; i<8 ; i++)                                                  //148/ 
		if (bh=sb->s_imap[i])                                          //149/ 
			if ((j=find_first_zero(bh->b_data))<8192)              //150/ 
				break;                                         //151/ 
	if (!bh || j >= 8192 || j+i*8192 > sb->s_ninodes) {                    //152/ 
		iput(inode);                                                   //153/ 
		return NULL;                                                   //154/ 
	}                                                                      //155/ 
	if (set_bit(j,bh->b_data))                                             //156/ 
		panic("new_inode: bit already set");                           //157/ 
	bh->b_dirt = 1;                                                        //158/ 
	inode->i_count=1;                                                      //159/ 
	inode->i_nlinks=1;                                                     //160/ 
	inode->i_dev=dev;                                                      //161/ 
	inode->i_uid=current->euid;                                            //162/ 
	inode->i_gid=current->egid;                                            //163/ 
	inode->i_dirt=1;                                                       //164/ 
	inode->i_num = j + i*8192;                                             //165/ 
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;       //166/ 
	return inode;                                                          //167/ 
}                                                                              //168/ 

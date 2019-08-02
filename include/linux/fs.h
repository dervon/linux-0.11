/*                                                                               //  1/ 
 * This file has definitions for some important file table                       //  2/ 
 * structures etc.                                                               //  3/ 
 */                                                                              //  4/ 
                                                                                 //  5/ 
#ifndef _FS_H                                                                    //  6/ 
#define _FS_H                                                                    //  7/ 
                                                                                 //  8/ 
#include <sys/types.h>                                                           //  9/ 
                                                                                 // 10/ 
/* devices are as follows: (same as minix, so we can use the minix               // 11/ 
 * file system. These are major numbers.)                                        // 12/ 
 *                                                                               // 13/ 
 * 0 - unused (nodev)                                                            // 14/ 
 * 1 - /dev/mem                                                                  // 15/ 
 * 2 - /dev/fd                                                                   // 16/ 
 * 3 - /dev/hd                                                                   // 17/ 
 * 4 - /dev/ttyx                                                                 // 18/ 
 * 5 - /dev/tty                                                                  // 19/ 
 * 6 - /dev/lp                                                                   // 20/ 
 * 7 - unnamed pipes                                                             // 21/ 
 */                                                                              // 22/ 
                                                                                 // 23/ 
#define IS_SEEKABLE(x) ((x)>=1 && (x)<=3)                                        // 24/ 
                                                                                 // 25/ 
#define READ 0                                                                   // 26/ 
#define WRITE 1                                                                  // 27/ 
#define READA 2		/* read-ahead - don't pause */                           // 28/ 
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */         // 29/ 
                                                                                 // 30/ 
void buffer_init(long buffer_end);                                               // 31/ 
                                                                                 // 32/ 
#define MAJOR(a) (((unsigned)(a))>>8)                                            // 33/ 取处设备号a中的主设备号
#define MINOR(a) ((a)&0xff)                                                      // 34/ 取处设备号a中的次设备号
                                                                                 // 35/ 
#define NAME_LEN 14                                                              // 36/ 
#define ROOT_INO 1                                                               // 37/ 根i节点号
                                                                                 // 38/ 
#define I_MAP_SLOTS 8                                                            // 39/ 内存中超级块结构中的i节点位图在高速缓冲块的指针数组的元素个数
#define Z_MAP_SLOTS 8                                                            // 40/ 内存中超级块结构中的逻辑块位图在高速缓冲块的指针数组的元素个数
#define SUPER_MAGIC 0x137F                                                       // 41/ MINIX1.0文件系统的幻数
                                                                                 // 42/ 
#define NR_OPEN 20                                                               // 43/ 可打开文件数
#define NR_INODE 32                                                              // 44/ 建有MINIX文件系统的360KB软盘中的总i节点数
#define NR_FILE 64                                                               // 45/ 文件表(数组)表项的项数，即系统同时只能打开64个文件
#define NR_SUPER 8                                                               // 46/ 内存中超级块结构表(数组)的表项数
#define NR_HASH 307                                                              // 47/ 高速缓冲块的缓冲头结构的哈希表长度
#define NR_BUFFERS nr_buffers                                                    // 48/ 
#define BLOCK_SIZE 1024                                                          // 49/ 
#define BLOCK_SIZE_BITS 10                                                       // 50/ 
#ifndef NULL                                                                     // 51/ 
#define NULL ((void *) 0)                                                        // 52/ 
#endif                                                                           // 53/ 
                                                                                 // 54/ 
#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))                // 55/ 块设备每个逻辑块可存放的i节点数
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))         // 56/ 
                                                                                 // 57/ 
#define PIPE_HEAD(inode) ((inode).i_zone[0])                                     // 58/ 
#define PIPE_TAIL(inode) ((inode).i_zone[1])                                     // 59/ 
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))     // 60/ 
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))                   // 61/ 
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))                       // 62/ 
#define INC_PIPE(head) \                                                         // 63/ 
__asm__("incl %0\n\tandl $4095,%0"::"m" (head))                                  // 64/ 
                                                                                 // 65/ 
typedef char buffer_block[BLOCK_SIZE];                                           // 66/ 
                                                                                 // 67/ 
struct buffer_head {                                                             // 68/ 缓冲块头数据结构
	char * b_data;			/* pointer to data block (1024 bytes) */ // 69/ 指向该缓冲块中数据区(1024字节)的指针
	unsigned long b_blocknr;	/* block number */                       // 70/ 块设备上的逻辑块号(1块=2扇区)
	unsigned short b_dev;		/* device (0 = free) */                  // 71/ 数据源的设备号(主+次)(0 = free)
	unsigned char b_uptodate;                                                // 72/ 更新标志：表示该缓冲块中数据是否已更新，即是否有效
	unsigned char b_dirt;		/* 0-clean,1-dirty */                    // 73/ 修改标志：0- 未修改;1- 已修改,表示该缓冲块上的数据与块设备上的对应数据块内容不同
	unsigned char b_count;		/* users using this block */             // 74/ 使用该块的用户数，表示该缓冲块正被各个进程引用的次数。b_count是0的块为空闲块
	unsigned char b_lock;		/* 0 - ok, 1 -locked */                  // 75/ 缓冲区是否被锁定：0- 未锁定;1- 已锁定,表示驱动程序正在对该缓冲块内容进行修改
	struct task_struct * b_wait;                                             // 76/ 指向等待该缓冲块解锁的任务
	struct buffer_head * b_prev;                                             // 77/ hash队列上前一块(这四个指针用于缓冲区管理)
	struct buffer_head * b_next;                                             // 78/ hash队列上下一块
	struct buffer_head * b_prev_free;                                        // 79/ 空闲表上前一块
	struct buffer_head * b_next_free;                                        // 80/ 空闲表上下一块
};                                                                               // 81/ 
                                                                                 // 82/ 
struct d_inode {                                                                 // 83/ 块设备上i节点结构
	unsigned short i_mode;                                                   // 84/ 文件的类型和访问权限属性
	unsigned short i_uid;                                                    // 85/ 文件宿主的用户ID
	unsigned long i_size;                                                    // 86/ 文件长度(字节)
	unsigned long i_time;                                                    // 87/ 修改时间(从1970.1.1:0时算起，秒)
	unsigned char i_gid;                                                     // 88/ 文件宿主的组ID
	unsigned char i_nlinks;                                                  // 89/ 硬链接数(有多少个文件目录项指向该i节点)
	unsigned short i_zone[9];                                                // 90/ 文件所占用的盘上逻辑块号数组
};                                                                               // 91/ 
                                                                                 // 92/ 
struct m_inode {                                                                 // 93/ 内存中i节点结构
	unsigned short i_mode;                                                   // 94/ 文件的类型和访问权限属性
	unsigned short i_uid;                                                    // 95/ 文件宿主的用户ID
	unsigned long i_size;                                                    // 96/ 文件长度(字节)
	unsigned long i_mtime;                                                   // 97/ 修改时间(从1970.1.1:0时算起，秒)
	unsigned char i_gid;                                                     // 98/ 文件宿主的组ID
	unsigned char i_nlinks;                                                  // 99/ 硬链接数(有多少个文件目录项指向该i节点)
	unsigned short i_zone[9];                                                //100/ 文件所占用的盘上逻辑块号数组
/* these are in memory also */                                                   //101/ 
	struct task_struct * i_wait;                                             //102/ 等待该i节点的进程
	unsigned long i_atime;                                                   //103/ 最后访问时间
	unsigned long i_ctime;                                                   //104/ i节点自身被修改时间
	unsigned short i_dev;                                                    //105/ i节点所在的设备号
	unsigned short i_num;                                                    //106/ i节点号
	unsigned short i_count;                                                  //107/ i节点被引用的次数，0表示空闲
	unsigned char i_lock;                                                    //108/ i节点被锁定标志
	unsigned char i_dirt;                                                    //109/ i节点已被修改(脏)标志
	unsigned char i_pipe;                                                    //110/ i节点用作管道标志
	unsigned char i_mount;                                                   //111/ i节点挂载了其他文件系统标志
	unsigned char i_seek;                                                    //112/ 搜索标志(lseek操作时)
	unsigned char i_update;                                                  //113/ i节点已更新标志
};                                                                               //114/ 
                                                                                 //115/ 
struct file {                                                                    //116/ 
	unsigned short f_mode;                                                   //117/ 文件操作模式(RW位)
	unsigned short f_flags;                                                  //118/ 文件打开和控制的标志
	unsigned short f_count;                                                  //119/ 对应文件引用计数值
	struct m_inode * f_inode;                                                //120/ 指向对应的内存中i节点
	off_t f_pos;                                                             //121/ 文件位置(读写偏移值)
};                                                                               //122/ 
                                                                                 //123/ 
struct super_block {                                                             //124/ 内存中超级块结构
	unsigned short s_ninodes;                                                //125/ 设备上的i节点总数
	unsigned short s_nzones;                                                 //126/ 设备上以逻辑块为单位的总逻辑块数(或称为区块)
	unsigned short s_imap_blocks;                                            //127/ i节点位图所占磁盘块数
	unsigned short s_zmap_blocks;                                            //128/ 逻辑块位图所占磁盘块数
	unsigned short s_firstdatazone;                                          //129/ 设备上数据区开始处占用的第一个逻辑块块号
	unsigned short s_log_zone_size;                                          //130/ Log2(磁盘块数/逻辑块)，对于MINIX1.0文件系统，每个逻辑块包含一个磁盘块，都为1KB大小
	unsigned long s_max_size;                                                //131/ 最大文件长度(字节表示)
	unsigned short s_magic;                                                  //132/ 文件系统幻数，指明文件系统的类型(0x137f表示MINIX1.0文件系统)
/* These are only in memory */                                                   //133/ 
	struct buffer_head * s_imap[8];                                          //134/ i节点位图在高速缓冲块指针数组
	struct buffer_head * s_zmap[8];                                          //135/ 逻辑块位图在高速缓冲块指针数组
	unsigned short s_dev;                                                    //136/ 超级块所在设备号
	struct m_inode * s_isup;                                                 //137/ 被安装文件系统根目录i节点
	struct m_inode * s_imount;                                               //138/ 被安装文件系统被挂载到的i节点
	unsigned long s_time;                                                    //139/ 修改时间
	struct task_struct * s_wait;                                             //140/ 等待本超级块的进程指针
	unsigned char s_lock;                                                    //141/ 锁定标志
	unsigned char s_rd_only;                                                 //142/ 只读标志
	unsigned char s_dirt;                                                    //143/ 已被修改(脏)标志
};                                                                               //144/ 
                                                                                 //145/ 
struct d_super_block {                                                           //146/ 块设备中超级块结构
	unsigned short s_ninodes;                                                //147/ 设备上的i节点总数
	unsigned short s_nzones;                                                 //148/ 设备上以逻辑块为单位的总逻辑块数(或称为区块)
	unsigned short s_imap_blocks;                                            //149/ i节点位图所占磁盘块数
	unsigned short s_zmap_blocks;                                            //150/ 逻辑块位图所占磁盘块数
	unsigned short s_firstdatazone;                                          //151/ 设备上数据区开始处占用的第一个逻辑块块号
	unsigned short s_log_zone_size;                                          //152/ Log2(磁盘块数/逻辑块)，对于MINIX1.0文件系统，每个逻辑块包含一个磁盘块，都为1KB大小
	unsigned long s_max_size;                                                //153/ 最大文件长度(字节表示)
	unsigned short s_magic;                                                  //154/ 文件系统幻数，指明文件系统的类型(0x137f表示MINIX1.0文件系统)
};                                                                               //155/ 
                                                                                 //156/ 
struct dir_entry {                                                               //157/ 文件的目录项结构
	unsigned short inode;                                                    //158/ i节点名
	char name[NAME_LEN];                                                     //159/ 文件名，长度NAME_LEN=14
};                                                                               //160/ 
                                                                                 //161/ 
extern struct m_inode inode_table[NR_INODE];                                     //162/ 内存中i节点结构表(数组)，总共NR_INODE=32个i节点
extern struct file file_table[NR_FILE];                                          //163/ 文件表(数组)(NR_FILE = 64个表项，即系统同时只能打开64个文件)
extern struct super_block super_block[NR_SUPER];                                 //164/ 定义内存中超级块结构表(数组)(NR_SUPER = 8)
extern struct buffer_head * start_buffer;                                        //165/ 高速缓冲区开始于内核代码末端位置，即内核模块结束位置后的第一个地址
extern int nr_buffers;                                                           //166/ 高速缓冲区中划分出来的高速缓冲块的总数目
                                                                                 //167/ 
extern void check_disk_change(int dev);                                          //168/ 
extern int floppy_change(unsigned int nr);                                       //169/ 
extern int ticks_to_floppy_on(unsigned int dev);                                 //170/ 
extern void floppy_on(unsigned int dev);                                         //171/ 
extern void floppy_off(unsigned int dev);                                        //172/ 
extern void truncate(struct m_inode * inode);                                    //173/ 
extern void sync_inodes(void);                                                   //174/ 
extern void wait_on(struct m_inode * inode);                                     //175/ 
extern int bmap(struct m_inode * inode,int block);                               //176/ 
extern int create_block(struct m_inode * inode,int block);                       //177/ 
extern struct m_inode * namei(const char * pathname);                            //178/ 
extern int open_namei(const char * pathname, int flag, int mode,                 //179/ 
	struct m_inode ** res_inode);                                            //180/ 
extern void iput(struct m_inode * inode);                                        //181/ 
extern struct m_inode * iget(int dev,int nr);                                    //182/ 
extern struct m_inode * get_empty_inode(void);                                   //183/ 
extern struct m_inode * get_pipe_inode(void);                                    //184/ 
extern struct buffer_head * get_hash_table(int dev, int block);                  //185/ 
extern struct buffer_head * getblk(int dev, int block);                          //186/ 
extern void ll_rw_block(int rw, struct buffer_head * bh);                        //187/ 
extern void brelse(struct buffer_head * buf);                                    //188/ 
extern struct buffer_head * bread(int dev,int block);                            //189/ 
extern void bread_page(unsigned long addr,int dev,int b[4]);                     //190/ 
extern struct buffer_head * breada(int dev,int block,...);                       //191/ 
extern int new_block(int dev);                                                   //192/ 
extern void free_block(int dev, int block);                                      //193/ 
extern struct m_inode * new_inode(int dev);                                      //194/ 
extern void free_inode(struct m_inode * inode);                                  //195/ 
extern int sync_dev(int dev);                                                    //196/ 
extern struct super_block * get_super(int dev);                                  //197/ 
extern int ROOT_DEV;                                                             //198/ 
                                                                                 //199/ 
extern void mount_root(void);                                                    //200/ 
                                                                                 //201/ 
#endif                                                                           //202/ 

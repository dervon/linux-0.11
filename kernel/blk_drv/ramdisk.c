/*                                                                               //  1/ 
 *  linux/kernel/blk_drv/ramdisk.c                                               //  2/ 
 *                                                                               //  3/ 
 *  Written by Theodore Ts'o, 12/2/91                                            //  4/ 
 */                                                                              //  5/ 
                                                                                 //  6/ 
#include <string.h>                                                              //  7/ 
                                                                                 //  8/ 
#include <linux/config.h>                                                        //  9/ 
#include <linux/sched.h>                                                         // 10/ 
#include <linux/fs.h>                                                            // 11/ 
#include <linux/kernel.h>                                                        // 12/ 
#include <asm/system.h>                                                          // 13/ 
#include <asm/segment.h>                                                         // 14/ 
#include <asm/memory.h>                                                          // 15/ 
                                                                                 // 16/ 
#define MAJOR_NR 1                                                               // 17/ 定义主设备号为ram盘的设备号1，用于给blk.h作出判断，确定一些符号和宏
#include "blk.h"                                                                 // 18/ 
                                                                                 // 19/ 
char	*rd_start;                                                               // 20/ 虚拟盘在内存中的开始地址
int	rd_length = 0;                                                           // 21/ 虚拟盘所占内存大小(字节)
                                                                                 // 22/ 
void do_rd_request(void)                                                         // 23/ [b;]对虚拟盘请求项进行处理，在处理完当前请求项后接着处理虚拟盘的下一请求项，直至处理完虚拟盘的所有请求项
{                                                                                // 24/ 
	int	len;                                                             // 25/ 
	char	*addr;                                                           // 26/ 
                                                                                 // 27/ 
	INIT_REQUEST;                                                            // 28/ 初始化请求项宏——判断当前请求项合法性，若已没有请求项则退出，若当前请求项不合法则死机
	addr = rd_start + (CURRENT->sector << 9);                                // 29/ 计算当前请求项要读写的设备(即虚拟盘)的起始物理扇区对应在物理内存中的起始地址赋给addr
	len = CURRENT->nr_sectors << 9;                                          // 30/ 计算当前请求项要读写的设备(即虚拟盘)的扇区数对应的总字节数赋给len
	if ((MINOR(CURRENT->dev) != 1) || (addr+len > rd_start+rd_length)) {     // 31/ 如果当前请求项对应的设备的子设备号不为1 或 当前请求项要读写的设备(即虚拟盘)中的扇区尾地址对应的内存地址超过了的虚拟盘末尾
		end_request(0);                                                  // 32/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值0，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
		goto repeat;                                                     // 33/ 跳转到INIT_REQUEST宏中的repeat处，重新开始处理下一个请求项
	}                                                                        // 34/ 
	if (CURRENT-> cmd == WRITE) {                                            // 35/ 如果是写命令
		(void ) memcpy(addr,                                             // 36/ 从源地址CURRENT->buffer处开始复制len个字节到目的地址addr处(此时可能ds、es都指向了同一个段)
			      CURRENT->buffer,                                   // 37/ 
			      len);                                              // 38/ 
	} else if (CURRENT->cmd == READ) {                                       // 39/ 如果是读命令
		(void) memcpy(CURRENT->buffer,                                   // 40/ 从源地址addr处开始复制len个字节到目的地址CURRENT->buffer处(此时可能ds、es都指向了同一个段)
			      addr,                                              // 41/ 
			      len);                                              // 42/ 
	} else                                                                   // 43/ 如果既不是读也不是写
		panic("unknown ramdisk-command");                                // 44/ 则死机
	end_request(1);                                                          // 45/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值1，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
	goto repeat;                                                             // 46/ 跳转到INIT_REQUEST宏中的repeat处，重新开始处理下一个请求项
}                                                                                // 47/ 
                                                                                 // 48/ 
/*                                                                               // 49/ 
 * Returns amount of memory which needs to be reserved.                          // 50/ 
 */                                                                              // 51/ 
long rd_init(long mem_start, int length)                                         // 52/ [b;]虚拟盘初始化，设置块设备表(数组)中虚拟盘对应的块设备结构中的请求操作函数指针为do_rd_request，设置虚拟盘在内存中的开始地址为参数mem_start，设置虚拟盘所占内存大小(字节)为参数length，将虚拟盘所占的内存都清零，返回虚拟盘所占内存大小(字节)
{                                                                                // 53/ 
	int	i;                                                               // 54/ 
	char	*cp;                                                             // 55/ 
                                                                                 // 56/ 
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;                           // 57/ 设置块设备表(数组)中虚拟盘对应的块设备结构中的请求操作函数指针为do_rd_request
	rd_start = (char *) mem_start;                                           // 58/ 设置虚拟盘在内存中的开始地址为参数mem_start
	rd_length = length;                                                      // 59/ 设置虚拟盘所占内存大小(字节)为参数length
	cp = rd_start;                                                           // 60/ 60-62行将虚拟盘所占的内存都清零
	for (i=0; i < length; i++)                                               // 61/ 
		*cp++ = '\0';                                                    // 62/ 
	return(length);                                                          // 63/ 返回虚拟盘所占内存大小(字节)
}                                                                                // 64/ 
                                                                                 // 65/ 
/*                                                                               // 66/ 
 * If the root device is the ram disk, try to load it.                           // 67/ 
 * In order to do this, the root device is originally set to the                 // 68/ 
 * floppy, and we later change it to be ram disk.                                // 69/ 
 */                                                                              // 70/ 
void rd_load(void)                                                               // 71/ [b;]尝试将集成盘(软盘)中的根文件系统加载到虚拟盘中，并将根文件系统所在设备的设备号设为虚拟盘的设备号0x0101
{                                                                                // 72/ 
	struct buffer_head *bh;                                                  // 73/ 
	struct super_block	s;                                               // 74/ 
	int		block = 256;	/* Start at block 256 */                 // 75/ 
	int		i = 1;                                                   // 76/ 
	int		nblocks;                                                 // 77/ 
	char		*cp;		/* Move pointer */                       // 78/ 
	                                                                         // 79/ 
	if (!rd_length)                                                          // 80/ 如果虚拟盘所占内存大小(字节)为0，即没有虚拟盘
		return;                                                          // 81/ 则直接退出
	printk("Ram disk: %d bytes, starting at 0x%x\n", rd_length,              // 82/ 打印虚拟盘的内存起始地址和长度信息
		(int) rd_start);                                                 // 83/ 
	if (MAJOR(ROOT_DEV) != 2)                                                // 84/ 如果根文件系统所在的设备不是软盘设备
		return;                                                          // 85/ 则直接退出
	bh = breada(ROOT_DEV,block+1,block,block+2,-1);                          // 86/ 从设备号ROOT_DEV指定的设备读取指定的一些块(257、256、258)到高速缓冲区，返回参数block+1对应的缓冲头指针用于立即使用，其他块只是预读进高速缓冲区，暂时不用
	if (!bh) {                                                               // 87/ 如果block+1对应的缓冲头指针为空
		printk("Disk error while looking for ramdisk!\n");               // 88/ 则打印出错消息
		return;                                                          // 89/ 直接退出
	}                                                                        // 90/ 
	*((struct d_super_block *) &s) = *((struct d_super_block *) bh->b_data); // 91/ 将block+1对应的缓冲块(即超级块)中的块设备中超级块结构复制到s中
	brelse(bh);                                                              // 92/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	if (s.s_magic != SUPER_MAGIC)                                            // 93/ 如果该块设备中超级块结构中的文件系统幻数指定的文件系统类型不是MINIX1.0文件系统
		/* No ram disk image present, assume normal floppy boot */       // 94/ 
		return;                                                          // 95/ 则直接退出
	nblocks = s.s_nzones << s.s_log_zone_size;                               // 96/ 将集成盘(即软盘)上的根文件系统所占的总磁盘块数(逻辑块数 * 每个逻辑块包含的磁盘块数)赋给nblocks
	if (nblocks > (rd_length >> BLOCK_SIZE_BITS)) {                          // 97/ 如果集成盘(即软盘)上的根文件系统所占的总磁盘块数大于内存虚拟盘所能容纳的块数
		printk("Ram disk image too big!  (%d blocks, %d avail)\n",       // 98/ 则打印出错信息
			nblocks, rd_length >> BLOCK_SIZE_BITS);                  // 99/ 
		return;                                                          //100/ 直接退出
	}                                                                        //101/ 
	printk("Loading %d bytes into ram disk... 0000k",                        //102/ 打印加载到虚拟盘的根文件系统所占的总字节数信息
		nblocks << BLOCK_SIZE_BITS);                                     //103/ 
	cp = rd_start;                                                           //104/ 将内存中虚拟盘的起始地址赋给cp
	while (nblocks) {                                                        //105/ 遍寻根文件系统所占的总磁盘块数
		if (nblocks > 2)                                                 //106/ 如果总磁盘块数大于2
			bh = breada(ROOT_DEV, block, block+1, block+2, -1);      //107/ 则超前预读，返回参数block对应的缓冲头指针赋给bh，用于立即使用
		else                                                             //108/ 如果总磁盘块数小于等于2
			bh = bread(ROOT_DEV, block);                             //109/ 则单块读取，返回参数block对应的缓冲头指针赋给bh
		if (!bh) {                                                       //110/ 如果bh为空
			printk("I/O error on block %d, aborting load\n",         //111/ 打印IO操作出错信息，放弃加载
				block);                                          //112/ 直接退出
			return;                                                  //113/ 
		}                                                                //114/ 
		(void) memcpy(cp, bh->b_data, BLOCK_SIZE);                       //115/ 将bh指定的缓冲块中BLOCK_SIZE=1024个字节数据复制到内存中虚拟盘的起始地址处(此时可能ds、es都指向了同一个段)
		brelse(bh);                                                      //116/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
		printk("\010\010\010\010\010%4dk",i);                            //117/ 打印已加载的块数
		cp += BLOCK_SIZE;                                                //118/ 
		block++;                                                         //119/ 
		nblocks--;                                                       //120/ 
		i++;                                                             //121/ 
	}                                                                        //122/ 
	printk("\010\010\010\010\010done \n");                                   //123/ 打印加载完成信息
	ROOT_DEV=0x0101;                                                         //124/ 将根文件系统所在设备的设备号设为虚拟盘的设备号0x0101
}                                                                                //125/ 

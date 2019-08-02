/*                                                                               //  1/ 
 *  linux/kernel/hd.c                                                            //  2/ 
 *                                                                               //  3/ 
 *  (C) 1991  Linus Torvalds                                                     //  4/ 
 */                                                                              //  5/ 
                                                                                 //  6/ 
/*                                                                               //  7/ 
 * This is the low-level hd interrupt support. It traverses the                  //  8/ 
 * request-list, using interrupts to jump between functions. As                  //  9/ 
 * all the functions are called within interrupts, we may not                    // 10/ 
 * sleep. Special care is recommended.                                           // 11/ 
 *                                                                               // 12/ 
 *  modified by Drew Eckhardt to check nr of hd's from the CMOS.                 // 13/ 
 */                                                                              // 14/ 
                                                                                 // 15/ 
#include <linux/config.h>                                                        // 16/ 
#include <linux/sched.h>                                                         // 17/ 
#include <linux/fs.h>                                                            // 18/ 
#include <linux/kernel.h>                                                        // 19/ 
#include <linux/hdreg.h>                                                         // 20/ 
#include <asm/system.h>                                                          // 21/ 
#include <asm/io.h>                                                              // 22/ 
#include <asm/segment.h>                                                         // 23/ 
                                                                                 // 24/ 
#define MAJOR_NR 3                                                               // 25/ 定义主设备号为硬盘的设备号3，用于给blk.h作出判断，确定一些符号和宏
#include "blk.h"                                                                 // 26/ 块设备头文件
                                                                                 // 27/ 
#define CMOS_READ(addr) ({ \                                                     // 28/ 读CMOS RAM中参数的宏函数，0x70是CMOS RAM的索引端口，其最高位是控制NMI中断的开关，为1表示阻断所有NMI信号；0x71是CMOS RAM的数据端口
outb_p(0x80|addr,0x70); \                                                        // 29/ 将0x80|addr通过al寄存器写入0x70端口中并等待一会
inb_p(0x71); \                                                                   // 30/ 从0x71端口读出值到al寄存器中，等待一会然后将值返回(作为宏CMOS_READ的值返回)
})                                                                               // 31/ 
                                                                                 // 32/ 
/* Max read/write errors/sector */                                               // 33/ 
#define MAX_ERRORS	7                                                        // 34/ 读/写一个扇区时允许的最多出错次数，并不一定表示每次读错误尝试最多7次
#define MAX_HD		2                                                        // 35/ 系统支持的最多硬盘数
                                                                                 // 36/ 
static void recal_intr(void);                                                    // 37/ 
                                                                                 // 38/ 
static int recalibrate = 1;                                                      // 39/ 重新校正标志。当设置了该标志，程序中会调用recal_intr()将磁头移动到0柱面
static int reset = 1;                                                            // 40/ 复位标志。当发生读写错误会设置该标志并调用相关复位函数，以复位硬盘和控制器
                                                                                 // 41/ 
/*                                                                               // 42/ 
 *  This struct defines the HD's and their types.                                // 43/ 
 */                                                                              // 44/ 
struct hd_i_struct {                                                             // 45/ 硬盘基本参数表信息结构
	int head,sect,cyl,wpcom,lzone,ctl;                                       // 46/ 磁盘的总磁头数、每磁道扇区数、磁盘的总柱面数、写前预补偿柱面号、磁头着陆区柱面号、控制字节
	};                                                                       // 47/ 
#ifdef HD_TYPE                                                                   // 48/ 如果include/linux/config.h配置文件中定义了符号常数HD_TYPE，就取其中定义好的参数信息，否则就暂时默认都设为0 
struct hd_i_struct hd_info[] = { HD_TYPE };                                      // 49/ 硬盘基本参数表信息结构数组
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))                 // 50/ 计算硬盘个数的宏
#else                                                                            // 51/ 
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };                  // 52/ 默认将两个硬盘基本参数表信息数据都设为0
static int NR_HD = 0;                                                            // 53/ 系统中存在的硬盘的个数
#endif                                                                           // 54/ 
                                                                                 // 55/ 
static struct hd_struct {                                                        // 56/ 定义硬盘分区子结构表(数组)(项0和项5分别表示两个硬盘的整体参数)
	long start_sect;                                                         // 57/ 硬盘或分区在硬盘中从硬盘0道开始算起的起始物理(绝对)扇区号
	long nr_sects;                                                           // 58/ 硬盘或分区中的扇区总数
} hd[5*MAX_HD]={{0,0},};                                                         // 59/ 
                                                                                 // 60/ 
#define port_read(port,buf,nr) \                                                 // 61/ 读端口port，共读nr字(1字占2个字节)，保存在buf中
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr):"cx","di")                 // 62/ 
                                                                                 // 63/ 
#define port_write(port,buf,nr) \                                                // 64/ 从buf中取数据写入端口port，共写nr字(1字占2个字节)
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr):"cx","si")                // 65/ 
                                                                                 // 66/ 
extern void hd_interrupt(void);                                                  // 67/ 
extern void rd_load(void);                                                       // 68/ 
                                                                                 // 69/ 
/* This may be used only once, enforced by 'static int callable' */              // 70/ 
int sys_setup(void * BIOS)                                                       // 71/ [b;]读取CMOS和硬盘参数表信息，用于设置硬盘分区结构hd，并尝试加载RAM虚拟盘和根文件系统(参数BIOS指向内存0x90080处，此处存放着2个硬盘的基本参数表)
{                                                                                // 72/ 
	static int callable = 1;                                                 // 73/ 限制本函数只能被调用1次的标志
	int i,drive;                                                             // 74/ 
	unsigned char cmos_disks;                                                // 75/ 
	struct partition *p;                                                     // 76/ 
	struct buffer_head * bh;                                                 // 77/ 
                                                                                 // 78/ 
	if (!callable)                                                           // 79/ 
		return -1;                                                       // 80/ 
	callable = 0;                                                            // 81/ 
#ifndef HD_TYPE                                                                  // 82/ 如果include/linux/config.h配置文件中未定义符号常数HD_TYPE，说明硬盘信息数组默认都为0，需要在此重新设置 
	for (drive=0 ; drive<2 ; drive++) {                                      // 83/ 
		hd_info[drive].cyl = *(unsigned short *) BIOS;                   // 84/ 填写硬盘的总柱面数
		hd_info[drive].head = *(unsigned char *) (2+BIOS);               // 85/ 填写硬盘的总磁头数
		hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);             // 86/ 填写硬盘的开始写前预补偿柱面号
		hd_info[drive].ctl = *(unsigned char *) (8+BIOS);                // 87/ 填写硬盘的控制字节
		hd_info[drive].lzone = *(unsigned short *) (12+BIOS);            // 88/ 填写硬盘的磁头着陆(停止)柱面号
		hd_info[drive].sect = *(unsigned char *) (14+BIOS);              // 89/ 填写硬盘的每磁道扇区数
		BIOS += 16;                                                      // 90/ 使BIOS指向第二个硬盘的参数表
	}                                                                        // 91/ 
	if (hd_info[1].cyl)                                                      // 92/ 通过判断第二个硬盘的柱面数是否为0得知是否有第二个硬盘存在，并设置硬盘数目变量NR_HD
		NR_HD=2;                                                         // 93/ 
	else                                                                     // 94/ 
		NR_HD=1;                                                         // 95/ 
#endif                                                                           // 96/ 
	for (i=0 ; i<NR_HD ; i++) {                                              // 97/ 设置硬盘分区子结构表hd[]中的项0和项5(代表两个硬盘)的参数
		hd[i*5].start_sect = 0;                                          // 98/ 设置硬盘起始物理(绝对)扇区号
		hd[i*5].nr_sects = hd_info[i].head*                              // 99/ 设置硬盘总扇区数
				hd_info[i].sect*hd_info[i].cyl;                  //100/ 
	}                                                                        //101/ 
                                                                                 //102/ 
	/*                                                                       //103/ 
		We querry CMOS about hard disks : it could be that               //104/ 
		we have a SCSI/ESDI/etc controller that is BIOS                  //105/ 
		compatable with ST-506, and thus showing up in our               //106/ 
		BIOS table, but not register compatable, and therefore           //107/ 
		not present in CMOS.                                             //108/ 
                                                                                 //109/ 
		Furthurmore, we will assume that our ST-506 drives               //110/ 
		<if any> are the primary drives in the system, and               //111/ 
		the ones reflected as drive 1 or 2.                              //112/ 
                                                                                 //113/ 
		The first drive is stored in the high nibble of CMOS             //114/ 
		byte 0x12, the second in the low nibble.  This will be           //115/ 
		either a 4 bit drive type or 0xf indicating use byte 0x19        //116/ 
		for an 8 bit type, drive 1, 0x1a for drive 2 in CMOS.            //117/ 
                                                                                 //118/ 
		Needless to say, a non-zero value means we have                  //119/ 
		an AT controller hard disk for that drive.                       //120/ 
                                                                                 //121/ 
		                                                                 //122/ 
	*/                                                                       //123/ 
                                                                                 //124/ 
	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)                               //125/ [r;]125-135行用来检测硬盘到底是不是AT控制器兼容的
		if (cmos_disks & 0x0f)                                           //126/ 
			NR_HD = 2;                                               //127/ 
		else                                                             //128/ 
			NR_HD = 1;                                               //129/ 
	else                                                                     //130/ 
		NR_HD = 0;                                                       //131/ 
	for (i = NR_HD ; i < 2 ; i++) {                                          //132/ 
		hd[i*5].start_sect = 0;                                          //133/ 
		hd[i*5].nr_sects = 0;                                            //134/ 
	}                                                                        //135/ 
	for (drive=0 ; drive<NR_HD ; drive++) {                                  //136/ 遍寻系统中的NR_HD个硬盘
		if (!(bh = bread(0x300 + drive*5,0))) {                          //137/ 从设备号(0x300 + drive*5)指定的设备上读取逻辑块号0指定的数据块(即主引导扇区MBR)到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针赋给bh
			printk("Unable to read partition table of drive %d\n\r", //138/ 若未读到则打印错误信息--“读不到分区表信息”
				drive);                                          //139/ 
			panic("");                                               //140/ 直接死机
		}                                                                //141/ 
		if (bh->b_data[510] != 0x55 || (unsigned char)                   //142/ 通过硬盘标志0xAA55来判断其中是否有分区表信息
		    bh->b_data[511] != 0xAA) {                                   //143/ 
			printk("Bad partition table on drive %d\n\r",drive);     //144/ 若硬盘标志不符合则打印出错信息--“损坏的分区表信息”
			panic("");                                               //145/ 直接死机
		}                                                                //146/ 
		p = 0x1BE + (void *)bh->b_data;                                  //147/ 硬盘分区表中的4个表项存在于硬盘的0柱面0磁道第1个扇区的0x1BE~0x1FD处
		for (i=1;i<5;i++,p++) {                                          //148/ 遍寻硬盘分区子结构表中10个表项
			hd[i+5*drive].start_sect = p->start_sect;                //149/ 将每个表项对应的分区的起始物理扇区号赋给硬盘分区子结构数组hd中的start_sect
			hd[i+5*drive].nr_sects = p->nr_sects;                    //150/ 将每个表项对应的分区的分区扇区总数赋给硬盘分区子结构数组hd中的nr_sects
		}                                                                //151/ 
		brelse(bh);                                                      //152/ 等待不为空的bh指定的缓冲块解锁，将buf指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	}                                                                        //153/ 
	if (NR_HD)                                                               //154/ 如果确实有硬盘存在
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");            //155/ 打印信息--“分区表正常”
	rd_load();                                                               //156/ 尝试将集成盘(软盘)中的根文件系统加载到虚拟盘中，并将根文件系统所在设备的设备号设为虚拟盘的设备号0x0101
	mount_root();                                                            //157/ 安装根文件系统
	return (0);                                                              //158/ 返回0
}                                                                                //159/ 
                                                                                 //160/ 
static int controller_ready(void)                                                //161/ [b;]判断并循环等待硬盘驱动器就绪，返回剩余的循环判断次数，如果返回0，表示等待超时出错
{                                                                                //162/ 
	int retries=10000;                                                       //163/ 定义循环判断的次数
                                                                                 //164/ 
	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);                      //165/ 读出硬盘控制器主状态寄存器的值，判断里面的(驱动器就绪比特位是否等于0，控制器忙位是否等于0）
	return (retries);                                                        //166/ 
}                                                                                //167/ 
                                                                                 //168/ 
static int win_result(void)                                                      //169/ [b;]检测硬盘命令执行后的状态，返回0表示正常；返回1表示出错(win表示温切斯特硬盘的缩写)
{                                                                                //170/ 
	int i=inb_p(HD_STATUS);                                                  //171/ 读出硬盘控制器主状态寄存器的值赋给i
                                                                                 //172/ 
	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))   //173/ 如果主状态为：命令执行未出错、驱动器寻道结束、驱动器未出写出错故障、驱动器准备就绪、控制器不忙
		== (READY_STAT | SEEK_STAT))                                     //174/ 
		return(0); /* ok */                                              //175/ 则返回0，表示正常
	if (i&1) i=inb(HD_ERROR);                                                //176/ 如果主状态显示命令执行出错，则将硬盘控制器中的错误寄存器的值赋给i[r;]此行似乎没有意义
	return (1);                                                              //177/ 返回1，表示出错
}                                                                                //178/ 
                                                                                 //179/ 
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,      //180/ [b;]向硬盘控制器发送命令块(drive-硬盘号/驱动器号；nsect-读写扇区数；sect-当前磁道内起始扇区号；head-磁头号；cyl-柱面号；cmd-命令码；intr_addr-硬盘中断处理程序中将调用的C处理函数指针)
		unsigned int head,unsigned int cyl,unsigned int cmd,             //181/ 
		void (*intr_addr)(void))                                         //182/ 
{                                                                                //183/ 
	register int port asm("dx");                                             //184/ 定义一个局部寄存器变量port并存放在指定寄存器dx中
                                                                                 //185/ 
	if (drive>1 || head>15)                                                  //186/ 保证硬盘的驱动器号 等于 0或1；保证硬盘的磁头号 小于等于 15
		panic("Trying to write bad sector");                             //187/ 
	if (!controller_ready())                                                 //188/ 判断并循环等待硬盘驱动器就绪，返回剩余的循环判断次数，如果返回0，表示等待超时出错
		panic("HD controller not ready");                                //189/ 
	do_hd = intr_addr;                                                       //190/ 设置硬盘中断发生时将调用的C函数指针do_hd为参数intr_addr指定的函数
	outb_p(hd_info[drive].ctl,HD_CMD);                                       //191/ 将驱动器号drive指定的硬盘的基本参数信息表结构中的控制字节写入硬盘控制器中的硬盘控制寄存器中，以建立相应的硬盘控制方式
	port=HD_DATA;                                                            //192/ 置port为数据寄存器端口0x1f0，开始向0x1f1-0x1f7发送7字节的参数命令块
	outb_p(hd_info[drive].wpcom>>2,++port);                                  //193/ 将驱动器号drive指定的硬盘的基本参数信息表结构中的写前预补偿柱面号(需除以4)写入硬盘控制器中的写前预补偿寄存器
	outb_p(nsect,++port);                                                    //194/ 将参数nsect指定的读写扇区数写入硬盘控制器中的扇区数寄存器
	outb_p(sect,++port);                                                     //195/ 将参数sect指定的起始扇区号写入硬盘控制器中的扇区号寄存器
	outb_p(cyl,++port);                                                      //196/ 将参数cyl指定的柱面号的低8位写入硬盘控制器中的柱面号寄存器
	outb_p(cyl>>8,++port);                                                   //197/ 将参数cyl指定的柱面号的高8位写入硬盘控制器中的柱面号寄存器
	outb_p(0xA0|(drive<<4)|head,++port);                                     //198/ 将参数drive指定的驱动器号和参数head指定的磁头号写入硬盘控制器中的驱动器/磁头寄存器(101dhhhh，d=驱动器号，hhhh=磁头号)
	outb(cmd,++port);                                                        //199/ 将参数cmd指定的命令码写入硬盘控制器中的命令寄存器
}                                                                                //200/ 
                                                                                 //201/ 
static int drive_busy(void)                                                      //202/ [b;]等待硬盘就绪，返回0表示硬盘已就绪；返回1表示等待超时
{                                                                                //203/ 
	unsigned int i;                                                          //204/ 
                                                                                 //205/ 
	for (i = 0; i < 10000; i++)                                              //206/ 循环读取硬盘控制器中的主状态寄存器
		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))   //207/ 如果控制器不忙 且 驱动器已就绪
			break;                                                   //208/ 则退出循环
	i = inb(HD_STATUS);                                                      //209/ 再读取硬盘控制器中的主状态寄存器
	i &= BUSY_STAT | READY_STAT | SEEK_STAT;                                 //210/ 
	if (i == READY_STAT | SEEK_STAT)                                         //211/ 如果控制器在忙 且 驱动器已就绪 且 驱动器寻道已结束
		return(0);                                                       //212/ 则返回0，表示硬盘就绪
	printk("HD controller times out\n\r");                                   //213/ 否则表示等待超时，打印超时出错信息
	return(1);                                                               //214/ 返回1
}                                                                                //215/ 
                                                                                 //216/ 
static void reset_controller(void)                                               //217/ [b;]复位硬盘控制器
{                                                                                //218/ 
	int	i;                                                               //219/ 
                                                                                 //220/ 
	outb(4,HD_CMD);                                                          //221/ 向硬盘控制器中的硬盘控制寄存器中写入控制字节4，即将其位2置1，使硬盘控制器复位
	for(i = 0; i < 100; i++) nop();                                          //222/ 等待一段时间，等待硬盘控制器复位结束
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);                                     //223/ 将驱动器号0指定的硬盘的基本参数信息表结构中的控制字节写入硬盘控制器中的硬盘控制寄存器中(但允许ECC重试、允许访问重试、禁止在柱面数+1处有生产商的坏区图时置1)，以建立相应的硬盘控制方式
	if (drive_busy())                                                        //224/ 等待硬盘就绪，返回0表示硬盘已就绪；返回1表示等待超时
		printk("HD-controller still busy\n\r");                          //225/ 打印信息--“硬盘控制器还在忙”
	if ((i = inb(HD_ERROR)) != 1)                                            //226/ 读取硬盘控制器中的错误寄存器的内容，如果不为1表示硬盘控制器复位失败
		printk("HD-controller reset failed: %02x\n\r",i);                //227/ 打印信息--“硬盘控制器复位失败”
}                                                                                //228/ 
                                                                                 //229/ 
static void reset_hd(int nr)                                                     //230/ [b;]复位硬盘驱动器号nr指定的硬盘控制器，并向硬盘控制器发送“建立驱动器参数”命令块
{                                                                                //231/ 
	reset_controller();                                                      //232/ 复位硬盘控制器
	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,          //233/ 向硬盘控制器发送建立驱动器参数(WIN_SPECIFY)命令块(nr-硬盘号/驱动器号；hd_info[nr].sect-读写扇区数；hd_info[nr].sect-当前磁道内起始扇区号；(hd_info[nr].head-1)-磁头号；hd_info[nr].cyl-柱面号；WIN_SPECIFY-命令码；&recal_intr-硬盘控制器复位及硬盘驱动器重新校正中断调用C处理函数指针)
		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);                        //234/ 
}                                                                                //235/ 
                                                                                 //236/ 
void unexpected_hd_interrupt(void)                                               //237/ [b;]发生意外硬盘中断时，硬盘中断处理程序中调用的默认C处理函数
{                                                                                //238/ 
	printk("Unexpected HD interrupt\n\r");                                   //239/ 
}                                                                                //240/ 
                                                                                 //241/ 
static void bad_rw_intr(void)                                                    //242/ [b;]如果读/写一个扇区时产生的错误次数大于等于MAX_ERRORS(=7，允许的最多出错次数)，则结束当前请求处理；接着判断产生的错误次数是否超过了3次，如果超过了，则置位复位标志，要求执行复位硬盘控制器的操作
{                                                                                //243/ [r;]不明白下一行的CURRENT->errors为什么要加一操作
	if (++CURRENT->errors >= MAX_ERRORS)                                     //244/ 如果读/写一个扇区时产生的错误次数大于等于MAX_ERRORS(=7，允许的最多出错次数)
		end_request(0);                                                  //245/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值0，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
	if (CURRENT->errors > MAX_ERRORS/2)                                      //246/ 如果读/写一个扇区时产生的错误次数大于MAX_ERRORS/2(=7/2 =3)
		reset = 1;                                                       //247/ 则置位复位标志，要求执行复位硬盘控制器的操作
}                                                                                //248/ 
                                                                                 //249/ 
static void read_intr(void)                                                      //250/ [b;]读扇区中断调用函数，在硬盘读命令结束时引发的硬盘中断过程中被调用
{                                                                                //251/ 
	if (win_result()) {                                                      //252/ 检测硬盘命令执行后的状态，返回0表示正常；返回1表示出错(win表示温切斯特硬盘的缩写)
		bad_rw_intr();                                                   //253/ 如果读/写一个扇区时产生的错误次数大于等于MAX_ERRORS(=7，允许的最多出错次数)，则结束当前请求处理；接着判断产生的错误次数是否超过了3次，如果超过了，则置位复位标志，要求执行复位硬盘控制器的操作
		do_hd_request();                                                 //254/ 如果复位标志是置位的，则进行硬盘控制器的复位，然后返回；如果重新校正标志是置位的，则向硬盘控制器发送驱动器重新校正(WIN_RESTORE)命令块，然后返回；如果复位标志和重新校正标志都没置位，则执行硬盘当前请求项的读写操作
		return;                                                          //255/ 返回
	}                                                                        //256/ 
	port_read(HD_DATA,CURRENT->buffer,256);                                  //257/ 从硬盘控制器中的数据寄存器的端口取数据写入当前请求项指定的高速缓冲块中，共写256字(1字占2个字节)=512字节
	CURRENT->errors = 0;                                                     //258/ 清当前请求项的出错次数
	CURRENT->buffer += 512;                                                  //259/ 将指向当前请求项指定的高速缓冲块的指针后移512字节
	CURRENT->sector++;                                                       //260/ 将当前请求项的起始物理扇区号(从磁盘/分区开头算起)加1
	if (--CURRENT->nr_sectors) {                                             //261/ 将当前请求项指定的要读的扇区数递减1，如果还大于0，说明还没有读完
		do_hd = &read_intr;                                              //262/ 则置硬盘调用C函数指针为read_intr()
		return;                                                          //263/ 返回
	}                                                                        //264/ 
	end_request(1);                                                          //265/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值1，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
	do_hd_request();                                                         //266/ 如果复位标志是置位的，则进行硬盘控制器的复位，然后返回；如果重新校正标志是置位的，则向硬盘控制器发送驱动器重新校正(WIN_RESTORE)命令块，然后返回；如果复位标志和重新校正标志都没置位，则执行硬盘当前请求项的读写操作
}                                                                                //267/ 
                                                                                 //268/ 
static void write_intr(void)                                                     //269/ [b;]写扇区中断调用函数，在硬盘写命令结束时引发的硬盘中断过程中被调用
{                                                                                //270/ 
	if (win_result()) {                                                      //271/ 检测硬盘命令执行后的状态，返回0表示正常；返回1表示出错(win表示温切斯特硬盘的缩写)
		bad_rw_intr();                                                   //272/ 如果读/写一个扇区时产生的错误次数大于等于MAX_ERRORS(=7，允许的最多出错次数)，则结束当前请求处理；接着判断产生的错误次数是否超过了3次，如果超过了，则置位复位标志，要求执行复位硬盘控制器的操作
		do_hd_request();                                                 //273/ 如果复位标志是置位的，则进行硬盘控制器的复位，然后返回；如果重新校正标志是置位的，则向硬盘控制器发送驱动器重新校正(WIN_RESTORE)命令块，然后返回；如果复位标志和重新校正标志都没置位，则执行硬盘当前请求项的读写操作
		return;                                                          //274/ 返回
	}                                                                        //275/ 
	if (--CURRENT->nr_sectors) {                                             //276/ 将当前请求项指定的要写的扇区数递减1，如果还大于0，说明还没有写完
		CURRENT->sector++;                                               //277/ 将当前请求项的起始物理扇区号(从磁盘/分区开头算起)加1
		CURRENT->buffer += 512;                                          //278/ 将指向当前请求项指定的高速缓冲块的指针后移512字节
		do_hd = &write_intr;                                             //279/ 则置硬盘调用C函数指针为write_intr()
		port_write(HD_DATA,CURRENT->buffer,256);                         //280/ 从当前请求项指定的高速缓冲块中取数据写入硬盘控制器中的数据寄存器的端口，共写256字(1字占2个字节)=512字节
		return;                                                          //281/ 返回
	}                                                                        //282/ 
	end_request(1);                                                          //283/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值1，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
	do_hd_request();                                                         //284/ 如果复位标志是置位的，则进行硬盘控制器的复位，然后返回；如果重新校正标志是置位的，则向硬盘控制器发送驱动器重新校正(WIN_RESTORE)命令块，然后返回；如果复位标志和重新校正标志都没置位，则执行硬盘当前请求项的读写操作
}                                                                                //285/ 
                                                                                 //286/ 
static void recal_intr(void)                                                     //287/ [b;]硬盘控制器复位及硬盘驱动器重新校正中断调用函数
{                                                                                //288/ 
	if (win_result())                                                        //289/ 检测硬盘命令执行后的状态，返回0表示正常；返回1表示出错(win表示温切斯特硬盘的缩写)
		bad_rw_intr();                                                   //290/ 如果读/写一个扇区时产生的错误次数大于等于MAX_ERRORS(=7，允许的最多出错次数)，则结束当前请求处理；接着判断产生的错误次数是否超过了3次，如果超过了，则置位复位标志，要求执行复位硬盘控制器的操作
	do_hd_request();                                                         //291/ 如果复位标志是置位的，则进行硬盘控制器的复位，然后返回；如果重新校正标志是置位的，则向硬盘控制器发送驱动器重新校正(WIN_RESTORE)命令块，然后返回；如果复位标志和重新校正标志都没置位，则执行硬盘当前请求项的读写操作
}                                                                                //292/ 
                                                                                 //293/ 
void do_hd_request(void)                                                         //294/ [b;]如果复位标志是置位的，则进行硬盘控制器的复位，然后返回；如果重新校正标志是置位的，则向硬盘控制器发送驱动器重新校正(WIN_RESTORE)命令块，然后返回；如果复位标志和重新校正标志都没置位，则执行硬盘当前请求项的读写操作
{                                                                                //295/ 
	int i,r;                                                                 //296/ 
	unsigned int block,dev;                                                  //297/ 
	unsigned int sec,head,cyl;                                               //298/ 
	unsigned int nsect;                                                      //299/ 
                                                                                 //300/ 
	INIT_REQUEST;                                                            //301/ 初始化请求项宏——判断当前请求项合法性，若已没有请求项则退出，若当前请求项不合法则死机
	dev = MINOR(CURRENT->dev);                                               //302/ 将当前请求项对应设备的次设备号赋给dev
	block = CURRENT->sector;                                                 //303/ 将当前请求项要读/写的设备的起始物理扇区号(从磁盘/分区开头算起)赋给block
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {                      //304/ 如果次设备号dev或者起始物理扇区号block超出了有效范围 
		end_request(0);                                                  //305/ 结束当前请求处理，如果当前请求项指定的缓冲头结构不为空，则设置该缓冲块的数据更新标志为参数值0，并解锁指定的缓冲区(块)，并唤醒等待该缓冲块的进程；唤醒CURRENT->waiting指向的任务，唤醒wait_for_request指向的任务，释放当前请求项，从请求链表中删除该请求项，并且使当前请求项指针指向下一个请求项
		goto repeat;                                                     //306/ 跳转回函数开头INIT_REQUEST中的起始repeat处
	}                                                                        //307/ 
	block += hd[dev].start_sect;                                             //308/ 将从磁盘/分区开头算起的起始物理扇区号 加上 该分区相对于该磁盘或分区在硬盘中从硬盘0道开始算起的的起始物理(绝对)扇区号 得到的当前请求项要读/写的设备相对于硬盘开头的绝对起始物理扇区号赋给block
	dev /= 5;                                                                //309/ 计算出硬盘号(0或1)赋给dev
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),           //310/ 310-313行利用硬盘号dev和绝对起始物理扇区号block计算出绝对起始物理扇区的总磁道数(赋给block)、当前磁道内扇区号(赋给sec)、当前柱面号(赋给cyl)、当前磁头号(赋给head)
		"r" (hd_info[dev].sect));                                        //311/ 
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),            //312/ 
		"r" (hd_info[dev].head));                                        //313/ 
	sec++;                                                                   //314/ 对绝对起始物理扇区号block对应的当前磁道内扇区号进行修整，因为磁盘上实际扇区计数是从1算起的
	nsect = CURRENT->nr_sectors;                                             //315/ 将当前请求项预读/写的扇区数赋给nsect
	if (reset) {                                                             //316/ 如果复位标志是置位的
		reset = 0;                                                       //317/ 将复位标志复位
		recalibrate = 1;                                                 //318/ 置位重新校正标志。当设置了该标志，程序中会调用recal_intr()将磁头移动到0柱面
		reset_hd(CURRENT_DEV);                                           //319/ 复位硬盘驱动器号CURRENT_DEV(当前请求项CURRENT中的设备号对应的硬盘(驱动器)号-(0或1))指定的硬盘控制器，并向硬盘控制器发送“建立驱动器参数”命令块
		return;                                                          //320/ 返回
	}                                                                        //321/ 
	if (recalibrate) {                                                       //322/ 如果重新校正标志是置位的
		recalibrate = 0;                                                 //323/ 复位重新校正标志
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,                      //324/ 向硬盘控制器发送驱动器重新校正(WIN_RESTORE)命令块，执行寻道操作，让处于任何地方的磁头移动到0柱面
			WIN_RESTORE,&recal_intr);                                //325/ 
		return;                                                          //326/ 返回
	}	                                                                 //327/ 
	if (CURRENT->cmd == WRITE) {                                             //328/ 如果当前请求是写扇区操作
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);            //329/ 则发送写(WIN_WRITE)命令块
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)        //330/ 循环读取硬盘控制器中的主状态寄存器的值，判断其中的数据请求服务标志是否置位，一旦置位，表示驱动器已经准备好在主机和数据端口之间传输数据，则跳出for循环
			/* nothing */ ;                                          //331/ 
		if (!r) {                                                        //332/ 若等到上面的for循环结束，数据请求服务标志都还没有置位
			bad_rw_intr();                                           //333/ 如果读/写一个扇区时产生的错误次数大于等于MAX_ERRORS(=7，允许的最多出错次数)，则结束当前请求处理；接着判断产生的错误次数是否超过了3次，如果超过了，则置位复位标志，要求执行复位硬盘控制器的操作
			goto repeat;                                             //334/ 跳转回函数开头INIT_REQUEST中的起始repeat处
		}                                                                //335/ 
		port_write(HD_DATA,CURRENT->buffer,256);                         //336/ 从当前请求项指定的高速缓冲块中取数据写入硬盘控制器中的数据寄存器的端口，共写256字(1字占2个字节)=512字节
	} else if (CURRENT->cmd == READ) {                                       //337/ 如果当前请求是读扇区操作
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);              //338/ 则发送读(WIN_READ)命令块
	} else                                                                   //339/ 否则
		panic("unknown hd-command");                                     //340/ 直接死机
}                                                                                //341/ 
                                                                                 //342/ 
void hd_init(void)                                                               //343/ [b;]硬盘系统初始化
{                                                                                //344/ 
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;                           //345/ 找到块设备表(数组)中硬盘对应的项，将该项中的请求操作的函数指针字段设置为DEVICE_REQUEST(即do_hd_request)
	set_intr_gate(0x2E,&hd_interrupt);                                       //346/ 在idt表中的第0x2E=46(从0算起)个描述符的位置放置一个中断门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为&hd_interrupt;P=1;DPL=0;TYPE=14->1110)
	outb_p(inb_p(0x21)&0xfb,0x21);                                           //347/ 复位8259主片的IR2屏蔽位，使得8259从片的中断可以传递到系统
	outb(inb_p(0xA1)&0xbf,0xA1);                                             //348/ 复位8259从片的IR6(IRQ14)屏蔽位，使得硬盘中断可以传递到系统
}                                                                                //349/ 

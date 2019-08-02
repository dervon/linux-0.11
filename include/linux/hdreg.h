/*                                                                               // 1/ 
 * This file contains some defines for the AT-hd-controller.                     // 2/ 
 * Various sources. Check out some definitions (see comments with                // 3/ 
 * a ques).                                                                      // 4/ 
 */                                                                              // 5/ 
#ifndef _HDREG_H                                                                 // 6/ 
#define _HDREG_H                                                                 // 7/ 
                                                                                 // 8/ 
/* Hd controller regs. Ref: IBM AT Bios-listing */                               // 9/ 
#define HD_DATA		0x1f0	/* _CTL when writing */                          //10/ 硬盘控制器中的数据寄存器的端口---扇区数据
#define HD_ERROR	0x1f1	/* see err-bits */                               //11/ 硬盘控制器中的错误寄存器的端口---错误状态
#define HD_NSECTOR	0x1f2	/* nr of sectors to read/write */                //12/ 硬盘控制器中的扇区数寄存器的端口---扇区数
#define HD_SECTOR	0x1f3	/* starting sector */                            //13/ 硬盘控制器中的扇区号寄存器的端口---当前磁道内起始扇区号
#define HD_LCYL		0x1f4	/* starting cylinder */                          //14/ 硬盘控制器中的柱面号寄存器的端口---柱面号低字节
#define HD_HCYL		0x1f5	/* high byte of starting cyl */                  //15/ 硬盘控制器中的柱面号寄存器的端口---柱面号高字节
#define HD_CURRENT	0x1f6	/* 101dhhhh , d=drive, hhhh=head */              //16/ 硬盘控制器中的驱动器/磁头寄存器的端口---驱动器号/磁头号(101dhhhh，d=驱动器号，hhhh=磁头号)
#define HD_STATUS	0x1f7	/* see status-bits */                            //17/ 硬盘控制器中的主状态寄存器的端口
#define HD_PRECOMP HD_ERROR	/* same io address, read=error, write=precomp */ //18/ 硬盘控制器中的写前预补偿寄存器的端口。读-错误寄存器；写-写前预补偿寄存器
#define HD_COMMAND HD_STATUS	/* same io address, read=status, write=cmd */    //19/ 硬盘控制器中的命令寄存器的端口。读-主状态寄存器；写-命令寄存器
                                                                                 //20/ 
#define HD_CMD		0x3f6                                                    //21/ 硬盘控制器中的硬盘控制寄存器的端口
                                                                                 //22/ 
/* Bits of HD_STATUS */                                                          //23/ 硬盘控制器中的主状态寄存器的各个标志的偏移位
#define ERR_STAT	0x01                                                     //24/ 命令执行错误标志
#define INDEX_STAT	0x02                                                     //25/ 收到索引标志
#define ECC_STAT	0x04	/* Corrected error */                            //26/ ECC校验错标志
#define DRQ_STAT	0x08                                                     //27/ 数据请求服务标志
#define SEEK_STAT	0x10                                                     //28/ 驱动器寻道结束标志
#define WRERR_STAT	0x20                                                     //29/ 驱动器故障(写出错)标志
#define READY_STAT	0x40                                                     //30/ 驱动器准备好(就绪)标志
#define BUSY_STAT	0x80                                                     //31/ 控制器忙碌标志
                                                                                 //32/ 
/* Values for HD_COMMAND */                                                      //33/ 硬盘控制器中的命令寄存器可存放的几种命令字节
#define WIN_RESTORE		0x10                                             //34/ 命令---驱动器重新校正
#define WIN_READ		0x20                                             //35/ 命令---读扇区
#define WIN_WRITE		0x30                                             //36/ 命令---写扇区
#define WIN_VERIFY		0x40                                             //37/ 命令---扇区检验
#define WIN_FORMAT		0x50                                             //38/ 命令---格式化磁道
#define WIN_INIT		0x60                                             //39/ 命令---控制器初始化
#define WIN_SEEK 		0x70                                             //40/ 命令---寻道
#define WIN_DIAGNOSE		0x90                                             //41/ 命令---控制器诊断
#define WIN_SPECIFY		0x91                                             //42/ 命令---建立驱动器参数
                                                                                 //43/ 
/* Bits for HD_ERROR */                                                          //44/ 硬盘控制器中的错误寄存器中的几种错误类型
#define MARK_ERR	0x01	/* Bad address mark ? */                         //45/ 诊断命令时--无错误；    其他命令时--数据标志丢失
#define TRK0_ERR	0x02	/* couldn't find track 0 */                      //46/ 诊断命令时--控制器出错； 其他命令时--磁道0错
#define ABRT_ERR	0x04	/* ? */                                          //47/ 诊断命令时--ECC部件错； 其他命令时--命令放弃
#define ID_ERR		0x10	/* ? */                                          //48/ 诊断命令时--；         其他命令时--ID未找到
#define ECC_ERR		0x40	/* ? */                                          //49/ 诊断命令时--；         其他命令时--ECC错误
#define	BBD_ERR		0x80	/* ? */                                          //50/ 诊断命令时--；         其他命令时--坏扇区
                                                                                 //51/ 
struct partition {                                                               //52/ 硬盘分区表中的分区结构
	unsigned char boot_ind;		/* 0x80 - active (unused) */             //53/ 引导标志。4个分区中同时只能有一个分区是可引导的。0x00-不从该分区引导；0x80-从该分区引导操作系统
	unsigned char head;		/* ? */                                  //54/ 分区起始磁头号
	unsigned char sector;		/* ? */                                  //55/ 分区(当前磁道内)起始扇区号(位0-5)和起始柱面号高2位(位6-7)
	unsigned char cyl;		/* ? */                                  //56/ 分区起始柱面号低8位
	unsigned char sys_ind;		/* ? */                                  //57/ 分区类型字节。0x0b-DOS；0x80-Old Minix；0x83-Linux...
	unsigned char end_head;		/* ? */                                  //58/ 分区的结束磁头号
	unsigned char end_sector;	/* ? */                                  //59/ 分区(当前磁道内)结束扇区号(位0-5)和结束柱面号高2位(位6-7)
	unsigned char end_cyl;		/* ? */                                  //60/ 分区结束柱面号低8位
	unsigned int start_sect;	/* starting sector counting from 0 */    //61/ 分区起始物理扇区号
	unsigned int nr_sects;		/* nr of sectors in partition */         //62/ 分区占用的扇区数
};                                                                               //63/ 
                                                                                 //64/ 
#endif                                                                           //65/ 

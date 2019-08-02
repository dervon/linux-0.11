/*                                                                             // 1/ 
 * This file contains some defines for the floppy disk controller.             // 2/ 
 * Various sources. Mostly "IBM Microcomputers: A Programmers                  // 3/ 
 * Handbook", Sanches and Canton.                                              // 4/ 
 */                                                                            // 5/ 
#ifndef _FDREG_H                                                               // 6/ 
#define _FDREG_H                                                               // 7/ 
                                                                               // 8/ 
extern int ticks_to_floppy_on(unsigned int nr);                                // 9/ 
extern void floppy_on(unsigned int nr);                                        //10/ 
extern void floppy_off(unsigned int nr);                                       //11/ 
extern void floppy_select(unsigned int nr);                                    //12/ 
extern void floppy_deselect(unsigned int nr);                                  //13/ 
                                                                               //14/ 
/* Fd controller regs. S&C, about page 340 */                                  //15/ 
#define FD_STATUS	0x3f4                                                  //16/ 软盘控制器中主状态寄存器的端口(位7:软盘控制器中数据寄存器就绪位，1-已就绪，0-未就绪；位6:传输方向，1-软盘控制器->CPU，0-CPU->软盘控制器；位5:非DMA方式位，1-非DMA方式，0-DMA方式；位4:控制器忙位，1-忙，0-不忙；位3-0:分别用于指示控制的软驱D-A忙位，1-忙，0-不忙)
#define FD_DATA		0x3f5                                                  //17/ 软盘控制器中数据寄存器的端口
#define FD_DOR		0x3f2		/* Digital Output Register */          //18/ 软盘控制器中数字输出寄存器的端口(位7-4:分别控制软驱D-A马达的启动，1-启动，0-关闭；位3:1-允许DMA和中断请求，0-禁止DMA和中断请求；位2:1-启动软盘控制器，0-复位软盘控制器；位1-0:00-11，用于选择控制的软驱A-D)
#define FD_DIR		0x3f7		/* Digital Input Register (read) */    //19/ 软盘控制器中数字输入寄存器的端口(位7:对软盘有效，表示盘片更换状态，1-已更换，0-未更换；位6-0:用于硬盘控制器接口)
#define FD_DCR		0x3f7		/* Diskette Control Register (write)*/ //20/ 软盘控制器中磁盘控制寄存器的端口(位7-2:不使用；位1-0:用于选择盘片在不同类型驱动器上使用的数据传输率，00-500kb/s，01-300kb/s，11-250kb/s)
                                                                               //21/ 
/* Bits of main status register */                                             //22/ 23-27行为软盘控制器主状态寄存器的各个偏移位宏
#define STATUS_BUSYMASK	0x0F		/* drive busy mask */                  //23/ 驱动器A-D忙
#define STATUS_BUSY	0x10		/* FDC busy */                         //24/ 软盘控制器忙
#define STATUS_DMA	0x20		/* 0- DMA mode */                      //25/ 非DMA方式
#define STATUS_DIR	0x40		/* 0- cpu->fdc */                      //26/ 传输方向：软盘控制器->CPU
#define STATUS_READY	0x80		/* Data reg ready */                   //27/ 软盘控制器的数据寄存器准备就绪
                                                                               //28/ 
/* Bits of FD_ST0 */                                                           //29/ 30-35行为软盘控制器状态字节0的各个偏移位宏
#define ST0_DS		0x03		/* drive select mask */                //30/ 驱动器选择号(中断时的驱动器号)
#define ST0_HA		0x04		/* Head (Address) */                   //31/ 磁头地址(中断时磁头号)
#define ST0_NR		0x08		/* Not Ready */                        //32/ 软驱未就绪
#define ST0_ECE		0x10		/* Equipment chech error */            //33/ 设备检查出错
#define ST0_SE		0x20		/* Seek end */                         //34/ 寻道操作或重新校正操作结束
#define ST0_INTR	0xC0		/* Interrupt code mask */              //35/ 中断原因
                                                                               //36/ 
/* Bits of FD_ST1 */                                                           //37/ 38-43行为软盘控制器状态字节1的各个偏移位宏
#define ST1_MAM		0x01		/* Missing Address Mark */             //38/ 未找到扇区地址标志
#define ST1_WP		0x02		/* Write Protect */                    //39/ 写保护
#define ST1_ND		0x04		/* No Data - unreadable */             //40/ 为找到指定的扇区
#define ST1_OR		0x10		/* OverRun */                          //41/ 数据传输超时
#define ST1_CRC		0x20		/* CRC error in data or addr */        //42/ CRC校验出错
#define ST1_EOC		0x80		/* End Of Cylinder */                  //43/ 访问超过磁道上最大扇区号
                                                                               //44/ 
/* Bits of FD_ST2 */                                                           //45/ 46-52行为软盘控制器状态字节2的各个偏移位宏
#define ST2_MAM		0x01		/* Missing Addess Mark (again) */      //46/ 未找到扇区数据标志
#define ST2_BC		0x02		/* Bad Cylinder */                     //47/ 扇区ID信息的磁道号C=0xFF，磁道坏
#define ST2_SNS		0x04		/* Scan Not Satisfied */               //48/ 检索(扫描)条件不满足要求
#define ST2_SEH		0x08		/* Scan Equal Hit */                   //49/ 检索(扫描)条件满足要求
#define ST2_WC		0x10		/* Wrong Cylinder */                   //50/ 扇区ID信息的磁道号C不符
#define ST2_CRC		0x20		/* CRC error in data field */          //51/ 扇区数据场CRC校验出错
#define ST2_CM		0x40		/* Control Mark = deleted */           //52/ SK=0时，读数据遇到删除标志
                                                                               //53/ 
/* Bits of FD_ST3 */                                                           //54/ 
#define ST3_HA		0x04		/* Head (Address) */                   //55/ 
#define ST3_TZ		0x10		/* Track Zero signal (1=track 0) */    //56/ 
#define ST3_WP		0x40		/* Write Protect */                    //57/ 
                                                                               //58/ 
/* Values for FD_COMMAND */                                                    //59/ 60-65定义了Linux0.11中使用的软盘控制器的几种命令
#define FD_RECALIBRATE	0x07		/* move to track 0 */                  //60/ 重新校正命令
#define FD_SEEK		0x0F		/* seek track */                       //61/ 磁头寻道命令
#define FD_READ		0xE6		/* read with MT, MFM, SKip deleted */  //62/ 读扇区数据命令
#define FD_WRITE	0xC5		/* write with MT, MFM */               //63/ 写扇区数据命令
#define FD_SENSEI	0x08		/* Sense Interrupt Status */           //64/ 检测中断状态命令
#define FD_SPECIFY	0x03		/* specify HUT etc */                  //65/ 设定驱动器参数命令
                                                                               //66/ 
/* DMA commands */                                                             //67/ 
#define DMA_READ	0x46                                                   //68/ 用于写入DMA方式寄存器的值(请求模式、地址递增、自动预置、DMA读传输、选择DMA通道2)
#define DMA_WRITE	0x4A                                                   //69/ 用于写入DMA方式寄存器的值(请求模式、地址递增、自动预置、DMA写传输、选择DMA通道2)
                                                                               //70/ 
#endif                                                                         //71/ 

/*                                                                         // 1/ 
 *  linux/kernel/serial.c                                                  // 2/ 
 *                                                                         // 3/ 
 *  (C) 1991  Linus Torvalds                                               // 4/ 
 */                                                                        // 5/ 
                                                                           // 6/ 
/*                                                                         // 7/ 
 *	serial.c                                                           // 8/ 
 *                                                                         // 9/ 
 * This module implements the rs232 io functions                           //10/ 
 *	void rs_write(struct tty_struct * queue);                          //11/ 
 *	void rs_init(void);                                                //12/ 
 * and all interrupts pertaining to serial IO.                             //13/ 
 */                                                                        //14/ 
                                                                           //15/ 
#include <linux/tty.h>                                                     //16/ 
#include <linux/sched.h>                                                   //17/ 
#include <asm/system.h>                                                    //18/ 
#include <asm/io.h>                                                        //19/ 
                                                                           //20/ 
#define WAKEUP_CHARS (TTY_BUF_SIZE/4)                                      //21/ 
                                                                           //22/ 
extern void rs1_interrupt(void);                                           //23/ 串行口1的中断处理程序(rs_io.s中的_rs1_interrupt)
extern void rs2_interrupt(void);                                           //24/ 串行口2的中断处理程序(rs_io.s中的_rs2_interrupt)
                                                                           //25/ 
static void init(int port)                                                 //26/ [b;]通过串口的端口基地址port(串口1：0x3f8；串口2：0x2f8)初始化串行端口——设置波特率因子0x30、8位数据位、1位停止位、无校验位、允许modem状态改变中断、允许接收器线路状态有错中断、允许已接收到数据中断
{                                                                          //27/ 
	outb_p(0x80,port+3);	/* set DLAB of line control reg */         //28/ 置位写线路控制寄存器的DLAB位
	outb_p(0x30,port);	/* LS of divisor (48 -> 2400 bps */        //29/ 29-30行用于将波特率因子0x0030写入对应的端口
	outb_p(0x00,port+1);	/* MS of divisor */                        //30/ 
	outb_p(0x03,port+3);	/* reset DLAB */                           //31/ 复位写线路控制寄存器的DLAB位，设置数据位为8位、无校验位、1位停止位
	outb_p(0x0b,port+4);	/* set DTR,RTS, OUT_2 */                   //32/ 使数据终端就绪DTR有效；使请求发送RTS有效；辅助用户指定输出2，允许INTRPT到系统
	outb_p(0x0d,port+1);	/* enable all intrs but writes */          //33/ 允许——modem状态改变中断；接收器线路状态有错中断；已接收到数据中断 禁止——发送保持寄存器空中断
	(void)inb(port);	/* read data port to reset things (?) */   //34/ 读数据口，以进行复位操作
}                                                                          //35/ 
                                                                           //36/ 
void rs_init(void)                                                         //37/ [b;]串口1和串口2的初始化
{                                                                          //38/ 
	set_intr_gate(0x24,rs1_interrupt);                                 //39/ 在idt表中的第0x24=36(从0算起)个描述符的位置放置一个中断门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为rs1_interrupt;P=1;DPL=0;TYPE=14->1110)
	set_intr_gate(0x23,rs2_interrupt);                                 //40/ 在idt表中的第0x23=35(从0算起)个描述符的位置放置一个中断门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为rs2_interrupt;P=1;DPL=0;TYPE=14->1110)
	init(tty_table[1].read_q.data);                                    //41/ 通过串口1的端口基地址tty_table[1].read_q.data初始化串行端口——设置波特率因子0x30、8位数据位、1位停止位、无校验位、允许modem状态改变中断、允许接收器线路状态有错中断、允许已接收到数据中断
	init(tty_table[2].read_q.data);                                    //42/ 通过串口2的端口基地址tty_table[2].read_q.data初始化串行端口——设置波特率因子0x30、8位数据位、1位停止位、无校验位、允许modem状态改变中断、允许接收器线路状态有错中断、允许已接收到数据中断
	outb(inb_p(0x21)&0xE7,0x21);                                       //43/ 复位8259主片的IR3、IR4屏蔽位，使得串口1和串口2的中断可以传递到系统
}                                                                          //44/ 
                                                                           //45/ 
/*                                                                         //46/ 
 * This routine gets called when tty_write has put something into          //47/ 
 * the write_queue. It must check wheter the queue is empty, and           //48/ 
 * set the interrupt register accordingly                                  //49/ 
 *                                                                         //50/ 
 *	void _rs_write(struct tty_struct * tty);                           //51/ 
 */                                                                        //52/ 
void rs_write(struct tty_struct * tty)                                     //53/ [b;]如果参数tty指定的串口tty结构的tty写队列的队列缓冲区不为空，则开启参数tty指定的串口tty结构的发送保持寄存器空中断允许
{                                                                          //54/ 
	cli();                                                             //55/ 关中断
	if (!EMPTY(tty->write_q))                                          //56/ 如果参数tty指定的串口tty结构的tty写队列的队列缓冲区不为空
		outb(inb_p(tty->write_q.data+1)|0x02,tty->write_q.data+1); //57/ 则开启参数tty指定的串口tty结构的发送保持寄存器空中断允许
	sti();                                                             //58/ 开中断
}                                                                          //59/ 

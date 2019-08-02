/*                                                                             // 1/ 
 * 'tty.h' defines some structures used by tty_io.c and some defines.          // 2/ 
 *                                                                             // 3/ 
 * NOTE! Don't touch this without checking that nothing in rs_io.s or          // 4/ 
 * con_io.s breaks. Some constants are hardwired into the system (mainly       // 5/ 
 * offsets into 'tty_queue'                                                    // 6/ 
 */                                                                            // 7/ 
                                                                               // 8/ 
#ifndef _TTY_H                                                                 // 9/ 
#define _TTY_H                                                                 //10/ 
                                                                               //11/ 
#include <termios.h>                                                           //12/ 
                                                                               //13/ 
#define TTY_BUF_SIZE 1024                                                      //14/ 
                                                                               //15/ 
struct tty_queue {                                                             //16/ tty缓冲队列结构
	unsigned long data;                                                    //17/ 队列缓冲区中含有字符行数值(不是当前字符数)，对于串口终端，则存放串行端口基地址(串口1：0x3f8；串口2：0x2f8)
	unsigned long head;                                                    //18/ 缓冲区中数据头偏移量，即数据头相对于队列缓冲区开始位置的偏移量
	unsigned long tail;                                                    //19/ 缓冲区中数据尾偏移量，即数据尾相对于队列缓冲区开始位置的偏移量
	struct task_struct * proc_list;                                        //20/ 等待进程列表指针
	char buf[TTY_BUF_SIZE];                                                //21/ 队列缓冲区
};                                                                             //22/ 
                                                                               //23/ 
#define INC(a) ((a) = ((a)+1) & (TTY_BUF_SIZE-1))                              //24/ 对数据头偏移量a或数据尾偏移量a的值进行增1调整
#define DEC(a) ((a) = ((a)-1) & (TTY_BUF_SIZE-1))                              //25/ 对数据头偏移量a或数据尾偏移量a的值进行减1调整
#define EMPTY(a) ((a).head == (a).tail)                                        //26/ 判断tty缓冲队列a的队列缓冲区是否为空，若为空则返回1，不为空返回0
#define LEFT(a) (((a).tail-(a).head-1)&(TTY_BUF_SIZE-1))                       //27/ 返回tty缓冲队列a的队列缓冲区中还可存放字符的剩余长度(字节数)
#define LAST(a) ((a).buf[(TTY_BUF_SIZE-1)&((a).head-1)])                       //28/ 返回tty缓冲队列a的队列缓冲区中剩余空间的末尾处字符(即数据头偏移量指定位置之前的一个字符)
#define FULL(a) (!LEFT(a))                                                     //29/ 判断tty缓冲队列a的队列缓冲区是否已满，若已满则返回1，未满返回0
#define CHARS(a) (((a).head-(a).tail)&(TTY_BUF_SIZE-1))                        //30/ 返回tty缓冲队列a的队列缓冲区中已存放字符的长度(字节数)
#define GETCH(queue,c) \                                                       //31/ 从tty缓冲队列queue的队列缓冲区中数据尾偏移量指定的位置取出一个字符放入c中，调整数据尾偏移量的值
(void)({c=(queue).buf[(queue).tail];INC((queue).tail);})                       //32/ 
#define PUTCH(c,queue) \                                                       //33/ 将字符c放入tty缓冲队列queue的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
(void)({(queue).buf[(queue).head]=(c);INC((queue).head);})                     //34/ 
                                                                               //35/ 
#define INTR_CHAR(tty) ((tty)->termios.c_cc[VINTR])                            //36/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的中断字符
#define QUIT_CHAR(tty) ((tty)->termios.c_cc[VQUIT])                            //37/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的退出字符
#define ERASE_CHAR(tty) ((tty)->termios.c_cc[VERASE])                          //38/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的擦除字符
#define KILL_CHAR(tty) ((tty)->termios.c_cc[VKILL])                            //39/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的终止字符(删除行)
#define EOF_CHAR(tty) ((tty)->termios.c_cc[VEOF])                              //40/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的文件结束字符
#define START_CHAR(tty) ((tty)->termios.c_cc[VSTART])                          //41/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的开始字符
#define STOP_CHAR(tty) ((tty)->termios.c_cc[VSTOP])                            //42/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的停止字符
#define SUSPEND_CHAR(tty) ((tty)->termios.c_cc[VSUSP])                         //43/ 取出tty指定的tty结构中termios结构中的控制字符数组c_cc中的挂起字符
                                                                               //44/ 
struct tty_struct {                                                            //45/ tty结构(每项对应系统中的一个终端设备)
	struct termios termios;                                                //46/ 终端设备的IO属性和控制字符数据结构
	int pgrp;                                                              //47/ 所属进程组号
	int stopped;                                                           //48/ 停止标志，表示对应的终端设备是否已经停止使用
	void (*write)(struct tty_struct * tty);                                //49/ tty写函数指针，表示对应的终端设备的输出处理函数
	struct tty_queue read_q;                                               //50/ tty读队列，用于临时存放从键盘或串行终端输入的原始字符序列
	struct tty_queue write_q;                                              //51/ tty写队列，用于存放写到控制台显示屏或串行终端去的数据
	struct tty_queue secondary;                                            //52/ tty辅助队列(存放规范模式字符序列，可称为规范(熟)模式队列)，根据ICANON标志，存放从read_q中取出的经过行规则程序处理过的数据，或称为熟模式数据
	};                                                                     //53/ 
                                                                               //54/ 
extern struct tty_struct tty_table[];                                          //55/ tty结构数组
                                                                               //56/ 
/*	intr=^C		quit=^|		erase=del	kill=^U                //57/ 
	eof=^D		vtime=\0	vmin=\1		sxtc=\0                //58/ 
	start=^Q	stop=^S		susp=^Z		eol=\0                 //59/ 
	reprint=^R	discard=^U	werase=^W	lnext=^V               //60/ 
	eol2=\0                                                                //61/ 
*/                                                                             //62/ 
#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0" //63/ 17个控制字符的ASCII码
                                                                               //64/ 
void rs_init(void);                                                            //65/ 
void con_init(void);                                                           //66/ 
void tty_init(void);                                                           //67/ 
                                                                               //68/ 
int tty_read(unsigned c, char * buf, int n);                                   //69/ 
int tty_write(unsigned c, char * buf, int n);                                  //70/ 
                                                                               //71/ 
void rs_write(struct tty_struct * tty);                                        //72/ 
void con_write(struct tty_struct * tty);                                       //73/ 
                                                                               //74/ 
void copy_to_cooked(struct tty_struct * tty);                                  //75/ 
                                                                               //76/ 
#endif                                                                         //77/ 

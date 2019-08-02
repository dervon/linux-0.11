/*                                                                               //  1/ 
 *  linux/kernel/tty_io.c                                                        //  2/ 
 *                                                                               //  3/ 
 *  (C) 1991  Linus Torvalds                                                     //  4/ 
 */                                                                              //  5/ 
                                                                                 //  6/ 
/*                                                                               //  7/ 
 * 'tty_io.c' gives an orthogonal feeling to tty's, be they consoles             //  8/ 
 * or rs-channels. It also implements echoing, cooked mode etc.                  //  9/ 
 *                                                                               // 10/ 
 * Kill-line thanks to John T Kohl.                                              // 11/ 
 */                                                                              // 12/ 
#include <ctype.h>                                                               // 13/ 
#include <errno.h>                                                               // 14/ 
#include <signal.h>                                                              // 15/ 
                                                                                 // 16/ 
#define ALRMMASK (1<<(SIGALRM-1))                                                // 17/ 警告(alarm)信号屏蔽位
#define KILLMASK (1<<(SIGKILL-1))                                                // 18/ 终止(kill)信号屏蔽位
#define INTMASK (1<<(SIGINT-1))                                                  // 19/ 键盘中断(int)信号屏蔽位
#define QUITMASK (1<<(SIGQUIT-1))                                                // 20/ 键盘退出(quit)信号屏蔽位
#define TSTPMASK (1<<(SIGTSTP-1))                                                // 21/ tty发出的停止进程(tty stop)信号屏蔽位
                                                                                 // 22/ 
#include <linux/sched.h>                                                         // 23/ 
#include <linux/tty.h>                                                           // 24/ 
#include <asm/segment.h>                                                         // 25/ 
#include <asm/system.h>                                                          // 26/ 
                                                                                 // 27/ 
#define _L_FLAG(tty,f)	((tty)->termios.c_lflag & f)                             // 28/ 判断tty指定的tty结构中的termios结构中的本地模式标志中f指定的标志是否置位，若置位则返回1，否则返回0
#define _I_FLAG(tty,f)	((tty)->termios.c_iflag & f)                             // 29/ 判断tty指定的tty结构中的termios结构中的输入模式标志中f指定的标志是否置位，若置位则返回1，否则返回0
#define _O_FLAG(tty,f)	((tty)->termios.c_oflag & f)                             // 30/ 判断tty指定的tty结构中的termios结构中的输出模式标志中f指定的标志是否置位，若置位则返回1，否则返回0
                                                                                 // 31/ 
#define L_CANON(tty)	_L_FLAG((tty),ICANON)                                    // 32/ 判断tty指定的tty结构中的termios结构中的本地模式标志中 (开启规范模式(熟模式)) 标志ICANON是否置位，若置位则返回1，否则返回0
#define L_ISIG(tty)	_L_FLAG((tty),ISIG)                                      // 33/ 判断tty指定的tty结构中的termios结构中的本地模式标志中 (当收到字符INTR、QUIT、SUSP或DSUSP，产生相应的信号) 标志ISIG是否置位，若置位则返回1，否则返回0
#define L_ECHO(tty)	_L_FLAG((tty),ECHO)                                      // 34/ 判断tty指定的tty结构中的termios结构中的本地模式标志中 (回显输入字符) 标志ECHO是否置位，若置位则返回1，否则返回0
#define L_ECHOE(tty)	_L_FLAG((tty),ECHOE)                                     // 35/ 判断tty指定的tty结构中的termios结构中的本地模式标志中 (若设置了ICANON，则ERASE/WERASE将擦除前一字符/单词)标志ECHOE是否置位，若置位则返回1，否则返回0
#define L_ECHOK(tty)	_L_FLAG((tty),ECHOK)                                     // 36/ 判断tty指定的tty结构中的termios结构中的本地模式标志中 (若设置了ICANON，则KILL字符将擦除当前行) 标志ECHOK是否置位，若置位则返回1，否则返回0
#define L_ECHOCTL(tty)	_L_FLAG((tty),ECHOCTL)                                   // 37/ 判断tty指定的tty结构中的termios结构中的本地模式标志中 (若设置了ECHO，则除TAB、NL、START和STOP以外的ASCII控制信号将被回显成像^X式样，X值是控制符+0x40) 标志ECHOCTL是否置位，若置位则返回1，否则返回0
#define L_ECHOKE(tty)	_L_FLAG((tty),ECHOKE)                                    // 38/ 判断tty指定的tty结构中的termios结构中的本地模式标志中 (若设置了ICANON，则KILL通过擦除行上的所有字符被回显) 标志ECHOKE是否置位，若置位则返回1，否则返回0
                                                                                 // 39/ 
#define I_UCLC(tty)	_I_FLAG((tty),IUCLC)                                     // 40/ 判断tty指定的tty结构中的termios结构中的输入模式标志中 (在输入时将大写字符转换成小写字符) 标志IUCLC是否置位，若置位则返回1，否则返回0
#define I_NLCR(tty)	_I_FLAG((tty),INLCR)                                     // 41/ 判断tty指定的tty结构中的termios结构中的输入模式标志中 (输入时将换行符NL映射成回车符CR) 标志INLCR是否置位，若置位则返回1，否则返回0
#define I_CRNL(tty)	_I_FLAG((tty),ICRNL)                                     // 42/ 判断tty指定的tty结构中的termios结构中的输入模式标志中 (在输入时将回车符CR映射成换行符NL) 标志ICRNL是否置位，若置位则返回1，否则返回0
#define I_NOCR(tty)	_I_FLAG((tty),IGNCR)                                     // 43/ 判断tty指定的tty结构中的termios结构中的输入模式标志中 (忽略回车符CR) 标志IGNCR是否置位，若置位则返回1，否则返回0
                                                                                 // 44/ 
#define O_POST(tty)	_O_FLAG((tty),OPOST)                                     // 45/ 判断tty指定的tty结构中的termios结构中的输出模式标志中 (执行输出处理) 标志OPOST是否置位，若置位则返回1，否则返回0
#define O_NLCR(tty)	_O_FLAG((tty),ONLCR)                                     // 46/ 判断tty指定的tty结构中的termios结构中的输出模式标志中 (在输出时将换行符NL映射成回车-换行符CR-NL) 标志ONLCR是否置位，若置位则返回1，否则返回0
#define O_CRNL(tty)	_O_FLAG((tty),OCRNL)                                     // 47/ 判断tty指定的tty结构中的termios结构中的输出模式标志中 (在输出时将回车符CR映射成换行符NL) 标志OCRNL是否置位，若置位则返回1，否则返回0
#define O_NLRET(tty)	_O_FLAG((tty),ONLRET)                                    // 48/ 判断tty指定的tty结构中的termios结构中的输出模式标志中 (换行符NL执行回车符的功能) 标志ONLRET是否置位，若置位则返回1，否则返回0
#define O_LCUC(tty)	_O_FLAG((tty),OLCUC)                                     // 49/ 判断tty指定的tty结构中的termios结构中的输出模式标志中 (在输出时将小写字符转换成大写字符) 标志OLCUC是否置位，若置位则返回1，否则返回0
                                                                                 // 50/ 
struct tty_struct tty_table[] = {                                                // 51/ tty结构数组的初始数据，第一项对应控制台(包括键盘和显示器)，第二项对应串口1，第三项对应串口2
	{                                                                        // 52/ 
		{ICRNL,		/* change incoming CR to NL */                   // 53/ 输入模式标志——(在输入时将回车符CR映射成换行符NL)
		OPOST|ONLCR,	/* change outgoing NL to CRNL */                 // 54/ 输出模式标志——(执行输出处理)(在输出时将换行符NL映射成回车-换行符CR-NL)
		0,                                                               // 55/ 控制模式标志
		ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,                         // 56/ 本地模式标志——(当收到字符INTR、QUIT、SUSP或DSUSP，产生相应的信号)(开启规范模式(熟模式))(回显输入字符)(若设置了ECHO，则除TAB、NL、START和STOP以外的ASCII控制信号将被回显成像^X式样，X值是控制符+0x40)(若设置了ICANON，则KILL通过擦除行上的所有字符被回显)
		0,		/* console termio */                             // 57/ 线路规程
		INIT_C_CC},                                                      // 58/ 控制字符数组——17个控制字符的ASCII码
		0,			/* initial pgrp */                       // 59/ 所属进程组号
		0,			/* initial stopped */                    // 60/ 停止标志——对应的终端设备未停止使用
		con_write,                                                       // 61/ tty写函数指针——表示对应的终端设备的输出处理函数
		{0,0,0,0,""},		/* console read-queue */                 // 62/ tty读队列
		{0,0,0,0,""},		/* console write-queue */                // 63/ tty写队列
		{0,0,0,0,""}		/* console secondary queue */            // 64/ tty辅助队列
	},{                                                                      // 65/ 
		{0, /* no translation */                                         // 66/ 输入模式标志
		0,  /* no translation */                                         // 67/ 输出模式标志
		B2400 | CS8,                                                     // 68/ 控制模式标志——(波特率 2400)(每字符8比特位)
		0,                                                               // 69/ 本地模式标志
		0,                                                               // 70/ 线路规程
		INIT_C_CC},                                                      // 71/ 控制字符数组——17个控制字符的ASCII码
		0,                                                               // 72/ 所属进程组号
		0,                                                               // 73/ 停止标志——对应的终端设备未停止使用
		rs_write,                                                        // 74/ tty写函数指针——表示对应的终端设备的输出处理函数
		{0x3f8,0,0,0,""},		/* rs 1 */                       // 75/ tty读队列——0x3f8为串口1的端口基地址
		{0x3f8,0,0,0,""},                                                // 76/ tty写队列——0x3f8为串口1的端口基地址
		{0,0,0,0,""}                                                     // 77/ tty辅助队列
	},{                                                                      // 78/ 
		{0, /* no translation */                                         // 79/ 输入模式标志
		0,  /* no translation */                                         // 80/ 输出模式标志
		B2400 | CS8,                                                     // 81/ 控制模式标志——(波特率 2400)(每字符8比特位)
		0,                                                               // 82/ 本地模式标志
		0,                                                               // 83/ 线路规程
		INIT_C_CC},                                                      // 84/ 控制字符数组——17个控制字符的ASCII码
		0,                                                               // 85/ 所属进程组号
		0,                                                               // 86/ 停止标志——对应的终端设备未停止使用
		rs_write,                                                        // 87/ tty写函数指针——表示对应的终端设备的输出处理函数
		{0x2f8,0,0,0,""},		/* rs 2 */                       // 88/ tty读队列——0x2f8为串口2的端口基地址
		{0x2f8,0,0,0,""},                                                // 89/ tty写队列——0x2f8为串口2的端口基地址
		{0,0,0,0,""}                                                     // 90/ tty辅助队列
	}                                                                        // 91/ 
};                                                                               // 92/ 
                                                                                 // 93/ 
/*                                                                               // 94/ 
 * these are the tables used by the machine code handlers.                       // 95/ 
 * you can implement pseudo-tty's or something by changing                       // 96/ 
 * them. Currently not done.                                                     // 97/ 
 */                                                                              // 98/ 
struct tty_queue * table_list[]={                                                // 99/ tty读写缓冲队列结构地址表
	&tty_table[0].read_q, &tty_table[0].write_q,                             //100/ 控制台终端读写缓冲队列地址
	&tty_table[1].read_q, &tty_table[1].write_q,                             //101/ 串口1读写缓冲队列地址
	&tty_table[2].read_q, &tty_table[2].write_q                              //102/ 串口2读写缓冲队列地址
	};                                                                       //103/ 
                                                                                 //104/ 
void tty_init(void)                                                              //105/ [b;]tty终端初始化——初始化串口终端和控制台终端
{                                                                                //106/ 
	rs_init();                                                               //107/ 串口1和串口2的初始化
	con_init();                                                              //108/ 控制台的初始化
}                                                                                //109/ 
                                                                                 //110/ 
void tty_intr(struct tty_struct * tty, int mask)                                 //111/ [b;]向tty指向的tty结构中所属进程组号所指明的进程组中所有进程发送指定信号mask
{                                                                                //112/ 
	int i;                                                                   //113/ 
                                                                                 //114/ 
	if (tty->pgrp <= 0)                                                      //115/ 检查tty指向的tty结构中所属进程组号的有效性，如果小于等于0(等于0表示进程是初始进程init，没有控制终端)
		return;                                                          //116/ 则返回
	for (i=0;i<NR_TASKS;i++)                                                 //117/ 遍寻64(0-63)个任务槽
		if (task[i] && task[i]->pgrp==tty->pgrp)                         //118/ 如果槽中的任务属于tty指向的tty结构中所属进程组号所指明的进程组
			task[i]->signal |= mask;                                 //119/ 则向该任务发送信号mask
}                                                                                //120/ 
                                                                                 //121/ 
static void sleep_if_empty(struct tty_queue * queue)                             //122/ [b;]如果当前进程没有信号要处理，并且queue指向的tty缓冲队列的队列缓冲区为空，则把当前任务置为可中断的睡眠状态
{                                                                                //123/ 
	cli();                                                                   //124/ 关中断
	while (!current->signal && EMPTY(*queue))                                //125/ 如果当前进程没有接收到信号，并且queue指向的tty缓冲队列的队列缓冲区为空
		interruptible_sleep_on(&queue->proc_list);                       //126/ 则把当前任务置为可中断的睡眠状态，让当前任务的tmp指向queue->proc_list指向的旧睡眠队列头，而让queue->proc_list指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up或者中断或者收到信号唤醒了queue->proc_list指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
	sti();                                                                   //127/ 开中断
}                                                                                //128/ 
                                                                                 //129/ 
static void sleep_if_full(struct tty_queue * queue)                              //130/ [b;]如果当前进程没有信号要处理，并且queue指向的tty缓冲队列的队列缓冲区已满，则把当前任务置为可中断的睡眠状态
{                                                                                //131/ 
	if (!FULL(*queue))                                                       //132/ 如果queue指向的tty缓冲队列的队列缓冲区未满
		return;                                                          //133/ 则返回
	cli();                                                                   //134/ 关中断
	while (!current->signal && LEFT(*queue)<128)                             //135/ 如果当前进程没有接收到信号，并且queue指向的tty缓冲队列的队列缓冲区中还可存放字符的剩余长度小于128
		interruptible_sleep_on(&queue->proc_list);                       //136/ 则把当前任务置为可中断的睡眠状态，让当前任务的tmp指向queue->proc_list指向的旧睡眠队列头，而让queue->proc_list指向当前任务(即新睡眠队列头)，然后重新调度。等到wake_up或者中断或者收到信号唤醒了queue->proc_list指向的当前任务(即新睡眠队列头)，切换返回到当前任务继续执行时，再将当前任务的tmp指向的睡眠队列头置为就绪态，这样就嵌套唤醒睡眠队列上的所有任务
	sti();                                                                   //137/ 开中断
}                                                                                //138/ 
                                                                                 //139/ 
void wait_for_keypress(void)                                                     //140/ [b;]等待按键按下——如果当前进程没有信号要处理，并且控制台对应的tty辅助缓冲队列的队列缓冲区为空，则把当前任务置为可中断的睡眠状态
{                                                                                //141/ 
	sleep_if_empty(&tty_table[0].secondary);                                 //142/ 如果当前进程没有信号要处理，并且(&tty_table[0].secondary)指向的tty辅助缓冲队列的队列缓冲区为空，则把当前任务置为可中断的睡眠状态
}                                                                                //143/ 
                                                                                 //144/ 
void copy_to_cooked(struct tty_struct * tty)                                     //145/ [b;]根据tty指定的tty结构中的termios结构中设置的各种标志，将指定tty终端队列缓冲区中的字符复制转换成规范模式(熟模式)字符并存放在辅助队列(规范模式队列)中
{                                                                                //146/ 
	signed char c;                                                           //147/ 
                                                                                 //148/ 
	while (!EMPTY(tty->read_q) && !FULL(tty->secondary)) {                   //149/ 循环操作——如果tty缓冲队列(tty->read_q)的队列缓冲区不为空，并且tty缓冲队列(tty->secondary)的队列缓冲区未满
		GETCH(tty->read_q,c);                                            //150/ 则从tty缓冲队列(tty->read_q)的队列缓冲区中数据尾偏移量指定的位置取出一个字符放入c中，调整数据尾偏移量的值
		if (c==13)                                                       //151/ 如果取出的字符是回车符CR
			if (I_CRNL(tty))                                         //152/ 若tty指定的tty结构中的termios结构中的输入模式标志中 (在输入时将回车符CR映射成换行符NL) 标志ICRNL已置位
				c=10;                                            //153/ 则将回车符CR转换为换行符NL
			else if (I_NOCR(tty))                                    //154/ 若tty指定的tty结构中的termios结构中的输入模式标志中 (忽略回车符CR) 标志IGNCR已置位
				continue;                                        //155/ 则结束本次循环
			else ;                                                   //156/ 
		else if (c==10 && I_NLCR(tty))                                   //157/ 如果取出的字符是换行符NL，并且tty指定的tty结构中的termios结构中的输入模式标志中 (输入时将换行符NL映射成回车符CR) 标志INLCR已置位
			c=13;                                                    //158/ 则将换行符NL转换为回车符CR
		if (I_UCLC(tty))                                                 //159/ 如果tty指定的tty结构中的termios结构中的输入模式标志中 (在输入时将大写字符转换成小写字符) 标志IUCLC已置位
			c=tolower(c);                                            //160/ 则判断ASCII码值为c的字符是否是大写字符，如果是则转换为小写字符
		if (L_CANON(tty)) {                                              //161/ 如果tty指定的tty结构中的termios结构中的本地模式标志中 (开启规范模式(熟模式)) 标志ICANON已置位
			if (c==KILL_CHAR(tty)) {                                 //162/ 如果字符c是终止字符(删除行)
				/* deal with killing the input line */           //163/ 
				while(!(EMPTY(tty->secondary) ||                 //164/ 如果tty缓冲队列(tty->secondary)的队列缓冲区不为空
				        (c=LAST(tty->secondary))==10 ||          //165/ 或者 tty缓冲队列(tty->secondary)的队列缓冲区中剩余空间的末尾处字符是换行符NL
				        c==EOF_CHAR(tty))) {                     //166/ 或者 字符c是控制字符数组c_cc中的文件结束字符
					if (L_ECHO(tty)) {                       //167/ 若tty指定的tty结构中的termios结构中的本地模式标志中 (回显输入字符) 标志ECHO已置位
						if (c<32)                        //168/ 如果字符c是控制字符(即ASCII码值小于32，控制字符DEL例外，其ASCII码为127)
							PUTCH(127,tty->write_q); //169/ 则将ASCII为127的控制字符DEL放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
						PUTCH(127,tty->write_q);         //170/ 将ASCII为127的控制字符DEL放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
						tty->write(tty);                 //171/ 执行tty指定的tty结构中的输出处理函数
					}                                        //172/ 
					DEC(tty->secondary.head);                //173/ 对数据头偏移量(tty->secondary.head)的值进行减1调整
				}                                                //174/ 
				continue;                                        //175/ 结束本次循环
			}                                                        //176/ 
			if (c==ERASE_CHAR(tty)) {                                //177/ 如果字符c是擦除字符
				if (EMPTY(tty->secondary) ||                     //178/ 如果tty缓冲队列(tty->secondary)的队列缓冲区为空
				   (c=LAST(tty->secondary))==10 ||               //179/ 或者 tty缓冲队列(tty->secondary)的队列缓冲区中剩余空间的末尾处字符是换行符NL
				   c==EOF_CHAR(tty))                             //180/ 或者 字符c是文件结束字符
					continue;                                //181/ 则结束本次循环
				if (L_ECHO(tty)) {                               //182/ 如果tty指定的tty结构中的termios结构中的本地模式标志中 (回显输入字符) 标志ECHO已置位
					if (c<32)                                //183/ 如果字符c是控制字符(即ASCII码值小于32，控制字符DEL例外，其ASCII码为127)
						PUTCH(127,tty->write_q);         //184/ 则将ASCII为127的控制字符DEL放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
					PUTCH(127,tty->write_q);                 //185/ 将ASCII为127的控制字符DEL放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
					tty->write(tty);                         //186/ 执行tty指定的tty结构中的输出处理函数
				}                                                //187/ 
				DEC(tty->secondary.head);                        //188/ 对数据头偏移量(tty->secondary.head)的值进行减1调整
				continue;                                        //189/ 结束本次循环
			}                                                        //190/ 
			if (c==STOP_CHAR(tty)) {                                 //191/ 如果字符c是停止字符
				tty->stopped=1;                                  //192/ 将tty指定的tty结构中的停止标志置位
				continue;                                        //193/ 结束本次循环
			}                                                        //194/ 
			if (c==START_CHAR(tty)) {                                //195/ 如果字符c是开始字符
				tty->stopped=0;                                  //196/ 将tty指定的tty结构中的停止标志复位
				continue;                                        //197/ 结束本次循环
			}                                                        //198/ 
		}                                                                //199/ 
		if (L_ISIG(tty)) {                                               //200/ 如果tty指定的tty结构中的termios结构中的本地模式标志中 (当收到字符INTR、QUIT、SUSP或DSUSP，产生相应的信号) 标志ISIG已置位
			if (c==INTR_CHAR(tty)) {                                 //201/ 如果字符c是中断字符
				tty_intr(tty,INTMASK);                           //202/ 向tty指向的tty结构中所属进程组号所指明的进程组中所有进程发送指定信号INTMASK(来自键盘的中断)
				continue;                                        //203/ 结束本次循环
			}                                                        //204/ 
			if (c==QUIT_CHAR(tty)) {                                 //205/ 如果字符c是退出字符
				tty_intr(tty,QUITMASK);                          //206/ 向tty指向的tty结构中所属进程组号所指明的进程组中所有进程发送指定信号QUITMASK(来自键盘的退出)
				continue;                                        //207/ 结束本次循环
			}                                                        //208/ 
		}                                                                //209/ 
		if (c==10 || c==EOF_CHAR(tty))                                   //210/ 如果字符c是换行符NL或文件结束字符
			tty->secondary.data++;                                   //211/ 则将tty指向的tty结构中的tty辅助缓冲队列的队列缓冲区中字符行数值加1
		if (L_ECHO(tty)) {                                               //212/ 如果tty指定的tty结构中的termios结构中的本地模式标志中 (回显输入字符) 标志ECHO已置位
			if (c==10) {                                             //213/ 如果字符c是换行符NL
				PUTCH(10,tty->write_q);                          //214/ 将换行符NL放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
				PUTCH(13,tty->write_q);                          //215/ 将回车符CR放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
			} else if (c<32) {                                       //216/ 如果字符c是控制字符(即ASCII码值小于32，控制字符DEL例外，其ASCII码为127)
				if (L_ECHOCTL(tty)) {                            //217/ 如果tty指定的tty结构中的termios结构中的本地模式标志中 (若设置了ECHO，则除TAB、NL、START和STOP以外的ASCII控制信号将被回显成像^X式样，X值是控制符+0x40) 标志ECHOCTL已置位
					PUTCH('^',tty->write_q);                 //218/ 将字符'^'放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
					PUTCH(c+64,tty->write_q);                //219/ 将字符c+64放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
				}                                                //220/ 
			} else                                                   //221/ 否则
				PUTCH(c,tty->write_q);                           //222/ 将字符c放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
			tty->write(tty);                                         //223/ 执行tty指定的tty结构中的输出处理函数
		}                                                                //224/ 
		PUTCH(c,tty->secondary);                                         //225/ 将字符c放入tty缓冲队列(tty->secondary)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
	}                                                                        //226/ 
	wake_up(&tty->secondary.proc_list);                                      //227/ 唤醒tty->secondary.proc_list指向的任务，tty->secondary.proc_list是任务等待队列头指针
}                                                                                //228/ 
                                                                                 //229/ 
int tty_read(unsigned channel, char * buf, int nr)                               //230/ [b;]tty读函数——从终端辅助缓冲队列中读取nr个字符，放入用户指定的缓冲区buf中(channel是子设备号，即tty_table[]中的序号，linux0.11终端只有3个子设备，分别是控制台终端-0，串口终端1-1，串口终端2-2)
{                                                                                //231/ 
	struct tty_struct * tty;                                                 //232/ 
	char c, * b=buf;                                                         //233/ 
	int minimum,time,flag=0;                                                 //234/ 
	long oldalarm;                                                           //235/ 
                                                                                 //236/ 
	if (channel>2 || nr<0) return -1;                                        //237/ 保证子设备号和欲读字节数的有效性
	tty = &tty_table[channel];                                               //238/ 取出tty结构数组中子设备号channel对应的项的地址赋给tty
	oldalarm = current->alarm;                                               //239/ 保存当前进程报警定时值(滴答数)到oldalarm中
	time = 10L*tty->termios.c_cc[VTIME];                                     //240/ 设置读操作定时值赋给time
	minimum = tty->termios.c_cc[VMIN];                                       //241/ 将最少需要读取的字符个数赋给minimum
	if (time && !minimum) {                                                  //242/ 如果设置了超时定时值(即time不等于0) 且 没有设置最少需读取字符个数
		minimum=1;                                                       //243/ 则将minimum设置为1
		if (flag=(!oldalarm || time+jiffies<oldalarm))                   //244/ 如果当前进程的原报警定时值为0 或者 (time+当前系统时间值)小于当前进程的原报警定时值
			current->alarm = time+jiffies;                           //245/ 则重置当前进程的报警定时值为(time+当前系统时间值)，并置位flag标志
	}                                                                        //246/ 
	if (minimum>nr)                                                          //247/ 如果设置的最少需要读取的字符个数 大于 欲读字节数nr
		minimum=nr;                                                      //248/ 则重置minimum为欲读字节数nr
	while (nr>0) {                                                           //249/ 如果欲读字节数nr大于0
		if (flag && (current->signal & ALRMMASK)) {                      //250/ 如果flag标志已置位 且 当前进程收到了定时报警信号SIGALRM 
			current->signal &= ~ALRMMASK;                            //251/ 则将当前进程的信号位图中的定时报警信号SIGALRM去除
			break;                                                   //252/ 跳出while循环
		}                                                                //253/ 
		if (current->signal)                                             //254/ 如果当前进程接收到信号
			break;                                                   //255/ 则跳出while循环
		if (EMPTY(tty->secondary) || (L_CANON(tty) &&                    //256/ 如果tty缓冲队列(tty->secondary)的队列缓冲区为空 或者 
		!tty->secondary.data && LEFT(tty->secondary)>20)) {              //257/ ( tty指定的tty结构中的termios结构中的本地模式标志中(开启规范模式(熟模式))标志ICANON已置位 并且 辅助队列中字符行数为0 并且 tty缓冲队列(tty->secondary)的队列缓冲区中还可存放字符的剩余长度(字节数)大于20 )
			sleep_if_empty(&tty->secondary);                         //258/ 则如果当前进程没有信号要处理，并且tty缓冲队列(tty->secondary)的队列缓冲区为空，则把当前任务置为可中断的睡眠状态
			continue;                                                //259/ 结束本次循环
		}                                                                //260/ 
		do {                                                             //261/ 
			GETCH(tty->secondary,c);                                 //262/ 从tty缓冲队列(tty->secondary)的队列缓冲区中数据尾偏移量指定的位置取出一个字符放入c中，调整数据尾偏移量的值
			if (c==EOF_CHAR(tty) || c==10)                           //263/ 如果字符c是文件结束字符或换行符NL
				tty->secondary.data--;                           //264/ 则
			if (c==EOF_CHAR(tty) && L_CANON(tty))                    //265/ 如果字符c是文件结束字符 且 tty指定的tty结构中的termios结构中的本地模式标志中(开启规范模式(熟模式))标志ICANON已置位
				return (b-buf);                                  //266/ 则返回已读字符数
			else {                                                   //267/ 否则
				put_fs_byte(c,b++);                              //268/ 将字符c存放在fs段中b++指定的内存地址处
				if (!--nr)                                       //269/ 将欲读字符数nr减1，如果值已经为0
					break;                                   //270/ 则跳出内层while循环
			}                                                        //271/ [r;]此处有个bug，参见P487
		} while (nr>0 && !EMPTY(tty->secondary));                        //272/ 当欲读字符数nr大于0 且 tty缓冲队列(tty->secondary)的队列缓冲区不为空
		if (time && !L_CANON(tty))                                       //273/ 如果设置了超时定时值(即time不等于0) 且 tty指定的tty结构中的termios结构中的本地模式标志中 (开启规范模式(熟模式)) 标志ICANON未置位
			if (flag=(!oldalarm || time+jiffies<oldalarm))           //274/ 如果当前进程的原报警定时值为0 或者 (time+当前系统时间值)小于当前进程的原报警定时值
				current->alarm = time+jiffies;                   //275/ 则重置当前进程的报警定时值为(time+当前系统时间值)，并置位flag标志
			else                                                     //276/ 否则
				current->alarm = oldalarm;                       //277/ 重置当前进程的报警定时值为当前进程的原报警定时值，并复位flag标志
		if (L_CANON(tty)) {                                              //278/ 如果tty指定的tty结构中的termios结构中的本地模式标志中 (开启规范模式(熟模式)) 标志ICANON已置位
			if (b-buf)                                               //279/ 若已经读到起码一个字符
				break;                                           //280/ 则跳出while循环
		} else if (b-buf >= minimum)                                     //281/ 若已读到的字符数大于等于最少要求读取的字符数
			break;                                                   //282/ 则跳出while循环
	}                                                                        //283/ 
	current->alarm = oldalarm;                                               //284/ 将当前进程的报警定时值恢复原值
	if (current->signal && !(b-buf))                                         //285/ 如果当前进程接收到信号 且 没有读取到任何字符
		return -EINTR;                                                   //286/ 则返回出错码(中断的系统调用)
	return (b-buf);                                                          //287/ 返回已读字符数
}                                                                                //288/ 
                                                                                 //289/ 
int tty_write(unsigned channel, char * buf, int nr)                              //290/ [b;]tty写函数——将用户指定的缓冲区buf中的nr个字符写入终端写缓冲队列中(channel是子设备号，即tty_table[]中的序号，linux0.11终端只有3个子设备，分别是控制台终端-0，串口终端1-1，串口终端2-2)
{                                                                                //291/ 
	static cr_flag=0;                                                        //292/ 定义回车标志
	struct tty_struct * tty;                                                 //293/ 
	char c, *b=buf;                                                          //294/ 
                                                                                 //295/ 
	if (channel>2 || nr<0) return -1;                                        //296/ 保证子设备号和欲写字节数的有效性
	tty = channel + tty_table;                                               //297/ 取出tty结构数组中子设备号channel对应的项的地址赋给tty
	while (nr>0) {                                                           //298/ 如果欲写字节数nr大于0
		sleep_if_full(&tty->write_q);                                    //299/ 如果当前进程没有信号要处理，并且tty缓冲队列tty->write_q的队列缓冲区已满，则把当前任务置为可中断的睡眠状态
		if (current->signal)                                             //300/ 如果当前进程接收到信号
			break;                                                   //301/ 则跳出while循环
		while (nr>0 && !FULL(tty->write_q)) {                            //302/ 当欲写字符数nr大于0 且 tty缓冲队列(tty->secondary)的队列缓冲区未满
			c=get_fs_byte(b);                                        //303/ 将fs段中b指定的内存地址处的一个字节存取出来赋给字符c
			if (O_POST(tty)) {                                       //304/ 如果tty指定的tty结构中的termios结构中的输出模式标志中 (执行输出处理) 标志OPOST已置位
				if (c=='\r' && O_CRNL(tty))                      //305/ 若字符c是回车符'\r' 且 tty指定的tty结构中的termios结构中的输出模式标志中(在输出时将回车符CR映射成换行符NL)标志OCRNL已置位
					c='\n';                                  //306/ 则将字符c转换为换行符'\n'
				else if (c=='\n' && O_NLRET(tty))                //307/ 若字符c是换行符'\n' 且 tty指定的tty结构中的termios结构中的输出模式标志中(换行符NL执行回车符的功能)标志ONLRET已置位
					c='\r';                                  //308/ 则将字符c转换为回车符'\r'
				if (c=='\n' && !cr_flag && O_NLCR(tty)) {        //309/ 若字符c是换行符'\n' 且 回车标志cr_flag等于0 且 tty指定的tty结构中的termios结构中的输出模式标志中(在输出时将换行符NL映射成回车-换行符CR-NL)标志ONLCR已置位
					cr_flag = 1;                             //310/ 则将回车标志cr_flag置位
					PUTCH(13,tty->write_q);                  //311/ 将回车符'\r'放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
					continue;                                //312/ 结束本次循环
				}                                                //313/ 
				if (O_LCUC(tty))                                 //314/ 如果tty指定的tty结构中的termios结构中的输出模式标志中 (在输出时将小写字符转换成大写字符) 标志OLCUC已置位
					c=toupper(c);                            //315/ 则判断ASCII码值为c的字符是否是小写字符，如果是则转换为大写字符赋给c
			}                                                        //316/ 
			b++; nr--;                                               //317/ 将用户数据缓冲区指针b前移一个字节；欲写字节数减一个字节
			cr_flag = 0;                                             //318/ 复位回车标志cr_flag
			PUTCH(c,tty->write_q);                                   //319/ 将字符c放入tty缓冲队列(tty->write_q)的队列缓冲区中数据头偏移量指定的位置，调整数据头偏移量的值
		}                                                                //320/ 
		tty->write(tty);                                                 //321/ 执行tty指定的tty结构中的输出处理函数
		if (nr>0)                                                        //322/ 如果欲写字节数还未写完
			schedule();                                              //323/ 则调度执行其他程序，以等待写队列中字符取走
	}                                                                        //324/ 
	return (b-buf);                                                          //325/ 返回已写入字节数
}                                                                                //326/ 
                                                                                 //327/ 
/*                                                                               //328/ 
 * Jeh, sometimes I really like the 386.                                         //329/ 
 * This routine is called from an interrupt,                                     //330/ 
 * and there should be absolutely no problem                                     //331/ 
 * with sleeping even in an interrupt (I hope).                                  //332/ 
 * Of course, if somebody proves me wrong, I'll                                  //333/ 
 * hate intel for all time :-). We'll have to                                    //334/ 
 * be careful and see to reinstating the interrupt                               //335/ 
 * chips before calling this, though.                                            //336/ 
 *                                                                               //337/ 
 * I don't think we sleep here under normal circumstances                        //338/ 
 * anyway, which is good, as the task sleeping might be                          //339/ 
 * totally innocent.                                                             //340/ 
 */                                                                              //341/ 
void do_tty_interrupt(int tty)                                                   //342/ [b;]tty中断处理调用函数——根据(tty_table+tty)指定的tty结构中的termios结构中设置的各种标志，将指定tty终端队列缓冲区中的字符复制转换成规范模式(熟模式)字符并存放在辅助队列(规范模式队列)中，该函数会被串口读字符中断和键盘中断调用
{                                                                                //343/ 
	copy_to_cooked(tty_table+tty);                                           //344/ 根据(tty_table+tty)指定的tty结构中的termios结构中设置的各种标志，将指定tty终端队列缓冲区中的字符复制转换成规范模式(熟模式)字符并存放在辅助队列(规范模式队列)中
}                                                                                //345/ 
                                                                                 //346/ 
void chr_dev_init(void)                                                          //347/ 字符设备初始化函数，空着，为以后扩展使用
{                                                                                //348/ 
}                                                                                //349/ 

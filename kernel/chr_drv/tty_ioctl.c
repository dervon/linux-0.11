/*                                                                              //  1/ 
 *  linux/kernel/chr_drv/tty_ioctl.c                                            //  2/ 
 *                                                                              //  3/ 
 *  (C) 1991  Linus Torvalds                                                    //  4/ 
 */                                                                             //  5/ 
                                                                                //  6/ 
#include <errno.h>                                                              //  7/ 
#include <termios.h>                                                            //  8/ 
                                                                                //  9/ 
#include <linux/sched.h>                                                        // 10/ 
#include <linux/kernel.h>                                                       // 11/ 
#include <linux/tty.h>                                                          // 12/ 
                                                                                // 13/ 
#include <asm/io.h>                                                             // 14/ 
#include <asm/segment.h>                                                        // 15/ 
#include <asm/system.h>                                                         // 16/ 
                                                                                // 17/ 
static unsigned short quotient[] = {                                            // 18/ 波特率因子数组(波特率 = 1.8432MHz / (16 * 波特率因子))
	0, 2304, 1536, 1047, 857,                                               // 19/ 
	768, 576, 384, 192, 96,                                                 // 20/ 
	64, 48, 24, 12, 6, 3                                                    // 21/ 
};                                                                              // 22/ 
                                                                                // 23/ 
static void change_speed(struct tty_struct * tty)                               // 24/ [b;]设置tty指向的tty结构的传输波特率，使其生效
{                                                                               // 25/ 
	unsigned short port,quot;                                               // 26/ 
                                                                                // 27/ 
	if (!(port = tty->read_q.data))                                         // 28/ 取出tty指向的tty结构中的tty读缓冲队列中的串口基地址赋给port，通过检查port的值判断是不是串口终端
		return;                                                         // 29/ 若不是串口终端，则直接退出
	quot = quotient[tty->termios.c_cflag & CBAUD];                          // 30/ 通过tty指向的tty结构中termios结构中的控制模式标志中的波特率索引号(即波特率因子数组的下标)找到波特率因子数组中的对应的波特率因子值赋给quot
	cli();                                                                  // 31/ 关中断
	outb_p(0x80,port+3);		/* set DLAB */                          // 32/ 将UART线路控制寄存器的位7(DLAB)置位
	outb_p(quot & 0xff,port);	/* LS of divisor */                     // 33/ 将波特率因子的低8位写入UART
	outb_p(quot >> 8,port+1);	/* MS of divisor */                     // 34/ 将波特率因子的高8位写入UART
	outb(0x03,port+3);		/* reset DLAB */                        // 35/ 将UART线路控制寄存器的位7(DLAB)复位
	sti();                                                                  // 36/ 开中断
}                                                                               // 37/ 
                                                                                // 38/ 
static void flush(struct tty_queue * queue)                                     // 39/ [b;]刷新清空queue指向的tty缓冲队列的队列缓冲区
{                                                                               // 40/ 
	cli();                                                                  // 41/ 关中断
	queue->head = queue->tail;                                              // 42/ 清空queue指向的tty缓冲队列的队列缓冲区
	sti();                                                                  // 43/ 开中断
}                                                                               // 44/ 
                                                                                // 45/ 
static void wait_until_sent(struct tty_struct * tty)                            // 46/ [b;]等待字符发送出去
{                                                                               // 47/ 
	/* do nothing - not implemented */                                      // 48/ 
}                                                                               // 49/ 
                                                                                // 50/ 
static void send_break(struct tty_struct * tty)                                 // 51/ [b;]发送BREAK控制符
{                                                                               // 52/ 
	/* do nothing - not implemented */                                      // 53/ 
}                                                                               // 54/ 
                                                                                // 55/ 
static int get_termios(struct tty_struct * tty, struct termios * termios)       // 56/ [b;]将参数tty指向的tty结构中termios结构信息复制到参数termios指向的termios结构中
{                                                                               // 57/ 
	int i;                                                                  // 58/ 
                                                                                // 59/ 
	verify_area(termios, sizeof (*termios));                                // 60/ 对当前任务的逻辑地址中从termios到termios+sizeof(*termios)这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
	for (i=0 ; i< (sizeof (*termios)) ; i++)                                // 61/ 遍寻整个termios结构
		put_fs_byte( ((char *)&tty->termios)[i] , i+(char *)termios );  // 62/ 将一字节((char *)&tty->termios)[i]存放在fs段中(i+(char *)termios)指定的内存地址处
	return 0;                                                               // 63/ 
}                                                                               // 64/ 
                                                                                // 65/ 
static int set_termios(struct tty_struct * tty, struct termios * termios)       // 66/ [b;]将参数termios指向的termios结构信息复制到参数tty指向的tty结构中termios结构中
{                                                                               // 67/ 
	int i;                                                                  // 68/ 
                                                                                // 69/ 
	for (i=0 ; i< (sizeof (*termios)) ; i++)                                // 70/ 遍寻整个termios结构
		((char *)&tty->termios)[i]=get_fs_byte(i+(char *)termios);      // 71/ 将fs段中(i+(char *)termios)指定的内存地址处的一个字节存取出来赋给((char *)&tty->termios)[i]
	change_speed(tty);                                                      // 72/ 设置tty指向的tty结构的传输波特率，使其生效
	return 0;                                                               // 73/ 
}                                                                               // 74/ 
                                                                                // 75/ 
static int get_termio(struct tty_struct * tty, struct termio * termio)          // 76/ [r;]AT&T系统V的termio结构操作
{                                                                               // 77/ 
	int i;                                                                  // 78/ 
	struct termio tmp_termio;                                               // 79/ 
                                                                                // 80/ 
	verify_area(termio, sizeof (*termio));                                  // 81/ 
	tmp_termio.c_iflag = tty->termios.c_iflag;                              // 82/ 
	tmp_termio.c_oflag = tty->termios.c_oflag;                              // 83/ 
	tmp_termio.c_cflag = tty->termios.c_cflag;                              // 84/ 
	tmp_termio.c_lflag = tty->termios.c_lflag;                              // 85/ 
	tmp_termio.c_line = tty->termios.c_line;                                // 86/ 
	for(i=0 ; i < NCC ; i++)                                                // 87/ 
		tmp_termio.c_cc[i] = tty->termios.c_cc[i];                      // 88/ 
	for (i=0 ; i< (sizeof (*termio)) ; i++)                                 // 89/ 
		put_fs_byte( ((char *)&tmp_termio)[i] , i+(char *)termio );     // 90/ 
	return 0;                                                               // 91/ 
}                                                                               // 92/ 
                                                                                // 93/ 
/*                                                                              // 94/ 
 * This only works as the 386 is low-byt-first                                  // 95/ 
 */                                                                             // 96/ 
static int set_termio(struct tty_struct * tty, struct termio * termio)          // 97/ [r;]AT&T系统V的termio结构操作
{                                                                               // 98/ 
	int i;                                                                  // 99/ 
	struct termio tmp_termio;                                               //100/ 
                                                                                //101/ 
	for (i=0 ; i< (sizeof (*termio)) ; i++)                                 //102/ 
		((char *)&tmp_termio)[i]=get_fs_byte(i+(char *)termio);         //103/ 
	*(unsigned short *)&tty->termios.c_iflag = tmp_termio.c_iflag;          //104/ 
	*(unsigned short *)&tty->termios.c_oflag = tmp_termio.c_oflag;          //105/ 
	*(unsigned short *)&tty->termios.c_cflag = tmp_termio.c_cflag;          //106/ 
	*(unsigned short *)&tty->termios.c_lflag = tmp_termio.c_lflag;          //107/ 
	tty->termios.c_line = tmp_termio.c_line;                                //108/ 
	for(i=0 ; i < NCC ; i++)                                                //109/ 
		tty->termios.c_cc[i] = tmp_termio.c_cc[i];                      //110/ 
	change_speed(tty);                                                      //111/ 
	return 0;                                                               //112/ 
}                                                                               //113/ 
                                                                                //114/ 
int tty_ioctl(int dev, int cmd, int arg)                                        //115/ [b;]根据设备号dev找到对应终端的tty结构，根据控制命令cmd分别进行处理
{                                                                               //116/ 
	struct tty_struct * tty;                                                //117/ 
	if (MAJOR(dev) == 5) {                                                  //118/ 如果设备号dev中的主设备号是5，即控制终端
		dev=current->tty;                                               //119/ 则将当前任务数据结构中的tty字段(即tty子设备号)赋给dev
		if (dev<0)                                                      //120/ 如果子设备号是负数，表示，表示当前任务没有控制终端
			panic("tty_ioctl: dev<0");                              //121/ 则直接死机
	} else                                                                  //122/ 否则
		dev=MINOR(dev);                                                 //123/ 将设备号dev中的子设备号赋给dev
	tty = dev + tty_table;                                                  //124/ 取出tty结构数组中子设备号dev对应的项的地址赋给tty
	switch (cmd) {                                                          //125/ 根据控制命令cmd分别进行处理
		case TCGETS:                                                    //126/ 
			return get_termios(tty,(struct termios *) arg);         //127/ 将参数tty指向的tty结构中termios结构信息复制到参数arg指向的termios结构中
		case TCSETSF:                                                   //128/ 
			flush(&tty->read_q); /* fallthrough */                  //129/ 刷新清空tty缓冲队列(tty->read_q)的队列缓冲区
		case TCSETSW:                                                   //130/ 
			wait_until_sent(tty); /* fallthrough */                 //131/ 等待字符发送出去
		case TCSETS:                                                    //132/ 
			return set_termios(tty,(struct termios *) arg);         //133/ 将参数arg指向的termios结构信息复制到参数tty指向的tty结构中termios结构中
		case TCGETA:                                                    //134/ 
			return get_termio(tty,(struct termio *) arg);           //135/ [r;]AT&T系统V的termio结构操作
		case TCSETAF:                                                   //136/ 
			flush(&tty->read_q); /* fallthrough */                  //137/ 刷新清空tty缓冲队列(tty->read_q)的队列缓冲区
		case TCSETAW:                                                   //138/ 
			wait_until_sent(tty); /* fallthrough */                 //139/ 等待字符发送出去
		case TCSETA:                                                    //140/ 
			return set_termio(tty,(struct termio *) arg);           //141/ [r;]AT&T系统V的termio结构操作
		case TCSBRK:                                                    //142/ 
			if (!arg) {                                             //143/ 
				wait_until_sent(tty);                           //144/ 等待字符发送出去
				send_break(tty);                                //145/ 发送BREAK控制符
			}                                                       //146/ 
			return 0;                                               //147/ 
		case TCXONC:                                                    //148/ 
			return -EINVAL; /* not implemented */                   //149/ 返回出错码(参数无效)
		case TCFLSH:                                                    //150/ 
			if (arg==0)                                             //151/ 
				flush(&tty->read_q);                            //152/ 刷新清空tty缓冲队列(tty->read_q)的队列缓冲区
			else if (arg==1)                                        //153/ 
				flush(&tty->write_q);                           //154/ 刷新清空tty缓冲队列(tty->write_q)的队列缓冲区
			else if (arg==2) {                                      //155/ 
				flush(&tty->read_q);                            //156/ 刷新清空tty缓冲队列(tty->read_q)的队列缓冲区
				flush(&tty->write_q);                           //157/ 刷新清空tty缓冲队列(tty->write_q)的队列缓冲区
			} else                                                  //158/ 
				return -EINVAL;                                 //159/ 
			return 0;                                               //160/ 
		case TIOCEXCL:                                                  //161/ 
			return -EINVAL; /* not implemented */                   //162/ 返回出错码(参数无效)
		case TIOCNXCL:                                                  //163/ 
			return -EINVAL; /* not implemented */                   //164/ 返回出错码(参数无效)
		case TIOCSCTTY:                                                 //165/ 
			return -EINVAL; /* set controlling term NI */           //166/ 返回出错码(参数无效)
		case TIOCGPGRP:                                                 //167/ 
			verify_area((void *) arg,4);                            //168/ 对当前任务的逻辑地址中从arg到arg+4这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
			put_fs_long(tty->pgrp,(unsigned long *) arg);           //169/ 将一长字(tty指定tty结构中的所属进程组号字段)存放在fs段中arg指定的内存地址处
			return 0;                                               //170/ 
		case TIOCSPGRP:                                                 //171/ 
			tty->pgrp=get_fs_long((unsigned long *) arg);           //172/ 将fs段中arg指定的内存地址处的一个长字存取出来赋给(tty指定tty结构中的所属进程组号字段)
			return 0;                                               //173/ 
		case TIOCOUTQ:                                                  //174/ 
			verify_area((void *) arg,4);                            //175/ 对当前任务的逻辑地址中从arg到arg+4这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
			put_fs_long(CHARS(tty->write_q),(unsigned long *) arg); //176/ 将一长字(tty写缓冲队列(tty->write_q)的队列缓冲区中已存放字符的长度(字节数))存放在fs段中arg指定的内存地址处
			return 0;                                               //177/ 
		case TIOCINQ:                                                   //178/ 
			verify_area((void *) arg,4);                            //179/ 对当前任务的逻辑地址中从arg到arg+4这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
			put_fs_long(CHARS(tty->secondary),                      //180/ 将一长字(tty辅助缓冲队列(tty->secondary)的队列缓冲区中已存放字符的长度(字节数))存放在fs段中arg指定的内存地址处
				(unsigned long *) arg);                         //181/ 
			return 0;                                               //182/ 
		case TIOCSTI:                                                   //183/ 
			return -EINVAL; /* not implemented */                   //184/ 返回出错码(参数无效)
		case TIOCGWINSZ:                                                //185/ 
			return -EINVAL; /* not implemented */                   //186/ 返回出错码(参数无效)
		case TIOCSWINSZ:                                                //187/ 
			return -EINVAL; /* not implemented */                   //188/ 返回出错码(参数无效)
		case TIOCMGET:                                                  //189/ 
			return -EINVAL; /* not implemented */                   //190/ 返回出错码(参数无效)
		case TIOCMBIS:                                                  //191/ 
			return -EINVAL; /* not implemented */                   //192/ 返回出错码(参数无效)
		case TIOCMBIC:                                                  //193/ 
			return -EINVAL; /* not implemented */                   //194/ 返回出错码(参数无效)
		case TIOCMSET:                                                  //195/ 
			return -EINVAL; /* not implemented */                   //196/ 返回出错码(参数无效)
		case TIOCGSOFTCAR:                                              //197/ 
			return -EINVAL; /* not implemented */                   //198/ 返回出错码(参数无效)
		case TIOCSSOFTCAR:                                              //199/ 
			return -EINVAL; /* not implemented */                   //200/ 返回出错码(参数无效)
		default:                                                        //201/ 
			return -EINVAL;                                         //202/ 返回出错码(参数无效)
	}                                                                       //203/ 
}                                                                               //204/ 

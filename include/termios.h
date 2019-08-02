#ifndef _TERMIOS_H                                                       //  1/ 
#define _TERMIOS_H                                                       //  2/ 
                                                                         //  3/ 
#define TTY_BUF_SIZE 1024                                                //  4/ 
                                                                         //  5/ 
/* 0x54 is just a magic number to make these relatively uniqe ('T') */   //  6/ 
                                                                         //  7/ 
#define TCGETS		0x5401                                           //  8/ 
#define TCSETS		0x5402                                           //  9/ 
#define TCSETSW		0x5403                                           // 10/ 
#define TCSETSF		0x5404                                           // 11/ 
#define TCGETA		0x5405                                           // 12/ 
#define TCSETA		0x5406                                           // 13/ 
#define TCSETAW		0x5407                                           // 14/ 
#define TCSETAF		0x5408                                           // 15/ 
#define TCSBRK		0x5409                                           // 16/ 
#define TCXONC		0x540A                                           // 17/ 
#define TCFLSH		0x540B                                           // 18/ 
#define TIOCEXCL	0x540C                                           // 19/ 
#define TIOCNXCL	0x540D                                           // 20/ 
#define TIOCSCTTY	0x540E                                           // 21/ 
#define TIOCGPGRP	0x540F                                           // 22/ 
#define TIOCSPGRP	0x5410                                           // 23/ 
#define TIOCOUTQ	0x5411                                           // 24/ 
#define TIOCSTI		0x5412                                           // 25/ 
#define TIOCGWINSZ	0x5413                                           // 26/ 
#define TIOCSWINSZ	0x5414                                           // 27/ 
#define TIOCMGET	0x5415                                           // 28/ 
#define TIOCMBIS	0x5416                                           // 29/ 
#define TIOCMBIC	0x5417                                           // 30/ 
#define TIOCMSET	0x5418                                           // 31/ 
#define TIOCGSOFTCAR	0x5419                                           // 32/ 
#define TIOCSSOFTCAR	0x541A                                           // 33/ 
#define TIOCINQ		0x541B                                           // 34/ 
                                                                         // 35/ 
struct winsize {                                                         // 36/ 
	unsigned short ws_row;                                           // 37/ 
	unsigned short ws_col;                                           // 38/ 
	unsigned short ws_xpixel;                                        // 39/ 
	unsigned short ws_ypixel;                                        // 40/ 
};                                                                       // 41/ 
                                                                         // 42/ 
#define NCC 8                                                            // 43/ 
struct termio {                                                          // 44/ [r;]AT&T系统V的termio结构
	unsigned short c_iflag;		/* input mode flags */           // 45/ 
	unsigned short c_oflag;		/* output mode flags */          // 46/ 
	unsigned short c_cflag;		/* control mode flags */         // 47/ 
	unsigned short c_lflag;		/* local mode flags */           // 48/ 
	unsigned char c_line;		/* line discipline */            // 49/ 
	unsigned char c_cc[NCC];	/* control characters */         // 50/ 
};                                                                       // 51/ 
                                                                         // 52/ 
#define NCCS 17                                                          // 53/ 
struct termios {                                                         // 54/ 行规则函数即是根据这个结构的设置参数进行操作的
	unsigned long c_iflag;		/* input mode flags */           // 55/ 输入模式标志，用于控制如何对终端输入字符进行变换处理
	unsigned long c_oflag;		/* output mode flags */          // 56/ 输出模式标志，用于控制如何把字符输出到终端上
	unsigned long c_cflag;		/* control mode flags */         // 57/ 控制模式标志，用于定义串行终端传输特性，包括波特率、字符比特位数以及停止位数等
	unsigned long c_lflag;		/* local mode flags */           // 58/ 本地模式标志，用于控制驱动程序与用户的交互，例如是否需要回显字符、终端是处于规范还是非规范模式等
	unsigned char c_line;		/* line discipline */            // 59/ 线路规程(速率)，针对不同的终端设备，可以有不同的行规程与之匹配。(Linux0.11中仅有一个行规则函数，c_line无效)
	unsigned char c_cc[NCCS];	/* control characters */         // 60/ 控制字符数组，包含了终端所有可以修改的特殊字符，例如可以通过修改其中的中断字符(^C)由其他按键产生
};                                                                       // 61/ 
                                                                         // 62/ 
/* c_cc characters */                                                    // 63/ 64-80行为c_cc[NCCS]的下标
#define VINTR 0                                                          // 64/ (^C),\003，中断字符
#define VQUIT 1                                                          // 65/ (^\),\034，退出字符
#define VERASE 2                                                         // 66/ (^H),\177，擦除字符
#define VKILL 3                                                          // 67/ (^U),\025，终止字符(删除行)
#define VEOF 4                                                           // 68/ (^D),\004，文件结束字符
#define VTIME 5                                                          // 69/ (\0),\0，超时定时值，参见P702
#define VMIN 6                                                           // 70/ (\1),\1，最小需读取的字符个数，参见P702
#define VSWTC 7                                                          // 71/ (\0),\0，交换字符
#define VSTART 8                                                         // 72/ (^Q),\021，开始字符
#define VSTOP 9                                                          // 73/ (^S),\023，停止字符
#define VSUSP 10                                                         // 74/ (^Z),\032，挂起字符
#define VEOL 11                                                          // 75/ (\0),\0，行结束字符
#define VREPRINT 12                                                      // 76/ (^R),\022，重显示字符
#define VDISCARD 13                                                      // 77/ (^O),\017，丢弃字符
#define VWERASE 14                                                       // 78/ (^W),\027，单词擦除字符
#define VLNEXT 15                                                        // 79/ (^V),\026，下一行字符
#define VEOL2 16                                                         // 80/ (\0),\0，行结束字符2
                                                                         // 81/ 
/* c_iflag bits */                                                       // 82/ [b;]输入模式标志，八进制表示
#define IGNBRK	0000001                                                  // 83/ 输入时忽略BREAK条件
#define BRKINT	0000002                                                  // 84/ 在BREAK时产生SIGINT信号
#define IGNPAR	0000004                                                  // 85/ 忽略奇偶校验出错的字符
#define PARMRK	0000010                                                  // 86/ 标记奇偶校验错
#define INPCK	0000020                                                  // 87/ 允许输入奇偶校验
#define ISTRIP	0000040                                                  // 88/ 屏蔽字符第8位
#define INLCR	0000100                                                  // 89/ 输入时将换行符NL映射成回车符CR
#define IGNCR	0000200                                                  // 90/ 忽略回车符CR
#define ICRNL	0000400                                                  // 91/ 在输入时将回车符CR映射成换行符NL
#define IUCLC	0001000                                                  // 92/ 在输入时将大写字符转换成小写字符
#define IXON	0002000                                                  // 93/ 允许开始/停止(XON/XOFF)输出控制
#define IXANY	0004000                                                  // 94/ 允许任何字符重启输出
#define IXOFF	0010000                                                  // 95/ 允许开始/停止(XON/XOFF)输入控制
#define IMAXBEL	0020000                                                  // 96/ 输入队列满时响铃
                                                                         // 97/ 
/* c_oflag bits */                                                       // 98/ [b;]输出模式标志，八进制表示
#define OPOST	0000001                                                  // 99/ 执行输出处理
#define OLCUC	0000002                                                  //100/ 在输出时将小写字符转换成大写字符
#define ONLCR	0000004                                                  //101/ 在输出时将换行符NL映射成回车-换行符CR-NL
#define OCRNL	0000010                                                  //102/ 在输出时将回车符CR映射成换行符NL
#define ONOCR	0000020                                                  //103/ 在0列不输出回车符CR
#define ONLRET	0000040                                                  //104/ 换行符NL执行回车符的功能
#define OFILL	0000100                                                  //105/ 延迟时使用填充字符而不使用时间延迟
#define OFDEL	0000200                                                  //106/ 填充字符是ASCII码DEL。如果未设置，则使用ASCII码NULL
#define NLDLY	0000400                                                  //107/ 选择换行延迟
#define   NL0	0000000                                                  //108/ 换行延迟类型0
#define   NL1	0000400                                                  //109/ 换行延迟类型1
#define CRDLY	0003000                                                  //110/ 选择回车延迟
#define   CR0	0000000                                                  //111/ 回车延迟类型0
#define   CR1	0001000                                                  //112/ 回车延迟类型1
#define   CR2	0002000                                                  //113/ 回车延迟类型2
#define   CR3	0003000                                                  //114/ 回车延迟类型3
#define TABDLY	0014000                                                  //115/ 选择水平制表延迟
#define   TAB0	0000000                                                  //116/ 水平制表延迟类型0
#define   TAB1	0004000                                                  //117/ 水平制表延迟类型1
#define   TAB2	0010000                                                  //118/ 水平制表延迟类型2
#define   TAB3	0014000                                                  //119/ 水平制表延迟类型3
#define   XTABS	0014000                                                  //120/ 将制表符TAB换成空格，该值表示空格数
#define BSDLY	0020000                                                  //121/ 选择退格延迟
#define   BS0	0000000                                                  //122/ 退格延迟类型0
#define   BS1	0020000                                                  //123/ 退格延迟类型1
#define VTDLY	0040000                                                  //124/ 选择纵向制表延迟
#define   VT0	0000000                                                  //125/ 纵向制表延迟类型0
#define   VT1	0040000                                                  //126/ 纵向制表延迟类型1
#define FFDLY	0040000                                                  //127/ 选择换页延迟
#define   FF0	0000000                                                  //128/ 换页延迟类型0
#define   FF1	0040000                                                  //129/ 换页延迟类型1
                                                                         //130/ 
/* c_cflag bit meaning */                                                //131/ [b;]控制模式标志，八进制表示
#define CBAUD	0000017                                                  //132/ 传输速率位屏蔽码
#define  B0	0000000		/* hang up */                            //133/ 挂断线路
#define  B50	0000001                                                  //134/ 波特率 50
#define  B75	0000002                                                  //135/ 波特率 75
#define  B110	0000003                                                  //136/ 波特率 110
#define  B134	0000004                                                  //137/ 波特率 134
#define  B150	0000005                                                  //138/ 波特率 150
#define  B200	0000006                                                  //139/ 波特率 200
#define  B300	0000007                                                  //140/ 波特率 300
#define  B600	0000010                                                  //141/ 波特率 600
#define  B1200	0000011                                                  //142/ 波特率 1200
#define  B1800	0000012                                                  //143/ 波特率 1800
#define  B2400	0000013                                                  //144/ 波特率 2400
#define  B4800	0000014                                                  //145/ 波特率 4800
#define  B9600	0000015                                                  //146/ 波特率 9600
#define  B19200	0000016                                                  //147/ 波特率 19200
#define  B38400	0000017                                                  //148/ 波特率 38400
#define EXTA B19200                                                      //149/ 扩展波特率A
#define EXTB B38400                                                      //150/ 扩展波特率B
#define CSIZE	0000060                                                  //151/ 字符位宽度屏蔽码
#define   CS5	0000000                                                  //152/ 每字符5比特位
#define   CS6	0000020                                                  //153/ 每字符6比特位
#define   CS7	0000040                                                  //154/ 每字符7比特位
#define   CS8	0000060                                                  //155/ 每字符8比特位
#define CSTOPB	0000100                                                  //156/ 设置2个停止位，而不是1个
#define CREAD	0000200                                                  //157/ 允许接收
#define CPARENB	0000400                                                  //158/ 开启输出时产生奇偶位、输入时进行奇偶校验
#define CPARODD	0001000                                                  //159/ 输入/输出校验是奇校验
#define HUPCL	0002000                                                  //160/ 最后进程关闭后挂断
#define CLOCAL	0004000                                                  //161/ 忽略调制解调器控制线路
#define CIBAUD	03600000		/* input baud rate (not used) */ //162/ 输入波特率(未使用)
#define CRTSCTS	020000000000		/* flow control */               //163/ 流控制
                                                                         //164/ 
#define PARENB CPARENB                                                   //165/ 开启输出时产生奇偶位、输入时进行奇偶校验
#define PARODD CPARODD                                                   //166/ 输入/输出校验是奇校验
                                                                         //167/ 
/* c_lflag bits */                                                       //168/ [b;]本地模式标志，八进制表示
#define ISIG	0000001                                                  //169/ 当收到字符INTR、QUIT、SUSP或DSUSP，产生相应的信号
#define ICANON	0000002                                                  //170/ 开启规范模式(熟模式)
#define XCASE	0000004                                                  //171/ 若设置了ICANON，则终端是大写字符的
#define ECHO	0000010                                                  //172/ 回显输入字符
#define ECHOE	0000020                                                  //173/ 若设置了ICANON，则ERASE/WERASE将擦除前一字符/单词
#define ECHOK	0000040                                                  //174/ 若设置了ICANON，则KILL字符将擦除当前行
#define ECHONL	0000100                                                  //175/ 若设置了ICANON，则即使ECHO没有开启也回显NL字符
#define NOFLSH	0000200                                                  //176/ 当生成SIGINT和SIGQUIT信号时不刷新输入输出队列，当生成SIGSUSP信号时，刷新输入队列
#define TOSTOP	0000400                                                  //177/ 发送SIGTTOU信号到后台进程的进程组，该后台进程试图写自己的控制终端
#define ECHOCTL	0001000                                                  //178/ 若设置了ECHO，则除TAB、NL、START和STOP以外的ASCII控制信号将被回显成像^X式样，X值是控制符+0x40
#define ECHOPRT	0002000                                                  //179/ 若设置了ICANON和IECHO，则字符在擦除时将显示
#define ECHOKE	0004000                                                  //180/ 若设置了ICANON，则KILL通过擦除行上的所有字符被回显
#define FLUSHO	0010000                                                  //181/ 输出被刷新。通过键入DISCARD字符，该标志被翻转
#define PENDIN	0040000                                                  //182/ 当下一个字符是读时，输入队列中的所有字符将被重显
#define IEXTEN	0100000                                                  //183/ 开启实现时定义的输入处理
                                                                         //184/ 
/* modem lines */                                                        //185/ [b;]modem线路信号符号常数
#define TIOCM_LE	0x001                                            //186/ 线路允许
#define TIOCM_DTR	0x002                                            //187/ 数据终端就绪
#define TIOCM_RTS	0x004                                            //188/ 请求发送
#define TIOCM_ST	0x008                                            //189/ 串行数据发送
#define TIOCM_SR	0x010                                            //190/ 串行数据接收
#define TIOCM_CTS	0x020                                            //191/ 清除发送
#define TIOCM_CAR	0x040                                            //192/ 载波监测
#define TIOCM_RNG	0x080                                            //193/ 响铃指示
#define TIOCM_DSR	0x100                                            //194/ 数据设备就绪
#define TIOCM_CD	TIOCM_CAR                                        //195/ 
#define TIOCM_RI	TIOCM_RNG                                        //196/ 
                                                                         //197/ 
/* tcflow() and TCXONC use these */                                      //198/ 
#define	TCOOFF		0                                                //199/ 挂起输出
#define	TCOON		1                                                //200/ 重启被挂起的输出
#define	TCIOFF		2                                                //201/ 系统传输一个STOP字符，使设备停止向系统传输数据
#define	TCION		3                                                //202/ 系统传输一个START字符，使设备开始向系统传输数据
                                                                         //203/ 
/* tcflush() and TCFLSH use these */                                     //204/ 
#define	TCIFLUSH	0                                                //205/ 清接收到的数据但不读
#define	TCOFLUSH	1                                                //206/ 清已写的数据但不传送
#define	TCIOFLUSH	2                                                //207/ 清接收到的数据但不读，清已写的数据但不传送
                                                                         //208/ 
/* tcsetattr uses these */                                               //209/ 
#define	TCSANOW		0                                                //210/ 改变立即发生
#define	TCSADRAIN	1                                                //211/ 改变在所有已写的输出被传输之后发生
#define	TCSAFLUSH	2                                                //212/ 改变在所有已写的输出被传输之后并且在所有接收到但还没有读取的数据被丢弃之后发生
                                                                         //213/ 
typedef int speed_t;                                                     //214/ 
                                                                         //215/ 
extern speed_t cfgetispeed(struct termios *termios_p);                   //216/ 
extern speed_t cfgetospeed(struct termios *termios_p);                   //217/ 
extern int cfsetispeed(struct termios *termios_p, speed_t speed);        //218/ 
extern int cfsetospeed(struct termios *termios_p, speed_t speed);        //219/ 
extern int tcdrain(int fildes);                                          //220/ 
extern int tcflow(int fildes, int action);                               //221/ 
extern int tcflush(int fildes, int queue_selector);                      //222/ 
extern int tcgetattr(int fildes, struct termios *termios_p);             //223/ 
extern int tcsendbreak(int fildes, int duration);                        //224/ 
extern int tcsetattr(int fildes, int optional_actions,                   //225/ 
	struct termios *termios_p);                                      //226/ 
                                                                         //227/ 
#endif                                                                   //228/ 

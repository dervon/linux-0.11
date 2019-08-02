/*                                                                             ##  1# 
 *  linux/kernel/keyboard.S                                                    ##  2# 
 *                                                                             ##  3# 
 *  (C) 1991  Linus Torvalds                                                   ##  4# 
 */                                                                            ##  5# 
                                                                               ##  6# 
/*                                                                             ##  7# 
 *	Thanks to Alfred Leung for US keyboard patches                         ##  8# 
 *		Wolfgang Thiel for German keyboard patches                     ##  9# 
 *		Marc Corsini for the French keyboard                           ## 10# 
 */                                                                            ## 11# 
                                                                               ## 12# 
#include <linux/config.h>                                                      ## 13# 
                                                                               ## 14# 
.text                                                                          ## 15# 
.globl _keyboard_interrupt                                                     ## 16# 
                                                                               ## 17# 
/*                                                                             ## 18# 
 * these are for the keyboard read functions                                   ## 19# 
 */                                                                            ## 20# 
size	= 1024		/* must be a power of two ! And MUST be the same       ## 21# 键盘缓冲队列的队列缓冲区的长度(字节数)，即控制台对应的tty结构中的tty读缓冲队列的队列缓冲区长度
			   as in tty_io.c !!!! */                              ## 22# 
head = 4                                                                       ## 23# 键盘缓冲队列中数据头偏移量字段在键盘缓冲队列结构tty_queue中的偏移量
tail = 8                                                                       ## 24# 键盘缓冲队列中数据尾偏移量字段在键盘缓冲队列结构tty_queue中的偏移量
proc_list = 12                                                                 ## 25# 键盘缓冲队列中等待进程列表字段在键盘缓冲队列结构tty_queue中的偏移量
buf = 16                                                                       ## 26# 键盘缓冲队列中缓冲区字段在键盘缓冲队列结构tty_queue中的偏移量
                                                                               ## 27# 
mode:	.byte 0		/* caps, alt, ctrl and shift mode */                   ## 28# mode为键盘特殊键的按下状态标志(位7-caps键按下；位6-caps键的状态，与leds中的位2一样；位5-右alt键按下；位4-左alt键按下；位3-右ctrl键按下；位2-左ctrl键按下；位1-右shift键按下；位0-左shift键按下)
leds:	.byte 2		/* num-lock, caps, scroll-lock mode (nom-lock on) */   ## 29# leds用于表示键盘指示灯的状态标志(位7至位3-不使用；位2-caps键的状态，与leds中的位2一样；位1-(num-lock)键的状态；位0-(scroll-lock)键的状态)
e0:	.byte 0                                                                ## 30# 若收到的扫描码是0xe0，意味着还有1个字符跟随其后；若收到的扫描码是0xe1，意味着还有2个字符跟随其后(位7至位2-不使用；位1-0xe1标志，置位表示收到了0xe1前导码；位0-0xe0标志，置位表示收到了0xe0前导码)
                                                                               ## 31# 
/*                                                                             ## 32# 
 *  con_int is the real interrupt routine that reads the                       ## 33# 
 *  keyboard scan-code and converts it into the appropriate                    ## 34# 
 *  ascii character(s).                                                        ## 35# 
 */                                                                            ## 36# 
_keyboard_interrupt:                                                           ## 37# 键盘中断处理程序入口
	pushl %eax                                                             ## 38# 
	pushl %ebx                                                             ## 39# 
	pushl %ecx                                                             ## 40# 
	pushl %edx                                                             ## 41# 
	push %ds                                                               ## 42# 
	push %es                                                               ## 43# 
	movl $0x10,%eax                                                        ## 44# 
	mov %ax,%ds                                                            ## 45# 将ds指向内核数据段
	mov %ax,%es                                                            ## 46# 将es指向内核数据段
	xorl %al,%al		/* %eax is scan code */                        ## 47# 
	inb $0x60,%al                                                          ## 48# 从键盘控制器的0x60端口读出扫描码放入al中
	cmpb $0xe0,%al                                                         ## 49# 判断扫描码是否是0xe0
	je set_e0                                                              ## 50# 如果是0xe0则跳转到set_e0
	cmpb $0xe1,%al                                                         ## 51# 判断扫描码是否是0xe1
	je set_e1                                                              ## 52# 如果是0xe1则跳转到set_e1
	call key_table(,%eax,4)                                                ## 53# 如果扫描码既不是0xe0也不是0xe1，则调用键处理程序key_table+eax*4
	movb $0,e0                                                             ## 54# 将e0中的0xe0标志和0xe1标志复位
e0_e1:	inb $0x61,%al                                                          ## 55# 从键盘控制器的0x61端口读出原始数据赋给al
	jmp 1f                                                                 ## 56# 56-57行用于延时一会
1:	jmp 1f                                                                 ## 57# 
1:	orb $0x80,%al                                                          ## 58# 将al的位7置位，用于禁止键盘
	jmp 1f                                                                 ## 59# 59-60行用于延时一会
1:	jmp 1f                                                                 ## 60# 
1:	outb %al,$0x61                                                         ## 61# 将al中的新值写回键盘控制器的0x61端口，使禁止键盘生效
	jmp 1f                                                                 ## 62# 62-63行用于延时一会
1:	jmp 1f                                                                 ## 63# 
1:	andb $0x7F,%al                                                         ## 64# 将al的位7复位，用于允许键盘
	outb %al,$0x61                                                         ## 65# 将al中的新值写回键盘控制器的0x61端口，使允许键盘生效
	movb $0x20,%al                                                         ## 66# 
	outb %al,$0x20                                                         ## 67# 向8259A主芯片发送EOI(中断结束)信号
	pushl $0                                                               ## 68# 将控制台终端的tty序号0压栈，作为参数传入
	call _do_tty_interrupt                                                 ## 69# [r;]调用tty_io.c中的do_tty_interrupt，进行下一步处理
	addl $4,%esp                                                           ## 70# 丢弃入栈的参数
	pop %es                                                                ## 71# 
	pop %ds                                                                ## 72# 
	popl %edx                                                              ## 73# 
	popl %ecx                                                              ## 74# 
	popl %ebx                                                              ## 75# 
	popl %eax                                                              ## 76# 
	iret                                                                   ## 77# 中断返回
set_e0:	movb $1,e0                                                             ## 78# 收到了扫描前导码0xe0，置位e0中的0xe0标志，即位0
	jmp e0_e1                                                              ## 79# 跳转到e0_e1去执行
set_e1:	movb $2,e0                                                             ## 80# 收到了扫描前导码0xe1，置位e0中的0xe1标志，即位1
	jmp e0_e1                                                              ## 81# 跳转到e0_e1去执行
                                                                               ## 82# 
/*                                                                             ## 83# 
 * This routine fills the buffer with max 8 bytes, taken from                  ## 84# 
 * %ebx:%eax. (%edx is high). The bytes are written in the                     ## 85# 
 * order %al,%ah,%eal,%eah,%bl,%bh ... until %eax is zero.                     ## 86# 
 */                                                                            ## 87# 
put_queue:                                                                     ## 88# 89-109行用于将al中的字符放入控制台终端读缓冲队列中的缓冲区
	pushl %ecx                                                             ## 89# 
	pushl %edx                                                             ## 90# 
	movl _table_list,%edx		# read-queue for console               ## 91# 将tty读写缓冲队列结构地址表中的控制台终端读缓冲队列地址&tty_table[0].read_q赋给edx
	movl head(%edx),%ecx                                                   ## 92# 将控制台终端读缓冲队列中的缓冲区数据头偏移量赋给ecx
1:	movb %al,buf(%edx,%ecx)                                                ## 93# 将al中的字符放入控制台终端读缓冲队列tty_table[0].read_q中的数据头偏移量指定的队列缓冲区中的位置
	incl %ecx                                                              ## 94# 将ecx中的数据头偏移量加1
	andl $size-1,%ecx                                                      ## 95# 保证ecx中的数据头偏移量超过了队列缓冲区的末端，则使其折回到队列缓冲区的始端
	cmpl tail(%edx),%ecx		# buffer full - discard everything     ## 96# 将ecx中的数据头偏移量与控制台终端读缓冲队列中的数据尾偏移量进行比较
	je 3f                                                                  ## 97# 如果相等，说明队列缓冲区已满，则ebx:eax中的除al外的其他字符全部抛弃，跳转到下面的标号3处返回
	shrdl $8,%ebx,%eax                                                     ## 98# (队列缓冲区未满)将ebx(值为0)中的数据右移8位到eax中，ebx本身不变
	je 2f                                                                  ## 99# 如果eax=0，说明eax中没有其他字符了，则跳转到下面的标号2处执行
	shrl $8,%ebx                                                           ##100# (ebx:eax中还有其他字符)将ebx的值右移8位
	jmp 1b                                                                 ##101# 跳转到上面的标号1处执行
2:	movl %ecx,head(%edx)                                                   ##102# 将ecx中最新的数据头偏移量写回到控制台终端读缓冲队列tty_table[0].read_q中的数据头偏移量字段中
	movl proc_list(%edx),%ecx                                              ##103# 控制台终端读缓冲队列tty_table[0].read_q中的等待进程列表指针字段值赋给ecx
	testl %ecx,%ecx                                                        ##104# 检测ecx是否为空
	je 3f                                                                  ##105# 如果ecx为空，说明控制台终端读缓冲队列的等待进程表中没有等待该队列的进程，则跳转到下面的标号3处返回
	movl $0,(%ecx)                                                         ##106# (控制台终端读缓冲队列的等待进程表中有等待该队列的进程)将该等待进程的状态置为就绪态，唤醒该进程
3:	popl %edx                                                              ##107# 
	popl %ecx                                                              ##108# 
	ret                                                                    ##109# 返回
                                                                               ##110# 
ctrl:	movb $0x04,%al                                                         ##111# 按下左ctrl时的处理子程序入口，0x04是mode中左ctrl键对应的比特位
	jmp 1f                                                                 ##112# 跳转到下面的标号1处执行
alt:	movb $0x10,%al                                                         ##113# 按下左alt时的处理子程序入口，0x10是mode中左alt键对应的比特位
1:	cmpb $0,e0                                                             ##114# 判断e0中是否有标志置位(即判断按下的是左边的还是右边的ctrl/alt键)
	je 2f                                                                  ##115# 如果按下的是左边的ctrl/alt键，则直接跳转到下面的标号2处执行
	addb %al,%al                                                           ##116# 否则将al中值左移一位，使得右边的ctrl/alt键在mode中对应的位置位
2:	orb %al,mode                                                           ##117# 将左边的或者右边的ctrl/alt键对应的mode中的比特位置位
	ret                                                                    ##118# 返回
unctrl:	movb $0x04,%al                                                         ##119# 松开左ctrl时的处理子程序入口，0x04是mode中左ctrl键对应的比特位
	jmp 1f                                                                 ##120# 跳转到下面的标号1处执行
unalt:	movb $0x10,%al                                                         ##121# 松开左alt时的处理子程序入口，0x10是mode中左alt键对应的比特位
1:	cmpb $0,e0                                                             ##122# 判断e0中是否有标志置位(即判断松开的是左边的还是右边的ctrl/alt键)
	je 2f                                                                  ##123# 如果松开的是左边的ctrl/alt键，则直接跳转到下面的标号2处执行
	addb %al,%al                                                           ##124# 否则将al中值左移一位，使得右边的ctrl/alt键在mode中对应的位置位
2:	notb %al                                                               ##125# 将al中的位取反
	andb %al,mode                                                          ##126# 将左边的或者右边的ctrl/alt键对应的mode中的比特位复位
	ret                                                                    ##127# 返回
                                                                               ##128# 
lshift:                                                                        ##129# 按下左shift时的处理子程序入口
	orb $0x01,mode                                                         ##130# 将左边的shift键对应的mode中的比特位置位
	ret                                                                    ##131# 返回
unlshift:                                                                      ##132# 松开左shift时的处理子程序入口
	andb $0xfe,mode                                                        ##133# 将左边的shift键对应的mode中的比特位复位
	ret                                                                    ##134# 返回
rshift:                                                                        ##135# 按下右shift时的处理子程序入口
	orb $0x02,mode                                                         ##136# 将右边的shift键对应的mode中的比特位置位
	ret                                                                    ##137# 返回
unrshift:                                                                      ##138# 松开右shift时的处理子程序入口
	andb $0xfd,mode                                                        ##139# 将右边的shift键对应的mode中的比特位复位
	ret                                                                    ##140# 返回
                                                                               ##141# 
caps:	testb $0x80,mode                                                       ##142# 按下caps时的处理子程序入口，判断(caps键按下)对应的mode中的位是否已置位
	jne 1f                                                                 ##143# 若没有置位，说明caps键没有被按下，则跳转到下面的标号1处直接返回
	xorb $4,leds                                                           ##144# 若已置位，则将(caps键的状态)对应的leds中的位取反
	xorb $0x40,mode                                                        ##145# 则将(caps键的状态)对应的mode中的位取反
	orb $0x80,mode                                                         ##146# 将(caps键按下)对应的mode中的位置位
set_leds:                                                                      ##147# 147-154行根据leds中的状态标志(caps-lock、num-lock和srcoll-lock)，关闭或开启相应的LED指示灯
	call kb_wait                                                           ##148# 调用kb_wait循环判断键盘控制器中的输入缓冲器是否已满，直至发现未满才从kb_wait返回
	movb $0xed,%al		/* set leds command */                         ##149# 命令0xed是键盘命令，用于置位/复位模式指示器，1-开启；0-关闭(该命令所带参数——位7-3：保留全为0；位2：caps-lock键；位1：num-lock键；位0：scroll-lock键)
	outb %al,$0x60                                                         ##150# 将0xed命令传入键盘控制器的0x60端口
	call kb_wait                                                           ##151# 调用kb_wait循环判断键盘控制器中的输入缓冲器是否已满，直至发现未满才从kb_wait返回
	movb leds,%al                                                          ##152# 152-153行用于将leds的值作为参数传入键盘控制器的0x60端口
	outb %al,$0x60                                                         ##153# 
	ret                                                                    ##154# 返回
uncaps:	andb $0x7f,mode                                                        ##155# 松开caps时的处理子程序入口，将(caps键按下)对应的mode中的比特位复位
	ret                                                                    ##156# 返回
scroll:                                                                        ##157# 按下scroll-lock时的处理子程序入口
	xorb $1,leds                                                           ##158# 将(scroll-lock键的状态)对应的leds中的位取反
	jmp set_leds                                                           ##159# 设置对应的LED指示灯，并返回
num:	xorb $2,leds                                                           ##160# 按下num-lock时的处理子程序入口
	jmp set_leds                                                           ##161# 设置对应的LED指示灯，并返回
                                                                               ##162# 
/*                                                                             ##163# 
 *  curosr-key/numeric keypad cursor keys are handled here.                    ##164# 
 *  checking for numeric keypad etc.                                           ##165# 
 */                                                                            ##166# [r;]下面这段代码的注释可能有误
cursor:                                                                        ##167# [b;]按下小键盘按键(根据num-lock标志作为方向模式(上、下、左、右、Home、End、PgUp、PgDn、Ins、Del)或数字模式(0.123456789)使能)的处理子程序入口
	subb $0x47,%al                                                         ##168# 判断扫描码是否是小键盘上的键发出的——如果大于等于0x47，则是
	jb 1f                                                                  ##169# 如果扫描码小于0x47，说明按下的不是小键盘上的键，则跳转到下面的标号1处直接返回
	cmpb $12,%al                                                           ##170# 
	ja 1f                                                                  ##171# 如果al的值大于0x53(0x47+12)，说明按下的不是小键盘上的键，则跳转到下面的标号1处直接返回
	jne cur2		/* check for ctrl-alt-del */                   ##172# (判断小键盘中的Del(.)键)如果al不等于0x53，说明按下的不是Del键，则跳转到cur2去执行
	testb $0x0c,mode                                                       ##173# (在按下的是Del键的情况下，尝试判断ctrl-alt-del组合键)判断是否有ctrl键按下
	je cur2                                                                ##174# 若没有ctrl键按下，则跳转到cur2去执行
	testb $0x30,mode                                                       ##175# 判断是否有alt键按下
	jne reboot                                                             ##176# 若有alt键按下，则跳转到reboot去热启动系统(即ctrl-alt-del组合键引起重启)
cur2:	cmpb $0x01,e0		/* e0 forces cursor movement */                ##177# (扫描码大于等于0x47，且小于0x53的情况)，判断e0的0xe0标志是否已置位
	je cur                                                                 ##178# [r;]如果e0的0xe0标志已置位，则不论num-lock标志是否已置位，小键盘都是作为方向模式使能，因此跳转到cur去执行
	testb $0x02,leds	/* not num-lock forces cursor */               ##179# [r;](e0的0xe0标志未置位，num-lock标志已置位：小键盘作为数字模式；num-lock标志未置位：小键盘作为方向模式)判断leds中的num-lock标志是否已置位
	je cur                                                                 ##180# 如果num-lock标志未置位，则小键盘作为方向模式，跳转到cur去执行
	testb $0x03,mode	/* shift forces cursor */                      ##181# 判断是否有shift键按下
	jne cur                                                                ##182# 若有shift键按下，则跳转到cur去执行(即shift会使得小键盘从数字模式转换为方向模式)
	xorl %ebx,%ebx                                                         ##183# 清ebx
	movb num_table(%eax),%al                                               ##184# 取得数字模式下小键盘中按下的键对应的字符("789 456 1230."当中的一个)赋给al
	jmp put_queue                                                          ##185# 将al中的字符放入控制台终端读缓冲队列中的缓冲区
1:	ret                                                                    ##186# 返回
                                                                               ##187# 
cur:	movb cur_table(%eax),%al                                               ##188# (小键盘作为方向模式使能时被按下)取得方向模式下小键盘中按下的键对应的字符("HA5 DGC YB623"当中的一个)赋给al
	cmpb $'9,%al                                                           ##189# 将取到的字符与字符'9'比较
	ja ok_cur                                                              ##190# 若取到的字符 > '9'，说明按下的是小键盘中的上、下、左、右、Home或End键，则跳转到ok_cur去执行
	movb $'~,%ah                                                           ##191# (取到的字符 <= '9'，说明按下的是小键盘中的PgUp、PgDn、Ins或Del键)要在功能字符序列中添入字符'~'
ok_cur:	shll $16,%eax                                                          ##192# 将ax的内容移到eax高字中
	movw $0x5b1b,%ax                                                       ##193# 将'ESC''['放入ax中，与eax高字中字符组成移动序列
	xorl %ebx,%ebx                                                         ##194# 清ebx
	jmp put_queue                                                          ##195# 将al中的字符放入控制台终端读缓冲队列中的缓冲区
                                                                               ##196# 
#if defined(KBD_FR)                                                            ##197# 
num_table:                                                                     ##198# 
	.ascii "789 456 1230."                                                 ##199# 法式键盘对应的小键盘在数字模式下的扫描码对应的字符表
#else                                                                          ##200# 
num_table:                                                                     ##201# 
	.ascii "789 456 1230,"                                                 ##202# 其他样式键盘对应的小键盘在数字模式下的扫描码对应的字符表
#endif                                                                         ##203# 
cur_table:                                                                     ##204# 
	.ascii "HA5 DGC YB623"                                                 ##205# 小键盘在方向模式下的扫描码对应的字符表
                                                                               ##206# 
/*                                                                             ##207# 
 * this routine handles function keys                                          ##208# 
 */                                                                            ##209# 
func:                                                                          ##210# 按下功能键(F1-F12)的处理子程序入口
	pushl %eax                                                             ##211# 
	pushl %ecx                                                             ##212# 
	pushl %edx                                                             ##213# 
	call _show_stat                                                        ##214# 调用show_stat()显示64个任务槽中已存在的各个任务的任务号、进程号、进程状态以及内核态栈中空闲(等于0)的字节数
	popl %edx                                                              ##215# 
	popl %ecx                                                              ##216# 
	popl %eax                                                              ##217# 
	subb $0x3B,%al                                                         ##218# 将扫描码al的值减去0x3B(F1的扫描码)
	jb end_func                                                            ##219# 如果扫描码小于0x3B，说明不是功能键，则跳转到end_func直接返回
	cmpb $9,%al                                                            ##220# 将扫描码与(0x3B+9)比较
	jbe ok_func                                                            ##221# 如果扫描码大于等于0x3B，小于等于0x3B+9，说明是功能键(F1-F10中的一个)，则跳转到ok_func去执行
	subb $18,%al                                                           ##222# 
	cmpb $10,%al                                                           ##223# 将扫描码与F11的扫描码0x57(0x3B+18+10)比较
	jb end_func                                                            ##224# 如果扫描码小于0x57，说明不是功能键，则跳转到end_func直接返回
	cmpb $11,%al                                                           ##225# 将扫描码与F12的扫描码0x58(0x3B+18+11)比较
	ja end_func                                                            ##226# 如果扫描码大于0x58，说明不是功能键，则跳转到end_func直接返回
ok_func:                                                                       ##227# 
	cmpl $4,%ecx		/* check that there is enough room */          ##228# [r;]ecx怎么来的？
	jl end_func                                                            ##229# [r;]因为要放入4个字符，所以如果ecx小于4，说明没有足够的空间，则跳转到end_func直接返回
	movl func_table(,%eax,4),%eax                                          ##230# 取得功能键对应的4个字符赋给eax
	xorl %ebx,%ebx                                                         ##231# 清ebx
	jmp put_queue                                                          ##232# 将al中的字符放入控制台终端读缓冲队列中的缓冲区
end_func:                                                                      ##233# 
	ret                                                                    ##234# 返回
                                                                               ##235# 
/*                                                                             ##236# 
 * function keys send F1:'esc [ [ A' F2:'esc [ [ B' etc.                       ##237# 
 */                                                                            ##238# 
func_table:                                                                    ##239# 功能键的扫描码对应的字符表
	.long 0x415b5b1b,0x425b5b1b,0x435b5b1b,0x445b5b1b                      ##240# 
	.long 0x455b5b1b,0x465b5b1b,0x475b5b1b,0x485b5b1b                      ##241# 
	.long 0x495b5b1b,0x4a5b5b1b,0x4b5b5b1b,0x4c5b5b1b                      ##242# 
                                                                               ##243# 
#if	defined(KBD_FINNISH)                                                   ##244# 254-291行为芬兰键盘的扫描码映射表
key_map:                                                                       ##245# 普通映射表
	.byte 0,27                                                             ##246# 
	.ascii "1234567890+'"                                                  ##247# 
	.byte 127,9                                                            ##248# 
	.ascii "qwertyuiop}"                                                   ##249# 
	.byte 0,13,0                                                           ##250# 
	.ascii "asdfghjkl|{"                                                   ##251# 
	.byte 0,0                                                              ##252# 
	.ascii "'zxcvbnm,.-"                                                   ##253# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##254# 
	.fill 16,1,0		/* 3A-49 */                                    ##255# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##256# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##257# 
	.byte '<                                                               ##258# 
	.fill 10,1,0                                                           ##259# 
                                                                               ##260# 
shift_map:                                                                     ##261# shift键同时按下时的映射表
	.byte 0,27                                                             ##262# 
	.ascii "!\"#$%&/()=?`"                                                 ##263# 
	.byte 127,9                                                            ##264# 
	.ascii "QWERTYUIOP]^"                                                  ##265# 
	.byte 13,0                                                             ##266# 
	.ascii "ASDFGHJKL\\["                                                  ##267# 
	.byte 0,0                                                              ##268# 
	.ascii "*ZXCVBNM;:_"                                                   ##269# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##270# 
	.fill 16,1,0		/* 3A-49 */                                    ##271# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##272# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##273# 
	.byte '>                                                               ##274# 
	.fill 10,1,0                                                           ##275# 
                                                                               ##276# 
alt_map:                                                                       ##277# 右alt键同时按下时的映射表
	.byte 0,0                                                              ##278# 
	.ascii "\0@\0$\0\0{[]}\\\0"                                            ##279# 
	.byte 0,0                                                              ##280# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##281# 
	.byte '~,13,0                                                          ##282# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##283# 
	.byte 0,0                                                              ##284# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##285# 
	.byte 0,0,0,0		/* 36-39 */                                    ##286# 
	.fill 16,1,0		/* 3A-49 */                                    ##287# 
	.byte 0,0,0,0,0		/* 4A-4E */                                    ##288# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##289# 
	.byte '|                                                               ##290# 
	.fill 10,1,0                                                           ##291# 
                                                                               ##292# 
#elif defined(KBD_US)                                                          ##293# 295-342行为美式键盘的扫描码映射表
                                                                               ##294# 
key_map:                                                                       ##295# 普通映射表
	.byte 0,27                                                             ##296# 
	.ascii "1234567890-="                                                  ##297# 
	.byte 127,9                                                            ##298# 
	.ascii "qwertyuiop[]"                                                  ##299# 
	.byte 13,0                                                             ##300# 
	.ascii "asdfghjkl;'"                                                   ##301# 
	.byte '`,0                                                             ##302# 
	.ascii "\\zxcvbnm,./"                                                  ##303# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##304# 
	.fill 16,1,0		/* 3A-49 */                                    ##305# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##306# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##307# 
	.byte '<                                                               ##308# 
	.fill 10,1,0                                                           ##309# 
                                                                               ##310# 
                                                                               ##311# 
shift_map:                                                                     ##312# shift键同时按下时的映射表
	.byte 0,27                                                             ##313# 
	.ascii "!@#$%^&*()_+"                                                  ##314# 
	.byte 127,9                                                            ##315# 
	.ascii "QWERTYUIOP{}"                                                  ##316# 
	.byte 13,0                                                             ##317# 
	.ascii "ASDFGHJKL:\""                                                  ##318# 
	.byte '~,0                                                             ##319# 
	.ascii "|ZXCVBNM<>?"                                                   ##320# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##321# 
	.fill 16,1,0		/* 3A-49 */                                    ##322# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##323# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##324# 
	.byte '>                                                               ##325# 
	.fill 10,1,0                                                           ##326# 
                                                                               ##327# 
alt_map:                                                                       ##328# 右alt键同时按下时的映射表
	.byte 0,0                                                              ##329# 
	.ascii "\0@\0$\0\0{[]}\\\0"                                            ##330# 
	.byte 0,0                                                              ##331# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##332# 
	.byte '~,13,0                                                          ##333# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##334# 
	.byte 0,0                                                              ##335# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##336# 
	.byte 0,0,0,0		/* 36-39 */                                    ##337# 
	.fill 16,1,0		/* 3A-49 */                                    ##338# 
	.byte 0,0,0,0,0		/* 4A-4E */                                    ##339# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##340# 
	.byte '|                                                               ##341# 
	.fill 10,1,0                                                           ##342# 
                                                                               ##343# 
#elif defined(KBD_GR)                                                          ##344# 346-393行为德式键盘的扫描码映射表
                                                                               ##345# 
key_map:                                                                       ##346# 普通映射表
	.byte 0,27                                                             ##347# 
	.ascii "1234567890\\'"                                                 ##348# 
	.byte 127,9                                                            ##349# 
	.ascii "qwertzuiop@+"                                                  ##350# 
	.byte 13,0                                                             ##351# 
	.ascii "asdfghjkl[]^"                                                  ##352# 
	.byte 0,'#                                                             ##353# 
	.ascii "yxcvbnm,.-"                                                    ##354# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##355# 
	.fill 16,1,0		/* 3A-49 */                                    ##356# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##357# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##358# 
	.byte '<                                                               ##359# 
	.fill 10,1,0                                                           ##360# 
                                                                               ##361# 
                                                                               ##362# 
shift_map:                                                                     ##363# shift键同时按下时的映射表
	.byte 0,27                                                             ##364# 
	.ascii "!\"#$%&/()=?`"                                                 ##365# 
	.byte 127,9                                                            ##366# 
	.ascii "QWERTZUIOP\\*"                                                 ##367# 
	.byte 13,0                                                             ##368# 
	.ascii "ASDFGHJKL{}~"                                                  ##369# 
	.byte 0,''                                                             ##370# 
	.ascii "YXCVBNM;:_"                                                    ##371# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##372# 
	.fill 16,1,0		/* 3A-49 */                                    ##373# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##374# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##375# 
	.byte '>                                                               ##376# 
	.fill 10,1,0                                                           ##377# 
                                                                               ##378# 
alt_map:                                                                       ##379# 右alt键同时按下时的映射表
	.byte 0,0                                                              ##380# 
	.ascii "\0@\0$\0\0{[]}\\\0"                                            ##381# 
	.byte 0,0                                                              ##382# 
	.byte '@,0,0,0,0,0,0,0,0,0,0                                           ##383# 
	.byte '~,13,0                                                          ##384# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##385# 
	.byte 0,0                                                              ##386# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##387# 
	.byte 0,0,0,0		/* 36-39 */                                    ##388# 
	.fill 16,1,0		/* 3A-49 */                                    ##389# 
	.byte 0,0,0,0,0		/* 4A-4E */                                    ##390# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##391# 
	.byte '|                                                               ##392# 
	.fill 10,1,0                                                           ##393# 
                                                                               ##394# 
                                                                               ##395# 
#elif defined(KBD_FR)                                                          ##396# 398-444行为法式键盘的扫描码映射表
                                                                               ##397# 
key_map:                                                                       ##398# 普通映射表
	.byte 0,27                                                             ##399# 
	.ascii "&{\"'(-}_/@)="                                                 ##400# 
	.byte 127,9                                                            ##401# 
	.ascii "azertyuiop^$"                                                  ##402# 
	.byte 13,0                                                             ##403# 
	.ascii "qsdfghjklm|"                                                   ##404# 
	.byte '`,0,42		/* coin sup gauche, don't know, [*|mu] */      ##405# 
	.ascii "wxcvbn,;:!"                                                    ##406# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##407# 
	.fill 16,1,0		/* 3A-49 */                                    ##408# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##409# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##410# 
	.byte '<                                                               ##411# 
	.fill 10,1,0                                                           ##412# 
                                                                               ##413# 
shift_map:                                                                     ##414# shift键同时按下时的映射表
	.byte 0,27                                                             ##415# 
	.ascii "1234567890]+"                                                  ##416# 
	.byte 127,9                                                            ##417# 
	.ascii "AZERTYUIOP<>"                                                  ##418# 
	.byte 13,0                                                             ##419# 
	.ascii "QSDFGHJKLM%"                                                   ##420# 
	.byte '~,0,'#                                                          ##421# 
	.ascii "WXCVBN?./\\"                                                   ##422# 
	.byte 0,'*,0,32		/* 36-39 */                                    ##423# 
	.fill 16,1,0		/* 3A-49 */                                    ##424# 
	.byte '-,0,0,0,'+	/* 4A-4E */                                    ##425# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##426# 
	.byte '>                                                               ##427# 
	.fill 10,1,0                                                           ##428# 
                                                                               ##429# 
alt_map:                                                                       ##430# 右alt键同时按下时的映射表
	.byte 0,0                                                              ##431# 
	.ascii "\0~#{[|`\\^@]}"                                                ##432# 
	.byte 0,0                                                              ##433# 
	.byte '@,0,0,0,0,0,0,0,0,0,0                                           ##434# 
	.byte '~,13,0                                                          ##435# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##436# 
	.byte 0,0                                                              ##437# 
	.byte 0,0,0,0,0,0,0,0,0,0,0                                            ##438# 
	.byte 0,0,0,0		/* 36-39 */                                    ##439# 
	.fill 16,1,0		/* 3A-49 */                                    ##440# 
	.byte 0,0,0,0,0		/* 4A-4E */                                    ##441# 
	.byte 0,0,0,0,0,0,0	/* 4F-55 */                                    ##442# 
	.byte '|                                                               ##443# 
	.fill 10,1,0                                                           ##444# 
                                                                               ##445# 
#else                                                                          ##446# 
#error "KBD-type not defined"                                                  ##447# 
#endif                                                                         ##448# 
/*                                                                             ##449# 
 * do_self handles "normal" keys, ie keys that don't change meaning            ##450# 
 * and which have just one character returns.                                  ##451# 
 */                                                                            ##452# 
do_self:                                                                       ##453# "普通"键(即含义没有变换，且只有一个字符返回的键)的处理子程序入口
	lea alt_map,%ebx                                                       ##454# 将右alt键同时按下时的映射表基地址alt_map赋给ebx
	testb $0x20,mode		/* alt-gr */                           ##455# 判断右alt键是否同时按下了
	jne 1f                                                                 ##456# 如果右alt键按下了，则跳转到上面的标号1处直接返回
	lea shift_map,%ebx                                                     ##457# 将shift键同时按下时的映射表基地址shift_map赋给ebx
	testb $0x03,mode                                                       ##458# 判断是否有shift键同时按下了
	jne 1f                                                                 ##459# 如果有shift键按下了，则跳转到上面的标号1处执行直接返回
	lea key_map,%ebx                                                       ##460# (既没有shift键，也没有右alt键按下)将普通键映射表基地址key_map赋给ebx
1:	movb (%ebx,%eax),%al                                                   ##461# 通过al中的扫描码找到普通键映射表中的字符赋给al
	orb %al,%al                                                            ##462# 判断是否找到了对应的字符
	je none                                                                ##463# 如果al=0，说明没有找到对应的字符，则跳转到标号none处直接返回
	testb $0x4c,mode		/* ctrl or caps */                     ##464# (找到了普通键对应的字符)464-470行用于保证在(有ctrl键已按下或caps键已锁定、并且字符在'a'-'}'(大于等于0x61小于等于0x7D)范围内)的情况下，将字符值减去32，即转换为大写字符(其中"{|}"被转换为"[\]")，否则直接跳转到下面的标号2处执行
	je 2f                                                                  ##465# 
	cmpb $'a,%al                                                           ##466# 
	jb 2f                                                                  ##467# 
	cmpb $'},%al                                                           ##468# 
	ja 2f                                                                  ##469# 
	subb $32,%al                                                           ##470# 
2:	testb $0x0c,mode		/* ctrl */                             ##471# 471-477行用于保证在(有ctrl键已按下、并且字符在'@'-'_'(大于等于0x40小于等于0x5F)范围内)的情况下，将字符值减去64，即转换为控制字符，否则直接跳转到下面的标号3处执行
	je 3f                                                                  ##472# 
	cmpb $64,%al                                                           ##473# 
	jb 3f                                                                  ##474# 
	cmpb $64+32,%al                                                        ##475# 
	jae 3f                                                                 ##476# 
	subb $64,%al                                                           ##477# 
3:	testb $0x10,mode		/* left alt */                         ##478# 478-480行用于保证在(有左ctrl键已按下)的情况下，将字符的位7置位，即生成值大于0x7f的扩展字符集中的字符，否则直接跳转到下面的标号4处执行
	je 4f                                                                  ##479# 
	orb $0x80,%al                                                          ##480# 
4:	andl $0xff,%eax                                                        ##481# 清eax的高字和ah
	xorl %ebx,%ebx                                                         ##482# 清ebx
	call put_queue                                                         ##483# 调用put_queue将al中的字符放入控制台终端读缓冲队列中的缓冲区
none:	ret                                                                    ##484# 返回
                                                                               ##485# 
/*                                                                             ##486# 
 * minus has a routine of it's own, as a 'E0h' before                          ##487# 
 * the scan code for minus means that the numeric keypad                       ##488# 
 * slash was pushed.                                                           ##489# 
 */                                                                            ##490# 
minus:	cmpb $1,e0                                                             ##491# 判断e0中的0xe0标志是否置位
	jne do_self                                                            ##492# 如果没有置位，则跳转到do_self对减号进行普通处理
	movl $'/,%eax                                                          ##493# (0xe0标志已置位)用'/'替换'-'，放入al中
	xorl %ebx,%ebx                                                         ##494# 清ebx
	jmp put_queue                                                          ##495# 将al中的字符放入控制台终端读缓冲队列中的缓冲区
                                                                               ##496# 
/*                                                                             ##497# 
 * This table decides which routine to call when a scan-code has been          ##498# 
 * gotten. Most routines just call do_self, or none, depending if              ##499# 
 * they are make or break.                                                     ##500# 
 */                                                                            ##501# 
key_table:                                                                     ##502# 
	.long none,do_self,do_self,do_self	/* 00-03 s0 esc 1 2 */         ##503# 
	.long do_self,do_self,do_self,do_self	/* 04-07 3 4 5 6 */            ##504# 
	.long do_self,do_self,do_self,do_self	/* 08-0B 7 8 9 0 */            ##505# 
	.long do_self,do_self,do_self,do_self	/* 0C-0F + ' bs tab */         ##506# 
	.long do_self,do_self,do_self,do_self	/* 10-13 q w e r */            ##507# 
	.long do_self,do_self,do_self,do_self	/* 14-17 t y u i */            ##508# 
	.long do_self,do_self,do_self,do_self	/* 18-1B o p } ^ */            ##509# 
	.long do_self,ctrl,do_self,do_self	/* 1C-1F enter ctrl a s */     ##510# 
	.long do_self,do_self,do_self,do_self	/* 20-23 d f g h */            ##511# 
	.long do_self,do_self,do_self,do_self	/* 24-27 j k l | */            ##512# 
	.long do_self,do_self,lshift,do_self	/* 28-2B { para lshift , */    ##513# 
	.long do_self,do_self,do_self,do_self	/* 2C-2F z x c v */            ##514# 
	.long do_self,do_self,do_self,do_self	/* 30-33 b n m , */            ##515# 
	.long do_self,minus,rshift,do_self	/* 34-37 . - rshift * */       ##516# 
	.long alt,do_self,caps,func		/* 38-3B alt sp caps f1 */     ##517# 
	.long func,func,func,func		/* 3C-3F f2 f3 f4 f5 */        ##518# 
	.long func,func,func,func		/* 40-43 f6 f7 f8 f9 */        ##519# 
	.long func,num,scroll,cursor		/* 44-47 f10 num scr home */   ##520# 
	.long cursor,cursor,do_self,cursor	/* 48-4B up pgup - left */     ##521# 
	.long cursor,cursor,do_self,cursor	/* 4C-4F n5 right + end */     ##522# 
	.long cursor,cursor,cursor,cursor	/* 50-53 dn pgdn ins del */    ##523# 
	.long none,none,do_self,func		/* 54-57 sysreq ? < f11 */     ##524# 
	.long func,none,none,none		/* 58-5B f12 ? ? ? */          ##525# 
	.long none,none,none,none		/* 5C-5F ? ? ? ? */            ##526# 
	.long none,none,none,none		/* 60-63 ? ? ? ? */            ##527# 
	.long none,none,none,none		/* 64-67 ? ? ? ? */            ##528# 
	.long none,none,none,none		/* 68-6B ? ? ? ? */            ##529# 
	.long none,none,none,none		/* 6C-6F ? ? ? ? */            ##530# 
	.long none,none,none,none		/* 70-73 ? ? ? ? */            ##531# 
	.long none,none,none,none		/* 74-77 ? ? ? ? */            ##532# 
	.long none,none,none,none		/* 78-7B ? ? ? ? */            ##533# 
	.long none,none,none,none		/* 7C-7F ? ? ? ? */            ##534# 
	.long none,none,none,none		/* 80-83 ? br br br */         ##535# 
	.long none,none,none,none		/* 84-87 br br br br */        ##536# 
	.long none,none,none,none		/* 88-8B br br br br */        ##537# 
	.long none,none,none,none		/* 8C-8F br br br br */        ##538# 
	.long none,none,none,none		/* 90-93 br br br br */        ##539# 
	.long none,none,none,none		/* 94-97 br br br br */        ##540# 
	.long none,none,none,none		/* 98-9B br br br br */        ##541# 
	.long none,unctrl,none,none		/* 9C-9F br unctrl br br */    ##542# 
	.long none,none,none,none		/* A0-A3 br br br br */        ##543# 
	.long none,none,none,none		/* A4-A7 br br br br */        ##544# 
	.long none,none,unlshift,none		/* A8-AB br br unlshift br */  ##545# 
	.long none,none,none,none		/* AC-AF br br br br */        ##546# 
	.long none,none,none,none		/* B0-B3 br br br br */        ##547# 
	.long none,none,unrshift,none		/* B4-B7 br br unrshift br */  ##548# 
	.long unalt,none,uncaps,none		/* B8-BB unalt br uncaps br */ ##549# 
	.long none,none,none,none		/* BC-BF br br br br */        ##550# 
	.long none,none,none,none		/* C0-C3 br br br br */        ##551# 
	.long none,none,none,none		/* C4-C7 br br br br */        ##552# 
	.long none,none,none,none		/* C8-CB br br br br */        ##553# 
	.long none,none,none,none		/* CC-CF br br br br */        ##554# 
	.long none,none,none,none		/* D0-D3 br br br br */        ##555# 
	.long none,none,none,none		/* D4-D7 br br br br */        ##556# 
	.long none,none,none,none		/* D8-DB br ? ? ? */           ##557# 
	.long none,none,none,none		/* DC-DF ? ? ? ? */            ##558# 
	.long none,none,none,none		/* E0-E3 e0 e1 ? ? */          ##559# 
	.long none,none,none,none		/* E4-E7 ? ? ? ? */            ##560# 
	.long none,none,none,none		/* E8-EB ? ? ? ? */            ##561# 
	.long none,none,none,none		/* EC-EF ? ? ? ? */            ##562# 
	.long none,none,none,none		/* F0-F3 ? ? ? ? */            ##563# 
	.long none,none,none,none		/* F4-F7 ? ? ? ? */            ##564# 
	.long none,none,none,none		/* F8-FB ? ? ? ? */            ##565# 
	.long none,none,none,none		/* FC-FF ? ? ? ? */            ##566# 
                                                                               ##567# 
/*                                                                             ##568# 
 * kb_wait waits for the keyboard controller buffer to empty.                  ##569# 
 * there is no timeout - if the buffer doesn't empty, we hang.                 ##570# 
 */                                                                            ##571# 
kb_wait:                                                                       ##572# 循环判断键盘控制器中的输入缓冲器是否已满，直至发现未满才返回
	pushl %eax                                                             ##573# 
1:	inb $0x64,%al                                                          ##574# 读出键盘控制器状态寄存器的值放入al中
	testb $0x02,%al                                                        ##575# 判断位1是否已置位，即判断输入缓冲器是否已满
	jne 1b                                                                 ##576# 若位1已置位，说明输入缓冲器已满，则跳转会标号1处重新判断
	popl %eax                                                              ##577# 
	ret                                                                    ##578# 返回(若输入缓冲器未满)
/*                                                                             ##579# 
 * This routine reboots the machine by asking the keyboard                     ##580# 
 * controller to pulse the reset-line low.                                     ##581# 
 */                                                                            ##582# 
reboot:                                                                        ##583# 使系统热启动
	call kb_wait                                                           ##584# 调用kb_wait循环判断键盘控制器中的输入缓冲器是否已满，直至发现未满才从kb_wait返回
	movw $0x1234,0x472	/* don't do memory check */                    ##585# 将0x1234写入内存物理地址0x472处(启动模式标志所在处)，使得系统在启动过程中，BIOS会读取到该值0x1234，便会跳过内存检测过程而执行热启动过程。若BIOS读到的值为0，则执行冷启动过程
	movb $0xfc,%al		/* pulse reset and A20 low */                  ##586# 
	outb %al,$0x64                                                         ##587# 将0xfc命令(送脉冲到输出端口，使得输出端口P23-P20的值为1100(0xc))写入键盘控制器的0x64端口，额A20不选通，且系统复位重启
die:	jmp die                                                                ##588# 死机

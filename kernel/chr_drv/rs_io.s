/*                                                                            ##  1# 
 *  linux/kernel/rs_io.s                                                      ##  2# 
 *                                                                            ##  3# 
 *  (C) 1991  Linus Torvalds                                                  ##  4# 
 */                                                                           ##  5# 
                                                                              ##  6# 
/*                                                                            ##  7# 
 *	rs_io.s                                                               ##  8# 
 *                                                                            ##  9# 
 * This module implements the rs232 io interrupts.                            ## 10# 
 */                                                                           ## 11# 
                                                                              ## 12# 
.text                                                                         ## 13# 
.globl _rs1_interrupt,_rs2_interrupt                                          ## 14# 定义两个全局符号，即使不用也强制引入
                                                                              ## 15# 
size	= 1024				/* must be power of two !             ## 16# 串口缓冲队列的队列缓冲区长度(字节数)，即串口对应的tty结构中的tty读写缓冲队列的队列缓冲区长度
					   and must match the value           ## 17# 
					   in tty_io.c!!! */                  ## 18# 
                                                                              ## 19# 
/* these are the offsets into the read/write buffer structures */             ## 20# 
rs_addr = 0                                                                   ## 21# 串口缓冲队列中串行端口基地址字段在串口缓冲队列结构tty_queue中的偏移量
head = 4                                                                      ## 22# 串口缓冲队列中数据头偏移量字段在串口缓冲队列结构tty_queue中的偏移量
tail = 8                                                                      ## 23# 串口缓冲队列中数据尾偏移量字段在串口缓冲队列结构tty_queue中的偏移量
proc_list = 12                                                                ## 24# 串口缓冲队列中等待进程列表字段在串口缓冲队列结构tty_queue中的偏移量
buf = 16                                                                      ## 25# 串口缓冲队列中缓冲区字段在串口缓冲队列结构tty_queue中的偏移量
                                                                              ## 26# 
startup	= 256		/* chars left in write queue when we restart it */    ## 27# 当串口的写缓冲队列的队列缓冲区满了之后，内核会将写进程置为等待状态，当队列缓冲区里面不到256个字符时，则唤醒写进程继续往里写
                                                                              ## 28# 
/*                                                                            ## 29# 
 * These are the actual interrupt routines. They look where                   ## 30# 
 * the interrupt is coming from, and take appropriate action.                 ## 31# 
 */                                                                           ## 32# 
.align 2                                                                      ## 33# 
_rs1_interrupt:                                                               ## 34# 串口1的中断处理程序
	pushl $_table_list+8                                                  ## 35# 将tty读写缓冲队列结构地址表中的串口1读缓冲队列地址的地址压栈
	jmp rs_int                                                            ## 36# 跳转到rs_int
.align 2                                                                      ## 37# 
_rs2_interrupt:                                                               ## 38# 串口2的中断处理程序
	pushl $_table_list+16                                                 ## 39# 将tty读写缓冲队列结构地址表中的串口2读缓冲队列地址的地址压栈
rs_int:                                                                       ## 40# 
	pushl %edx                                                            ## 41# 
	pushl %ecx                                                            ## 42# 
	pushl %ebx                                                            ## 43# 
	pushl %eax                                                            ## 44# 
	push %es                                                              ## 45# 
	push %ds		/* as this is an interrupt, we cannot */      ## 46# 
	pushl $0x10		/* know that bs is ok. Load it */             ## 47# 47-50行用于将es、ds指向内核数据段
	pop %ds                                                               ## 48# 
	pushl $0x10                                                           ## 49# 
	pop %es                                                               ## 50# 
	movl 24(%esp),%edx                                                    ## 51# 取得压入栈中的tty读写缓冲队列结构地址表中的串口读缓冲队列地址的地址赋给edx
	movl (%edx),%edx                                                      ## 52# 将edx指向的串口读缓冲队列地址赋给edx
	movl rs_addr(%edx),%edx                                               ## 53# 将串口读缓冲队列中串行端口基地址字段赋给edx
	addl $2,%edx		/* interrupt ident. reg */                    ## 54# 将edx中的端口指向UART中断标识寄存器
rep_int:                                                                      ## 55# [b;]下面开始判断中断来源
	xorl %eax,%eax                                                        ## 56# 
	inb %dx,%al                                                           ## 57# 读出UART中断标识寄存器的内容到al中
	testb $1,%al                                                          ## 58# 测试UART中断标识寄存器的位0是否置位
	jne end                                                               ## 59# 如果位0已置位，说明无待处理中断，则跳转到end执行
	cmpb $6,%al		/* this shouldn't happen, but ... */          ## 60# (位0为0，有待处理中断存在)，判断UART中断标识寄存器的值是否在有效范围内
	ja end                                                                ## 61# 如果超过了有效范围，即值大于6，则跳转到end执行
	movl 24(%esp),%ecx                                                    ## 62# (UART中断标识寄存器的值在有效范围内)取得压入栈中的tty读写缓冲队列结构地址表中的串口读缓冲队列地址的地址赋给ecx
	pushl %edx                                                            ## 63# 压栈——将edx中的UART中断标识寄存器的端口临时压栈
	subl $2,%edx                                                          ## 64# 再将串口读缓冲队列中串行端口基地址赋给edx
	call jmp_table(,%eax,2)		/* NOTE! not *4, bit0 is 0 already */ ## 65# 根据al中UART中断标识寄存器的值(因为位0是1，相当于已经乘过2了)，确定中断源，跳转jmp_table对应的处理程序入口
	popl %edx                                                             ## 66# 出栈——将UART中断标识寄存器的端口赋给edx
	jmp rep_int                                                           ## 67# 跳转回rep_int，继续判断有无待处理中断并作相应处理
end:	movb $0x20,%al                                                        ## 68# (中断退出处理)
	outb %al,$0x20		/* EOI */                                     ## 69# 向8259A主芯片发送EOI(中断结束)信号
	pop %ds                                                               ## 70# 
	pop %es                                                               ## 71# 
	popl %eax                                                             ## 72# 
	popl %ebx                                                             ## 73# 
	popl %ecx                                                             ## 74# 
	popl %edx                                                             ## 75# 
	addl $4,%esp		# jump over _table_list entry                 ## 76# 将栈中的tty读写缓冲队列结构地址表中的串口读缓冲队列地址的地址丢弃
	iret                                                                  ## 77# 中断返回
                                                                              ## 78# 
jmp_table:                                                                    ## 79# 各中断类型处理子程序地址跳转表，共4种中断来源
	.long modem_status,write_char,read_char,line_status                   ## 80# MODEM状态改变中断、发送保持寄存器空中断、已接受到数据中断、接受状态有错中断
                                                                              ## 81# 
.align 2                                                                      ## 82# 
modem_status:                                                                 ## 83# MODEM状态改变中断处理子程序入口
	addl $6,%edx		/* clear intr by reading modem status reg */  ## 84# 
	inb %dx,%al                                                           ## 85# 读UARTMODEM状态寄存器进行复位
	ret                                                                   ## 86# 返回
                                                                              ## 87# 
.align 2                                                                      ## 88# 
line_status:                                                                  ## 89# 接受状态有错中断处理子程序入口
	addl $5,%edx		/* clear intr by reading line status reg. */  ## 90# 
	inb %dx,%al                                                           ## 91# 读UART线路状态寄存器进行复位
	ret                                                                   ## 92# 返回
                                                                              ## 93# 
.align 2                                                                      ## 94# 
read_char:                                                                    ## 95# 已接受到数据中断处理子程序入口
	inb %dx,%al                                                           ## 96# 读取UART接受缓存寄存器中的字符到al中
	movl %ecx,%edx                                                        ## 97# 将tty读写缓冲队列结构地址表中的串口读缓冲队列地址的地址赋给edx
	subl $_table_list,%edx                                                ## 98# 计算出 串口读缓冲队列地址 相对于 tty读写缓冲队列结构地址表首地址 的偏移(字节数)赋给edx
	shrl $3,%edx                                                          ## 99# 计算出 串口读缓冲队列地址 在 tty读写缓冲队列结构地址表 中的下标赋给edx
	movl (%ecx),%ecx		# read-queue                          ##100# 将tty读写缓冲队列结构地址表中的串口读缓冲队列地址赋给ecx
	movl head(%ecx),%ebx                                                  ##101# 将串口读缓冲队列结构tty_queue中的数据头偏移量字段值赋给ebx
	movb %al,buf(%ecx,%ebx)                                               ##102# 将读取到的字符放入串口读缓冲队列中的队列缓冲区中数据头偏移量指定的位置
	incl %ebx                                                             ##103# 将ebx中的数据头偏移量字段值加1
	andl $size-1,%ebx                                                     ##104# 如果加1过后的数据头偏移量字段超过了缓冲区长度，则环回到缓冲区开头
	cmpl tail(%ecx),%ebx                                                  ##105# 将数据头偏移量与数据尾偏移量进行比较
	je 1f                                                                 ##106# 如果相等，表示串口读缓冲队列中的队列缓冲区已满，则跳转到下面的标号1处执行
	movl %ebx,head(%ecx)                                                  ##107# (串口读缓冲队列中的队列缓冲区未满)将ebx中的新的数据头偏移量字段值写回到串口读缓冲队列中
1:	pushl %edx                                                            ##108# 临时压栈——将串口读缓冲队列地址在tty读写缓冲队列结构地址表中的下标作为串口号压入栈中
	call _do_tty_interrupt                                                ##109# 调用tty中断处理C函数(tty_io.c，342行)
	addl $4,%esp                                                          ##110# 临时出栈——将栈中串口读缓冲队列地址在tty读写缓冲队列结构地址表中的下标丢弃
	ret                                                                   ##111# 返回
                                                                              ##112# 
.align 2                                                                      ##113# 
write_char:                                                                   ##114# 发送保持寄存器空中断处理子程序入口
	movl 4(%ecx),%ecx		# write-queue                         ##115# 将tty读写缓冲队列结构地址表中的串口写缓冲队列地址赋给ecx
	movl head(%ecx),%ebx                                                  ##116# 将串口写缓冲队列结构tty_queue中的数据头偏移量字段值赋给ebx
	subl tail(%ecx),%ebx                                                  ##117# 计算出串口写缓冲队列中队列缓冲区中的字符数赋给ebx
	andl $size-1,%ebx		# nr chars in queue                   ##118# 通过与运算判断串口写缓冲队列中队列缓冲区中的字符数是否为0
	je write_buffer_empty                                                 ##119# 如果字符数为0，则跳转到write_buffer_empty执行
	cmpl $startup,%ebx                                                    ##120# (字符数不为0)将字符数与startup=256进行比较
	ja 1f                                                                 ##121# 如果字符数超过256，则跳转到下面的标号1处执行
	movl proc_list(%ecx),%ebx	# wake up sleeping process            ##122# (字符数小于等于256)将串口写缓冲队列结构tty_queue中的等待进程列表指针字段赋给ebx
	testl %ebx,%ebx			# is there any?                       ##123# 判断该等待进程列表指针是否为空指针
	je 1f                                                                 ##124# 如果是空指针，说明没有进程在睡眠等待，则跳转到下面的标号1处执行
	movl $0,(%ebx)                                                        ##125# (该等待进程列表指针不为空)将该等待进程置为可运行状态，即唤醒该进程
1:	movl tail(%ecx),%ebx                                                  ##126# 将串口写缓冲队列结构tty_queue中的数据尾偏移量字段值赋给ebx
	movb buf(%ecx,%ebx),%al                                               ##127# 将串口写缓冲队列中的队列缓冲区中数据尾偏移量指定的位置的字符写入al中
	outb %al,%dx                                                          ##128# 将al中的字符写入到UART发送保持寄存器中
	incl %ebx                                                             ##129# 将ebx中的数据尾偏移量字段加1
	andl $size-1,%ebx                                                     ##130# 如果加1过后的数据尾偏移量字段超过了缓冲区长度，则环回到缓冲区开头
	movl %ebx,tail(%ecx)                                                  ##131# 将ebx中的新的数据尾偏移量字段值写回到串口写缓冲队列中
	cmpl head(%ecx),%ebx                                                  ##132# 将数据头偏移量与数据尾偏移量进行比较
	je write_buffer_empty                                                 ##133# 如果相等，表示串口写缓冲队列的队列缓冲区为空，跳转到write_buffer_empty执行
	ret                                                                   ##134# 返回
.align 2                                                                      ##135# 
write_buffer_empty:                                                           ##136# [b;]处理串口写缓冲队列的队列缓冲区为空的情况
	movl proc_list(%ecx),%ebx	# wake up sleeping process            ##137# 将串口写缓冲队列结构tty_queue中的等待进程列表指针字段赋给ebx
	testl %ebx,%ebx			# is there any?                       ##138# 判断该等待进程列表指针是否为空指针
	je 1f                                                                 ##139# 如果是空指针，说明没有进程在睡眠等待，则跳转到下面的标号1处执行
	movl $0,(%ebx)                                                        ##140# (该等待进程列表指针不为空)将该等待进程置为可运行状态，即唤醒该进程
1:	incl %edx                                                             ##141# 将edx中的端口指向UART中断允许寄存器
	inb %dx,%al                                                           ##142# 读取UART中断允许寄存器的值到al中
	jmp 1f                                                                ##143# 143-144用于延迟等待一会
1:	jmp 1f                                                                ##144# 
1:	andb $0xd,%al		/* disable transmit interrupt */              ##145# 将al中的UART中断允许寄存器值的位1复位(即禁止发送保持寄存器空中断)
	outb %al,%dx                                                          ##146# 将al中的新值写回UART中断允许寄存器中
	ret                                                                   ##147# 返回

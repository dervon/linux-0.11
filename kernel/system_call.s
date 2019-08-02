/*                                                                          ##  1# 
 *  linux/kernel/system_call.s                                              ##  2# 
 *                                                                          ##  3# 
 *  (C) 1991  Linus Torvalds                                                ##  4# 
 */                                                                         ##  5# 
                                                                            ##  6# 
/*                                                                          ##  7# 
 *  system_call.s  contains the system-call low-level handling routines.    ##  8# 
 * This also contains the timer-interrupt handler, as some of the code is   ##  9# 
 * the same. The hd- and flopppy-interrupts are also here.                  ## 10# 
 *                                                                          ## 11# 
 * NOTE: This code handles signal-recognition, which happens every time     ## 12# 
 * after a timer-interrupt and after each system call. Ordinary interrupts  ## 13# 
 * don't handle signal-recognition, as that would clutter them up totally   ## 14# 
 * unnecessarily.                                                           ## 15# 
 *                                                                          ## 16# 
 * Stack layout in 'ret_from_system_call':                                  ## 17# 
 *                                                                          ## 18# 
 *	 0(%esp) - %eax                                                     ## 19# 
 *	 4(%esp) - %ebx                                                     ## 20# 
 *	 8(%esp) - %ecx                                                     ## 21# 
 *	 C(%esp) - %edx                                                     ## 22# 
 *	10(%esp) - %fs                                                      ## 23# 
 *	14(%esp) - %es                                                      ## 24# 
 *	18(%esp) - %ds                                                      ## 25# 
 *	1C(%esp) - %eip                                                     ## 26# 
 *	20(%esp) - %cs                                                      ## 27# 
 *	24(%esp) - %eflags                                                  ## 28# 
 *	28(%esp) - %oldesp                                                  ## 29# 
 *	2C(%esp) - %oldss                                                   ## 30# 
 */                                                                         ## 31# 
                                                                            ## 32# 
SIG_CHLD	= 17                                                        ## 33# [r;]定义SIG_CHLD信号(子进程停止或结束)，似乎未用到
                                                                            ## 34# 
EAX		= 0x00                                                      ## 35# 35-46行是从系统调用返回刚进入ret_from_system_call时栈中的内容的对应偏移量
EBX		= 0x04                                                      ## 36# 
ECX		= 0x08                                                      ## 37# 
EDX		= 0x0C                                                      ## 38# 
FS		= 0x10                                                      ## 39# 
ES		= 0x14                                                      ## 40# 
DS		= 0x18                                                      ## 41# 
EIP		= 0x1C                                                      ## 42# 
CS		= 0x20                                                      ## 43# 
EFLAGS		= 0x24                                                      ## 44# 
OLDESP		= 0x28                                                      ## 45# 
OLDSS		= 0x2C                                                      ## 46# 
                                                                            ## 47# 48-53行为任务结构中变量对应的偏移值
state	= 0		# these are offsets into the task-struct.           ## 48# 进程状态码
counter	= 4                                                                 ## 49# 运行时间片，任务运行时间计数(递减)(滴答值)
priority = 8                                                                ## 50# 运行优先数，任务开始运行时counter = priority，越大则运行时间越长
signal	= 12                                                                ## 51# 信号位图，每个比特位代表一种信号，信号值=位偏移值+1
sigaction = 16		# MUST be 16 (=len of sigaction)                    ## 52# 信号执行属性结构数组的偏移值，对应信号将要执行的操作和标志信息
blocked = (33*16)                                                           ## 53# 进程信号屏蔽码(对应信号位图)
                                                                            ## 54# 
# offsets within sigaction                                                  ## 55# 56-59行为定义在sigaction结构中的偏移量
sa_handler = 0                                                              ## 56# 信号处理句柄，是对应信号指定要采取的行动
sa_mask = 4                                                                 ## 57# 信号的屏蔽码，在信号处理程序执行时将阻塞对这些信号的处理
sa_flags = 8                                                                ## 58# 信号选项标志(见include/signal.h)
sa_restorer = 12                                                            ## 59# 信号恢复函数指针(系统内部使用)，用于清理用户态堆栈
                                                                            ## 60# 
nr_system_calls = 72                                                        ## 61# Linux0.11内核中系统调用的总数
                                                                            ## 62# 
/*                                                                          ## 63# 
 * Ok, I get parallel printer interrupts while using the floppy for some    ## 64# 
 * strange reason. Urgel. Now I just ignore them.                           ## 65# 
 */                                                                         ## 66# 
.globl _system_call,_sys_fork,_timer_interrupt,_sys_execve                  ## 67# 67-69行定义入口点
.globl _hd_interrupt,_floppy_interrupt,_parallel_interrupt                  ## 68# 
.globl _device_not_available, _coprocessor_error                            ## 69# 
                                                                            ## 70# 
.align 2                                                                    ## 71# 
bad_sys_call:                                                               ## 72# [b;]错误的系统调用号
	movl $-1,%eax                                                       ## 73# eax中置-1，退出中断
	iret                                                                ## 74# 
.align 2                                                                    ## 75# 
reschedule:                                                                 ## 76# [b;]重新执行调度程序的入口
	pushl $ret_from_sys_call                                            ## 77# 使得调度程序schedule()返回后执行ret_from_sys_call，对信号进行识别处理
	jmp _schedule                                                       ## 78# 
.align 2                                                                    ## 79# 
_system_call:                                                               ## 80# [b;]Linux系统调用入口点，eax中是调用号
	cmpl $nr_system_calls-1,%eax                                        ## 81# 检测调用号是否超出范围
	ja bad_sys_call                                                     ## 82# 如果超出就跳转到bad_sys_call
	push %ds                                                            ## 83# 保存原段寄存器的值
	push %es                                                            ## 84# 
	push %fs                                                            ## 85# 
	pushl %edx                                                          ## 86# 
	pushl %ecx		# push %ebx,%ecx,%edx as parameters         ## 87# 
	pushl %ebx		# to the system call                        ## 88# 
	movl $0x10,%edx		# set up ds,es to kernel space              ## 89# 使得ds、es指向内核数据段
	mov %dx,%ds                                                         ## 90# 
	mov %dx,%es                                                         ## 91# 
	movl $0x17,%edx		# fs points to local data space             ## 92# 使得fs指向当前任务自身的数据段
	mov %dx,%fs                                                         ## 93# 
	call _sys_call_table(,%eax,4)                                       ## 94# 间接调用指定功能的C函数
	pushl %eax                                                          ## 95# [b;]将系统调用的返回值入栈
	movl _current,%eax                                                  ## 96# 取当前任务的数据结构地址放入eax中
	cmpl $0,state(%eax)		# state                             ## 97# 判断当前进程是否为就绪态
	jne reschedule                                                      ## 98# 不是就跳转执行调用程序
	cmpl $0,counter(%eax)		# counter                           ## 99# 判断当前进程的时间片是否用完了
	je reschedule                                                       ##100# 用完了就跳转执行调用程序
ret_from_sys_call:                                                          ##101# [b;]对信号进行识别处理
	movl _current,%eax		# task[0] cannot have signals       ##102# 
	cmpl _task,%eax                                                     ##103# 判断当前任务是否是初始任务task0
	je 3f                                                               ##104# 如果是就直接退出中断
	cmpw $0x0f,CS(%esp)		# was old code segment supervisor ? ##105# 判断调用程序是否是用户程序(0x0f->LDT中的代码段)
	jne 3f                                                              ##106# 如果不是就直接退出中断，因为任务在内核态执行不可抢占
	cmpw $0x17,OLDSS(%esp)		# was stack segment = 0x17 ?        ##107# 判断原堆栈段选择符是否为0x17(即原堆栈是否在用户段中)
	jne 3f                                                              ##108# 如果不是就直接退出中断，因为任务在内核态执行不可抢占
	movl signal(%eax),%ebx                                              ##109# 取信号位图给ebx
	movl blocked(%eax),%ecx                                             ##110# 取阻塞(屏蔽)信号位图给ecx
	notl %ecx                                                           ##111# 取反
	andl %ebx,%ecx                                                      ##112# 取得获取许可的信号位图放入ecx
	bsfl %ecx,%ecx                                                      ##113# 从位0开始扫描ecx，看是否有为1的位，若有将该位的偏移值存入ecx(0-31)，取得数值最小的信号偏移
	je 3f                                                               ##114# 若没有为1的位，直接退出中断
	btrl %ecx,%ebx                                                      ##115# 将原信号位图中的那一位复位
	movl %ebx,signal(%eax)                                              ##116# 重新保存信号位图到current->signal
	incl %ecx                                                           ##117# 将偏移值调整为从1开始的数(1-32)
	pushl %ecx                                                          ##118# 将偏移值入栈，作为do_signal()的参数之一
	call _do_signal                                                     ##119# 调用do_signal()，将对应的信号处理程序插入用户程序栈中，在当前系统调用结束返回后，
	popl %eax                                                           ##120# 先执行信号处理程序，在接着执行用户程序
3:	popl %eax                                                           ##121# 
	popl %ebx                                                           ##122# 
	popl %ecx                                                           ##123# 
	popl %edx                                                           ##124# 
	pop %fs                                                             ##125# 
	pop %es                                                             ##126# 
	pop %ds                                                             ##127# 
	iret                                                                ##128# 
                                                                            ##129# 
.align 2                                                                    ##130# 
_coprocessor_error:                                                         ##131# [r;]131-145行为int 16的中断处理程序-处理器错误中断，无错误号
	push %ds                                                            ##132# 
	push %es                                                            ##133# 
	push %fs                                                            ##134# 
	pushl %edx                                                          ##135# 
	pushl %ecx                                                          ##136# 
	pushl %ebx                                                          ##137# 
	pushl %eax                                                          ##138# 
	movl $0x10,%eax                                                     ##139# 使得ds、es指向内核数据段
	mov %ax,%ds                                                         ##140# 
	mov %ax,%es                                                         ##141# 
	movl $0x17,%eax                                                     ##142# 使得fs指向当前任务自身的数据段
	mov %ax,%fs                                                         ##143# 
	pushl $ret_from_sys_call                                            ##144# math_error()返回后对信号进行识别处理
	jmp _math_error                                                     ##145# 执行C函数math_error()
                                                                            ##146# 
.align 2                                                                    ##147# 
_device_not_available:                                                      ##148# [r;]148-173行为int 7的中断处理程序-设备(协处理器)不存在，无错误号
	push %ds                                                            ##149# 
	push %es                                                            ##150# 
	push %fs                                                            ##151# 
	pushl %edx                                                          ##152# 
	pushl %ecx                                                          ##153# 
	pushl %ebx                                                          ##154# 
	pushl %eax                                                          ##155# 
	movl $0x10,%eax                                                     ##156# 
	mov %ax,%ds                                                         ##157# 
	mov %ax,%es                                                         ##158# 
	movl $0x17,%eax                                                     ##159# 
	mov %ax,%fs                                                         ##160# 
	pushl $ret_from_sys_call                                            ##161# 
	clts				# clear TS so that we can use math  ##162# 
	movl %cr0,%eax                                                      ##163# 
	testl $0x4,%eax			# EM (math emulation bit)           ##164# 
	je _math_state_restore                                              ##165# 
	pushl %ebp                                                          ##166# 
	pushl %esi                                                          ##167# 
	pushl %edi                                                          ##168# 
	call _math_emulate                                                  ##169# 
	popl %edi                                                           ##170# 
	popl %esi                                                           ##171# 
	popl %ebp                                                           ##172# 
	ret                                                                 ##173# 
                                                                            ##174# 
.align 2                                                                    ##175# 
_timer_interrupt:                                                           ##176# [r;]176-197行为int 32的中断处理程序-时钟中断处理程序
	push %ds		# save ds,es and put kernel data space      ##177# 
	push %es		# into them. %fs is used by _system_call    ##178# 
	push %fs                                                            ##179# 
	pushl %edx		# we save %eax,%ecx,%edx as gcc doesn't     ##180# 
	pushl %ecx		# save those across function calls. %ebx    ##181# 
	pushl %ebx		# is saved as we use that in ret_sys_call   ##182# 
	pushl %eax                                                          ##183# 
	movl $0x10,%eax                                                     ##184# 使得ds、es指向内核数据段
	mov %ax,%ds                                                         ##185# 
	mov %ax,%es                                                         ##186# 
	movl $0x17,%eax                                                     ##187# 使得fs指向当前任务自身的数据段
	mov %ax,%fs                                                         ##188# 
	incl _jiffies                                                       ##189# 因为每10ms产生一个中断，所以每次增加一个滴答值
	movb $0x20,%al		# EOI to interrupt controller #1            ##190# 
	outb %al,$0x20                                                      ##191# 向8259A主芯片发送EOI(中断结束)信号
	movl CS(%esp),%eax                                                  ##192# 192-194行取执行系统调用代码的选择符中的当前特权级CPL压入栈中，作为do_timer的参数
	andl $3,%eax		# %eax is CPL (0 or 3, 0=supervisor)        ##193# 
	pushl %eax                                                          ##194# 
	call _do_timer		# 'do_timer(long CPL)' does everything from ##195# 调用do_timer()执行任务切换、计时等工作
	addl $4,%esp		# task switching to accounting ...          ##196# 调整esp的值
	jmp ret_from_sys_call                                               ##197# 跳转去对信号进行识别处理
                                                                            ##198# 
.align 2                                                                    ##199# 
_sys_execve:                                                                ##200# [r;]200-205行是sys_execve()系统调用
	lea EIP(%esp),%eax                                                  ##201# 
	pushl %eax                                                          ##202# eax指向堆栈中保存用户程序eip指针处(EIP+%esp)
	call _do_execve                                                     ##203# 
	addl $4,%esp                                                        ##204# 
	ret                                                                 ##205# 
                                                                            ##206# 
.align 2                                                                    ##207# 
_sys_fork:                                                                  ##208# [r;]208-219行是sys_fork()系统调用，用于创建子进程
	call _find_empty_process                                            ##209# 
	testl %eax,%eax                                                     ##210# 
	js 1f                                                               ##211# 
	push %gs                                                            ##212# 
	pushl %esi                                                          ##213# 
	pushl %edi                                                          ##214# 
	pushl %ebp                                                          ##215# 
	pushl %eax                                                          ##216# 
	call _copy_process                                                  ##217# 
	addl $20,%esp                                                       ##218# 
1:	ret                                                                 ##219# 
                                                                            ##220# 
_hd_interrupt:                                                              ##221# [r;]221-250行为int 46的中断处理程序-硬盘中断处理程序
	pushl %eax                                                          ##222# 
	pushl %ecx                                                          ##223# 
	pushl %edx                                                          ##224# 
	push %ds                                                            ##225# 
	push %es                                                            ##226# 
	push %fs                                                            ##227# 
	movl $0x10,%eax                                                     ##228# 
	mov %ax,%ds                                                         ##229# 
	mov %ax,%es                                                         ##230# 
	movl $0x17,%eax                                                     ##231# 
	mov %ax,%fs                                                         ##232# 
	movb $0x20,%al                                                      ##233# 
	outb %al,$0xA0		# EOI to interrupt controller #1            ##234# 
	jmp 1f			# give port chance to breathe               ##235# 
1:	jmp 1f                                                              ##236# 
1:	xorl %edx,%edx                                                      ##237# 
	xchgl _do_hd,%edx                                                   ##238# 
	testl %edx,%edx                                                     ##239# 
	jne 1f                                                              ##240# 
	movl $_unexpected_hd_interrupt,%edx                                 ##241# 
1:	outb %al,$0x20                                                      ##242# 
	call *%edx		# "interesting" way of handling intr.       ##243# 
	pop %fs                                                             ##244# 
	pop %es                                                             ##245# 
	pop %ds                                                             ##246# 
	popl %edx                                                           ##247# 
	popl %ecx                                                           ##248# 
	popl %eax                                                           ##249# 
	iret                                                                ##250# 
                                                                            ##251# 
_floppy_interrupt:                                                          ##252# [r;]252-278行为int 38的中断处理程序-软盘驱动器中断处理程序
	pushl %eax                                                          ##253# 
	pushl %ecx                                                          ##254# 
	pushl %edx                                                          ##255# 
	push %ds                                                            ##256# 
	push %es                                                            ##257# 
	push %fs                                                            ##258# 
	movl $0x10,%eax                                                     ##259# 
	mov %ax,%ds                                                         ##260# 
	mov %ax,%es                                                         ##261# 
	movl $0x17,%eax                                                     ##262# 
	mov %ax,%fs                                                         ##263# 
	movb $0x20,%al                                                      ##264# 
	outb %al,$0x20		# EOI to interrupt controller #1            ##265# 
	xorl %eax,%eax                                                      ##266# 
	xchgl _do_floppy,%eax                                               ##267# 
	testl %eax,%eax                                                     ##268# 
	jne 1f                                                              ##269# 
	movl $_unexpected_floppy_interrupt,%eax                             ##270# 
1:	call *%eax		# "interesting" way of handling intr.       ##271# 
	pop %fs                                                             ##272# 
	pop %es                                                             ##273# 
	pop %ds                                                             ##274# 
	popl %edx                                                           ##275# 
	popl %ecx                                                           ##276# 
	popl %eax                                                           ##277# 
	iret                                                                ##278# 
                                                                            ##279# 
_parallel_interrupt:                                                        ##280# [r;]280-285行为int 39的中断处理程序-并行口中断处理程序
	pushl %eax                                                          ##281# 
	movb $0x20,%al                                                      ##282# 
	outb %al,$0x20                                                      ##283# 
	popl %eax                                                           ##284# 
	iret                                                                ##285# 

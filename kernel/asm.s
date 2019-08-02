/*                                                                   ##  1# 
 *  linux/kernel/asm.s                                               ##  2# 
 *                                                                   ##  3# 
 *  (C) 1991  Linus Torvalds                                         ##  4# 
 */                                                                  ##  5# 
                                                                     ##  6# 
/*                                                                   ##  7# 
 * asm.s contains the low-level code for most hardware faults.       ##  8# 
 * page_exception is handled by the mm, so that isn't here. This     ##  9# 
 * file also handles (hopefully) fpu-exceptions due to TS-bit, as    ## 10# 
 * the fpu must be properly saved/resored. This hasn't been tested.  ## 11# 
 */                                                                  ## 12# 
                                                                     ## 13# [b;]下面是各个中断的中断处理程序的入口地址
.globl _divide_error,_debug,_nmi,_int3,_overflow,_bounds,_invalid_op ## 14# 中断号为0，1，2，3，4，5，6的中断处理程序的入口地址
.globl _double_fault,_coprocessor_segment_overrun                    ## 15# 中断号为8，9的中断处理程序的入口地址
.globl _invalid_TSS,_segment_not_present,_stack_segment              ## 16# 中断号为10，11，12的中断处理程序的入口地址
.globl _general_protection,_coprocessor_error,_irq13,_reserved       ## 17# 中断号为13，16，45，15的中断处理程序的入口地址
                                                                     ## 18# 
_divide_error:                                                       ## 19# [b;]19-51行为int 0的中断处理程序-处理被零除出错，无错误号
	pushl $_do_divide_error                                      ## 20# 压入do_divide_error函数的地址，用于后面的call *%eax调用
no_error_code:                                                       ## 21# 
	xchgl %eax,(%esp)                                            ## 22# 
	pushl %ebx                                                   ## 23# 
	pushl %ecx                                                   ## 24# 
	pushl %edx                                                   ## 25# 
	pushl %edi                                                   ## 26# 
	pushl %esi                                                   ## 27# 
	pushl %ebp                                                   ## 28# 
	push %ds                                                     ## 29# 
	push %es                                                     ## 30# 
	push %fs                                                     ## 31# 
	pushl $0		# "error code"                       ## 32# 因为无错误号，所以压入0，作为call *%eax要调用的函数的参数
	lea 44(%esp),%edx                                            ## 33# 
	pushl %edx                                                   ## 34# 压入eip保存位置的地址，作为call *%eax要调用的函数的参数
	movl $0x10,%edx                                              ## 35# 
	mov %dx,%ds                                                  ## 36# 
	mov %dx,%es                                                  ## 37# 
	mov %dx,%fs                                                  ## 38# 
	call *%eax                                                   ## 39# 调用eax指定地址处的函数，即最开始压入栈中的函数
	addl $8,%esp                                                 ## 40# 
	pop %fs                                                      ## 41# 
	pop %es                                                      ## 42# 
	pop %ds                                                      ## 43# 
	popl %ebp                                                    ## 44# 
	popl %esi                                                    ## 45# 
	popl %edi                                                    ## 46# 
	popl %edx                                                    ## 47# 
	popl %ecx                                                    ## 48# 
	popl %ebx                                                    ## 49# 
	popl %eax                                                    ## 50# 
	iret                                                         ## 51# 
                                                                     ## 52# 
_debug:                                                              ## 53# [b;]int 1的中断处理程序-debug调试中断入口点，无错误号
	pushl $_do_int3		# _do_debug                          ## 54# 压入do_int3函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 55# 
                                                                     ## 56# 
_nmi:                                                                ## 57# [b;]int 2的中断处理程序-非屏蔽中断调用入口点，无错误号
	pushl $_do_nmi                                               ## 58# 压入do_nmi函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 59# 
                                                                     ## 60# 
_int3:                                                               ## 61# [b;]int 3的中断处理程序-断点指令引起中断的入口点，无错误号
	pushl $_do_int3                                              ## 62# 压入do_int3函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 63# 
                                                                     ## 64# 
_overflow:                                                           ## 65# [b;]int 4的中断处理程序-溢出出错处理中断入口点，无错误号
	pushl $_do_overflow                                          ## 66# 压入do_overflow函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 67# 
                                                                     ## 68# 
_bounds:                                                             ## 69# [b;]int 5的中断处理程序-边界检查出错中断入口点，无错误号
	pushl $_do_bounds                                            ## 70# 压入do_bounds函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 71# 
                                                                     ## 72# 
_invalid_op:                                                         ## 73# [b;]int 6的中断处理程序-无效操作指令出错中断入口点，无错误号
	pushl $_do_invalid_op                                        ## 74# 压入do_invalid_op函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 75# 
                                                                     ## 76# 
_coprocessor_segment_overrun:                                        ## 77# [b;]int 9的中断处理程序-协处理器段超出出错中断入口点，无错误号
	pushl $_do_coprocessor_segment_overrun                       ## 78# 压入do_coprocessor_segment_overrun函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 79# 
                                                                     ## 80# 
_reserved:                                                           ## 81# [b;]int 15的中断处理程序-Intel保留中断入口点，无错误号
	pushl $_do_reserved                                          ## 82# 压入do_reserved函数的地址，用于后面的call *%eax调用
	jmp no_error_code                                            ## 83# 
                                                                     ## 84# 
_irq13:                                                              ## 85# [r;]int 45的中断处理程序-数学协处理器硬件中断入口点
	pushl %eax                                                   ## 86# 
	xorb %al,%al                                                 ## 87# 
	outb %al,$0xF0                                               ## 88# 向协处理器端口0xF0写入0，用于清忙锁存器
	movb $0x20,%al                                               ## 89# 
	outb %al,$0x20                                               ## 90# 向8259主芯片发送EOI(中断结束)信号
	jmp 1f                                                       ## 91# 
1:	jmp 1f                                                       ## 92# 
1:	outb %al,$0xA0                                               ## 93# 向8259从芯片发送EOI(中断结束)信号
	popl %eax                                                    ## 94# 
	jmp _coprocessor_error                                       ## 95# 跳转到_coprocessor_error(在system_call.s中)执行
                                                                     ## 96# 
_double_fault:                                                       ## 97# [b;]int 8的中断处理程序-双出错故障中断入口点，有错误号
	pushl $_do_double_fault                                      ## 98# 压入do_double_fault函数的地址，用于后面的call *%eax调用
error_code:                                                          ## 99# 
	xchgl %eax,4(%esp)		# error code <-> %eax        ##100# 
	xchgl %ebx,(%esp)		# &function <-> %ebx         ##101# 
	pushl %ecx                                                   ##102# 
	pushl %edx                                                   ##103# 
	pushl %edi                                                   ##104# 
	pushl %esi                                                   ##105# 
	pushl %ebp                                                   ##106# 
	push %ds                                                     ##107# 
	push %es                                                     ##108# 
	push %fs                                                     ##109# 
	pushl %eax			# error code                 ##110# 压入错误号，作为call *%ebx要调用的函数的参数
	lea 44(%esp),%eax		# offset                     ##111# 
	pushl %eax                                                   ##112# 压入eip保存位置的地址，作为call *%ebx要调用的函数的参数
	movl $0x10,%eax                                              ##113# 
	mov %ax,%ds                                                  ##114# 
	mov %ax,%es                                                  ##115# 
	mov %ax,%fs                                                  ##116# 
	call *%ebx                                                   ##117# 
	addl $8,%esp                                                 ##118# 
	pop %fs                                                      ##119# 
	pop %es                                                      ##120# 
	pop %ds                                                      ##121# 
	popl %ebp                                                    ##122# 
	popl %esi                                                    ##123# 
	popl %edi                                                    ##124# 
	popl %edx                                                    ##125# 
	popl %ecx                                                    ##126# 
	popl %ebx                                                    ##127# 
	popl %eax                                                    ##128# 
	iret                                                         ##129# 
                                                                     ##130# 
_invalid_TSS:                                                        ##131# [b;]int 10的中断处理程序-无效的任务状态段中断入口点，有错误号
	pushl $_do_invalid_TSS                                       ##132# 压入do_invalid_TSS函数的地址，用于后面的call *%ebx调用
	jmp error_code                                               ##133# 
                                                                     ##134# 
_segment_not_present:                                                ##135# [b;]int 11的中断处理程序-段不存在中断入口点，有错误号
	pushl $_do_segment_not_present                               ##136# 压入do_segment_not_present函数的地址，用于后面的call *%ebx调用
	jmp error_code                                               ##137# 
                                                                     ##138# 
_stack_segment:                                                      ##139# [b;]int 12的中断处理程序-堆栈段错误中断入口点，有错误号
	pushl $_do_stack_segment                                     ##140# 压入do_stack_segment函数的地址，用于后面的call *%ebx调用
	jmp error_code                                               ##141# 
                                                                     ##142# 
_general_protection:                                                 ##143# [b;]int 13的中断处理程序-一般保护性出错中断入口点，有错误号
	pushl $_do_general_protection                                ##144# 压入do_general_protection函数的地址，用于后面的call *%ebx调用
	jmp error_code                                               ##145# 
                                                                     ##146# 

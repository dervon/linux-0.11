/*                                                                               ##  1# 
 *  linux/boot/head.s                                                            ##  2# 
 *                                                                               ##  3# 
 *  (C) 1991  Linus Torvalds                                                     ##  4# 
 */                                                                              ##  5# 
                                                                                 ##  6# 
/*                                                                               ##  7# 
 *  head.s contains the 32-bit startup code.                                     ##  8# 
 *                                                                               ##  9# 
 * NOTE!!! Startup happens at absolute address 0x00000000, which is also where   ## 10# 
 * the page directory will exist. The startup code will be overwritten by        ## 11# 
 * the page directory.                                                           ## 12# 
 */                                                                              ## 13# 
.text                                                                            ## 14# 
.globl _idt,_gdt,_pg_dir,_tmp_floppy_area                                        ## 15# 
_pg_dir:                                                                         ## 16# 存放页目录表的位置(0x0000-0x1000处存放页目录表，原来代码部分被覆盖)
startup_32:                                                                      ## 17# 
	movl $0x10,%eax                                                          ## 18# [b;]18-22行将ds、es、fs、gs都指向setup.s中的
	mov %ax,%ds                                                              ## 19# [b;]预置数据段，可读写，基址=0，偏移量最大值=8MB-1
	mov %ax,%es                                                              ## 20# 
	mov %ax,%fs                                                              ## 21# [r;]#define PAGE_SIZE 4096 ； long user_stack [ PAGE_SIZE>>2 ];
	mov %ax,%gs                                                              ## 22# [r;]struct {long * a;short b;} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };
	lss _stack_start,%esp                                                    ## 23# [r;]引用kernel/sched.c第69行的stack_start，将它地址处的低字放入esp，高字0x0010放入ss
	call setup_idt                                                           ## 24# [b;]创建新的中断描述符表，并将256个中断处理程序都指向了无意义的ignore_int
	call setup_gdt                                                           ## 25# [b;]创建新的全局描述符表，只增大了原代码段和数据段的限长，并多加了253个空描述符
	movl $0x10,%eax		# reload all the segment registers               ## 26# [b;]26-30行用于重置ds、es、fs、gs指向新的(cs已经在setup_gdt返回时重置过了)
	mov %ax,%ds		# after changing gdt. CS was already             ## 27# [b;]数据段，可读写，基址=0，偏移量最大值=16MB-1，囊括整个内存
	mov %ax,%es		# reloaded in 'setup_gdt'                        ## 28# 
	mov %ax,%fs                                                              ## 29# [r;]#define PAGE_SIZE 4096 ； long user_stack [ PAGE_SIZE>>2 ];
	mov %ax,%gs                                                              ## 30# [r;]struct {long * a;short b;} stack_start  = { & user_stack [PAGE_SIZE>>2] , 0x10 };
	lss _stack_start,%esp                                                    ## 31# [r;]引用kernel/sched.c第69行的stack_start，将它地址处的低字放入esp，高字0x0010放入ss
	xorl %eax,%eax                                                           ## 32# [b;]32-36行用于判断A20是否开启
1:	incl %eax		# check that A20 really IS enabled               ## 33# 
	movl %eax,0x000000	# loop forever if it isn't                       ## 34# 
	cmpl %eax,0x100000                                                       ## 35# 
	je 1b                                                                    ## 36# 
/*                                                                               ## 37# 
 * NOTE! 486 should set bit 16, to check for write-protect in supervisor         ## 38# 
 * mode. Then it would be unnecessary with the "verify_area()"-calls.            ## 39# 
 * 486 users probably want to set the NE (#5) bit also, so as to use             ## 40# 
 * int 16 for math errors.                                                       ## 41# 
 */                                                                              ## 42# 
	movl %cr0,%eax		# check math chip                                ## 43# [b;]43-65行用于判断协处理器是否存在并进行相应设置，
	andl $0x80000011,%eax	# Save PG,PE,ET                                  ## 44# 将cr0中的其他位置零，保留PG、PE、ET位不变
/* "orl $0x10020,%eax" here for 486 might be good */                             ## 45# 
	orl $2,%eax		# set MP                                         ## 46# 将MP位置位
	movl %eax,%cr0                                                           ## 47# 将预设好的值写入cr0中
	call check_x87                                                           ## 48# 
	jmp after_page_tables                                                    ## 49# [b;]跳转到after_page_tables
                                                                                 ## 50# 
/*                                                                               ## 51# 
 * We depend on ET to be correct. This checks for 287/387.                       ## 52# 
 */                                                                              ## 53# 
check_x87:                                                                       ## 54# 
	fninit                                                                   ## 55# 是协处理器指令，用于初始化协处理器
	fstsw %ax                                                                ## 56# 是协处理器指令，将协处理器中状态寄存器的值存储到ax寄存器中
	cmpb $0,%al                                                              ## 57# 如果协处理器存在，那么初始化之后状态寄存器的值应为0，否则说明协处理器不存在
	je 1f			/* no coprocessor: have to set bits */           ## 58# 协处理器若存在，则跳转
	movl %cr0,%eax                                                           ## 59# 
	xorl $6,%eax		/* reset MP, set EM */                           ## 60# 在协处理器不存在的情况下，将cr0寄存器中的EM和MP位取反，
	movl %eax,%cr0                                                           ## 61# 用于复位MP(TS标志不影响WAIT的执行)、置位EM(说明系统无协处理器)
	ret                                                                      ## 62# 
.align 2                                                                         ## 63# 
1:	.byte 0xDB,0xE4		/* fsetpm for 287, ignored by 387 */             ## 64# 是协处理器指令fsetpm，用于将协处理器设为保护模式，用于CPU也处于保护模式的情况
	ret                                                                      ## 65# (查阅资料后发现80287和80387都支持该指令)
                                                                                 ## 66# 
/*                                                                               ## 67# 
 *  setup_idt                                                                    ## 68# 
 *                                                                               ## 69# 
 *  sets up a idt with 256 entries pointing to                                   ## 70# 
 *  ignore_int, interrupt gates. It then loads                                   ## 71# 
 *  idt. Everything that wants to install itself                                 ## 72# 
 *  in the idt-table may do so themselves. Interrupts                            ## 73# 
 *  are enabled elsewhere, when we can be relatively                             ## 74# 
 *  sure everything is ok. This routine will be over-                            ## 75# 
 *  written by the page tables.                                                  ## 76# 
 */                                                                              ## 77# 
setup_idt:                                                                       ## 78# 
	lea ignore_int,%edx                                                      ## 79# [b;]79-82行用于构建一个中断门描述符，高32位在eax中，低32位在edx中
	movl $0x00080000,%eax                                                    ## 80# 目标代码段选择子：0x0008 偏移地址：ignore_int，由于ignore_int子
	movw %dx,%ax		/* selector = 0x0008 = cs */                     ## 81# 程序在head.s，即一直在内核中，所以P=1，DPL=00
	movw $0x8E00,%dx	/* interrupt gate - dpl=0, present */            ## 82# 
                                                                                 ## 83# 
	lea _idt,%edi                                                            ## 84# [b;]84-93行用于在head.s中的_idt位置处将上面构建的中断门描述符
	mov $256,%ecx                                                            ## 85# [b;]复制256份填入，并加载idt寄存器的值
rp_sidt:                                                                         ## 86# 
	movl %eax,(%edi)                                                         ## 87# 
	movl %edx,4(%edi)                                                        ## 88# 
	addl $8,%edi                                                             ## 89# 
	dec %ecx                                                                 ## 90# 
	jne rp_sidt                                                              ## 91# 
	lidt idt_descr                                                           ## 92# 
	ret                                                                      ## 93# 
                                                                                 ## 94# 
/*                                                                               ## 95# 
 *  setup_gdt                                                                    ## 96# 
 *                                                                               ## 97# 
 *  This routines sets up a new gdt and loads it.                                ## 98# 
 *  Only two entries are currently built, the same                               ## 99# 
 *  ones that were built in init.s. The routine                                  ##100# 
 *  is VERY complicated at two whole lines, so this                              ##101# 
 *  rather long comment is certainly needed :-).                                 ##102# 
 *  This routine will beoverwritten by the page tables.                          ##103# 
 */                                                                              ##104# 
setup_gdt:                                                                       ##105# [b;]105-107行用于加载新的gdt值，不再使用setup.s中原来的中断描述符表
	lgdt gdt_descr                                                           ##106# 
	ret                                                                      ##107# 
                                                                                 ##108# 
/*                                                                               ##109# 
 * I put the kernel page tables right after the page directory,                  ##110# 
 * using 4 of them to span 16 Mb of physical memory. People with                 ##111# 
 * more than 16MB will have to expand this.                                      ##112# 
 */                                                                              ##113# (0x0000-0x1000处存放页目录表，原来的head.s代码部分被覆盖)
.org 0x1000                                                                      ##114# 准备在0x1000-0x2000处存放第一个页表
pg0:                                                                             ##115# 
                                                                                 ##116# 
.org 0x2000                                                                      ##117# 准备在0x2000-0x3000处存放第一个页表
pg1:                                                                             ##118# 
                                                                                 ##119# 
.org 0x3000                                                                      ##120# 准备在0x3000-0x4000处存放第一个页表
pg2:                                                                             ##121# 
                                                                                 ##122# 
.org 0x4000                                                                      ##123# 准备在0x4000-0x5000处存放第一个页表
pg3:                                                                             ##124# 
                                                                                 ##125# 
.org 0x5000                                                                      ##126# 
/*                                                                               ##127# 
 * tmp_floppy_area is used by the floppy-driver when DMA cannot                  ##128# 
 * reach to a buffer-block. It needs to be aligned, so that it isn't             ##129# 
 * on a 64kB border.                                                             ##130# 
 */                                                                              ##131# 
_tmp_floppy_area:                                                                ##132# 软盘缓冲区(1K大小)
	.fill 1024,1,0                                                           ##133# 
                                                                                 ##134# 
after_page_tables:                                                               ##135# 
	pushl $0		# These are the parameters to main :-)           ##136# 136-138行用于压入envp。argv指针和argc的值给main()用，
	pushl $0                                                                 ##137# 但是main()没有用到
	pushl $0                                                                 ##138# 
	pushl $L6		# return address for main, if it decides to.     ##139# 让main()函数返回后进入死循环
	pushl $_main                                                             ##140# [b;]压入init/main.c中的main函数的地址，用于从
	jmp setup_paging                                                         ##141# [b;]setup_paging返回后进入main函数执行
L6:                                                                              ##142# 
	jmp L6			# main should never return here, but             ##143# 
				# just in case, we know what happens.            ##144# 
                                                                                 ##145# 
/* This is the default interrupt "handler" :-) */                                ##146# 
int_msg:                                                                         ##147# 
	.asciz "Unknown interrupt\n\r"                                           ##148# 
.align 2                                                                         ##149# 
ignore_int:                                                                      ##150# 
	pushl %eax                                                               ##151# 
	pushl %ecx                                                               ##152# 
	pushl %edx                                                               ##153# 
	push %ds                                                                 ##154# 
	push %es                                                                 ##155# 
	push %fs                                                                 ##156# 
	movl $0x10,%eax                                                          ##157# 
	mov %ax,%ds                                                              ##158# 
	mov %ax,%es                                                              ##159# 
	mov %ax,%fs                                                              ##160# 
	pushl $int_msg                                                           ##161# 
	call _printk                                                             ##162# [r;]引用kernel/printk.c中的_printk
	popl %eax                                                                ##163# 
	pop %fs                                                                  ##164# 
	pop %es                                                                  ##165# 
	pop %ds                                                                  ##166# 
	popl %edx                                                                ##167# 
	popl %ecx                                                                ##168# 
	popl %eax                                                                ##169# 
	iret                                                                     ##170# 
                                                                                 ##171# 
                                                                                 ##172# 
/*                                                                               ##173# 
 * Setup_paging                                                                  ##174# 
 *                                                                               ##175# 
 * This routine sets up paging by setting the page bit                           ##176# 
 * in cr0. The page tables are set up, identity-mapping                          ##177# 
 * the first 16MB. The pager assumes that no illegal                             ##178# 
 * addresses are produced (ie >4Mb on a 4Mb machine).                            ##179# 
 *                                                                               ##180# 
 * NOTE! Although all physical memory should be identity                         ##181# 
 * mapped by this routine, only the kernel page functions                        ##182# 
 * use the >1Mb addresses directly. All "normal" functions                       ##183# 
 * use just the lower 1Mb, or the local data space, which                        ##184# 
 * will be mapped to some other place - mm keeps track of                        ##185# 
 * that.                                                                         ##186# 
 *                                                                               ##187# 
 * For those with more memory than 16 Mb - tough luck. I've                      ##188# 
 * not got it, why should you :-) The source is here. Change                     ##189# 
 * it. (Seriously - it shouldn't be too difficult. Mostly                        ##190# 
 * change some constants etc. I left it at 16Mb, as my machine                   ##191# 
 * even cannot be extended past that (ok, but it was cheap :-)                   ##192# 
 * I've tried to show which constants to change by having                        ##193# 
 * some kind of marker at them (search for "16Mb"), but I                        ##194# 
 * won't guarantee that's all :-( )                                              ##195# 
 */                                                                              ##196# 
.align 2                                                                         ##197# 
setup_paging:                                                                    ##198# [b;]设置1个页目录表和4个页表的内容，使4个页表映射内存0-16MB的范围
	movl $1024*5,%ecx		/* 5 pages - pg_dir+4 page tables */     ##199# 199-202行将1个页目录表和4个页表内容都清零
	xorl %eax,%eax                                                           ##200# 
	xorl %edi,%edi			/* pg_dir is at 0x000 */                 ##201# 
	cld;rep;stosl                                                            ##202# 
	movl $pg0+7,_pg_dir		/* set present bit/user r/w */           ##203# 203-206行设置4个页目录项指向那4个页表，且每个页目录项US=1；RW=1；P=1
	movl $pg1+7,_pg_dir+4		/*  --------- " " --------- */           ##204# 
	movl $pg2+7,_pg_dir+8		/*  --------- " " --------- */           ##205# 
	movl $pg3+7,_pg_dir+12		/*  --------- " " --------- */           ##206# 
	movl $pg3+4092,%edi                                                      ##207# 207-212行填写4个页表的所有页表项的内容(4个页表共可映射内存0-16MB的范围)，
	movl $0xfff007,%eax		/*  16Mb - 4096 + 7 (r/w user,p) */      ##208# 且每个页表项US=1；RW=1；P=1
	std                                                                      ##209# 
1:	stosl			/* fill pages backwards - more efficient :-) */  ##210# 
	subl $0x1000,%eax                                                        ##211# 
	jge 1b                                                                   ##212# 
	xorl %eax,%eax		/* pg_dir is at 0x0000 */                        ##213# 213-214行设置cr3的值为0x00000000，使其指向页目录表
	movl %eax,%cr3		/* cr3 - page directory start */                 ##214# 
	movl %cr0,%eax                                                           ##215# 215-217行置位cr0中的PG位，开启分页
	orl $0x80000000,%eax                                                     ##216# 
	movl %eax,%cr0		/* set paging (PG) bit */                        ##217# 
	ret			/* this also flushes prefetch-queue */           ##218# [b;]返回跳转到init/main.c中的main函数中执行
                                                                                 ##219# [b;](因为前面压入来main函数的地址，这里属于段内的函数间调用，只压入eip)
.align 2                                                                         ##220# 
.word 0                                                                          ##221# 
idt_descr:                                                                       ##222# 加载到idt使用
	.word 256*8-1		# idt contains 256 entries                       ##223# 
	.long _idt                                                               ##224# 
.align 2                                                                         ##225# 
.word 0                                                                          ##226# 
gdt_descr:                                                                       ##227# 加载到gdt使用
	.word 256*8-1		# so does gdt (not that that's any               ##228# 
	.long _gdt		# magic number, but it works for me :^)          ##229# 
                                                                                 ##230# 
	.align 3                                                                 ##231# 
_idt:	.fill 256,8,0		# idt is uninitialized                           ##232# 前面通过setup_idt将256个中断处理程序都指向了无意义的ignore_int
                                                                                 ##233# 
_gdt:	.quad 0x0000000000000000	/* NULL descriptor */                    ##234# 
	.quad 0x00c09a0000000fff	/* 16Mb */                               ##235# 代码段，可读可执行，基址=0，偏移量最大值=16MB-1，囊括整个内存
	.quad 0x00c0920000000fff	/* 16Mb */                               ##236# 数据段(ss栈段也指向这里)，可读写，基址=0，偏移量最大值=16MB-1，囊括整个内存
	.quad 0x0000000000000000	/* TEMPORARY - don't use */              ##237# 增加来253个临时的空描述符，暂时未使用
	.fill 252,8,0			/* space for LDT's and TSS's etc */      ##238# 

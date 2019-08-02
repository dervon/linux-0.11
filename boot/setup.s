!                                                                              !  1! 
!	setup.s		(C) 1991 Linus Torvalds                                !  2! 
!                                                                              !  3! 
! setup.s is responsible for getting the system data from the BIOS,            !  4! 
! and putting them into the appropriate places in system memory.               !  5! 
! both setup.s and system has been loaded by the bootblock.                    !  6! 
!                                                                              !  7! 
! This code asks the bios for memory/disk/other parameters, and                !  8! 
! puts them in a "safe" place: 0x90000-0x901FF, ie where the                   !  9! 
! boot-block used to be. It is then up to the protected mode                   ! 10! 
! system to read them from there before the area is overwritten                ! 11! 
! for buffer-blocks.                                                           ! 12! 
!                                                                              ! 13! 
                                                                               ! 14! 
! NOTE! These had better be the same as in bootsect.s!                         ! 15! 
                                                                               ! 16! 
INITSEG  = 0x9000	! we move boot here - out of the way                   ! 17! 原bootsect所处的段的段地址，将在此存放利用BIOS中断读到的机器系统数据
SYSSEG   = 0x1000	! system loaded at 0x10000 (65536).                    ! 18! system模块所在的0x10000(64KB)处
SETUPSEG = 0x9020	! this is the current segment                          ! 19! 本程序setup所在的段地址
                                                                               ! 20! 
.globl begtext, begdata, begbss, endtext, enddata, endbss                      ! 21! 
.text                                                                          ! 22! 
begtext:                                                                       ! 23! 
.data                                                                          ! 24! 
begdata:                                                                       ! 25! 
.bss                                                                           ! 26! 
begbss:                                                                        ! 27! 
.text                                                                          ! 28! 
                                                                               ! 29! 
entry start                                                                    ! 30! 
start:                                                                         ! 31! 
                                                                               ! 32! 
! ok, the read went well so we get current cursor position and save it for     ! 33! 
! posterity.                                                                   ! 34! 
                                                                               ! 35! 
	mov	ax,#INITSEG	! this is done in bootsect already, but...     ! 36! [b;]36-41行利用BIOS中断读取当前光标位置(行、列)，并保存到0x90000处
	mov	ds,ax                                                          ! 37! 
	mov	ah,#0x03	! read cursor pos                              ! 38! ah=0x03表示读光标位置
	xor	bh,bh                                                          ! 39! bh(页号)=0
	int	0x10		! save it in known place, con_init fetches     ! 40! BIOS中断
	mov	[0],dx		! it from 0x90000.                             ! 41! 中断返回：dh(行号)；dl(列号)
                                                                               ! 42! 
! Get memory size (extended mem, kB)                                           ! 43! 
                                                                               ! 44! [b;]45-47行取系统所含扩展内存大小并保存在内存0x90002处
	mov	ah,#0x88                                                       ! 45! ah=0x03表示读系统所含扩展内存大小
	int	0x15                                                           ! 46! BIOS中段
	mov	[2],ax                                                         ! 47! 将中断返回的扩展内存值存在0x90002处
                                                                               ! 48! 
! Get video-card data:                                                         ! 49! 
                                                                               ! 50! [b;]51-54行取显示卡当前显示模式【0x90004(1字)存放当前页；
	mov	ah,#0x0f                                                       ! 51! [b;]0x90006存放显示模式；0x90007存放字符列数】
	int	0x10                                                           ! 52! 
	mov	[4],bx		! bh = display page                            ! 53! 
	mov	[6],ax		! al = video mode, ah = window width           ! 54! 
                                                                               ! 55! 
! check for EGA/VGA and some config parameters                                 ! 56! [b;]59-63行检查显示方式(EGA/VGA)并取参数
                                                                               ! 57! 
	mov	ah,#0x12                                                       ! 58! 
	mov	bl,#0x10                                                       ! 59! 
	int	0x10                                                           ! 60! 
	mov	[8],ax                                                         ! 61! 0x90008处暂时未存有效内容
	mov	[10],bx                                                        ! 62! 0x9000A处存放安装的显存大小(0x00-64k;0x00-128k;0x00-192k;0x00-256k)
	mov	[12],cx                                                        ! 63! 0x9000B处存放显示状态(0x00-彩色模式,IO端口0x3dX;0x01-单色模式,IO端口0x3dX)
                                                                               ! 64! 0x9000C处存放显卡特性参数
! Get hd0 data                                                                 ! 65! 
                                                                               ! 66! [b;]67-75行获取第一个硬盘的参数表信息存在0x90080，表长度16B
	mov	ax,#0x0000                                                     ! 67! 中断0x41的向量值不是中断处理程序地址，而是参数表的首地址
	mov	ds,ax                                                          ! 68! 从DS：SI读到ES：DI
	lds	si,[4*0x41]                                                    ! 69! 
	mov	ax,#INITSEG                                                    ! 70! 
	mov	es,ax                                                          ! 71! 
	mov	di,#0x0080                                                     ! 72! 
	mov	cx,#0x10                                                       ! 73! 
	rep                                                                    ! 74! 
	movsb                                                                  ! 75! 
                                                                               ! 76! 
! Get hd1 data                                                                 ! 77! 
                                                                               ! 78! [b;]79-87行获取第二个硬盘的参数表信息存在0x90090，表长度16B
	mov	ax,#0x0000                                                     ! 79! 中断0x46的向量值不是中断处理程序地址，而是参数表的首地址
	mov	ds,ax                                                          ! 80! 从DS：SI读到ES：DI
	lds	si,[4*0x46]                                                    ! 81! 
	mov	ax,#INITSEG                                                    ! 82! 
	mov	es,ax                                                          ! 83! 
	mov	di,#0x0090                                                     ! 84! 
	mov	cx,#0x10                                                       ! 85! 
	rep                                                                    ! 86! 
	movsb                                                                  ! 87! 
                                                                               ! 88! 
! Check that there IS a hd1 :-)                                                ! 89! [b;]91-105行用于检测系统是否有第二个硬盘，如果没有就将上面读到的
                                                                               ! 90! [b;]第二个硬盘的参数表16B内容清零
	mov	ax,#0x01500                                                    ! 91! 
	mov	dl,#0x81                                                       ! 92! 
	int	0x13                                                           ! 93! 
	jc	no_disk1                                                       ! 94! 
	cmp	ah,#3                                                          ! 95! 
	je	is_disk1                                                       ! 96! 
no_disk1:                                                                      ! 97! 
	mov	ax,#INITSEG                                                    ! 98! 
	mov	es,ax                                                          ! 99! 
	mov	di,#0x0090                                                     !100! 
	mov	cx,#0x10                                                       !101! 
	mov	ax,#0x00                                                       !102! 
	rep                                                                    !103! 
	stosb                                                                  !104! 
is_disk1:                                                                      !105! 
                                                                               !106! 
! now we want to move to protected mode ...                                    !107! 
                                                                               !108! 
	cli			! no interrupts allowed !                      !109! [b;]关中断
                                                                               !110! 
! first we move the system to it's rightful place                              !111! 
                                                                               !112! 
	mov	ax,#0x0000                                                     !113! [b;]113-126行用于将system模块移动到0x00000处
	cld			! 'direction'=0, movs moves forward            !114! 
do_move:                                                                       !115! 
	mov	es,ax		! destination segment                          !116! 
	add	ax,#0x1000                                                     !117! 
	cmp	ax,#0x9000                                                     !118! 
	jz	end_move                                                       !119! 
	mov	ds,ax		! source segment                               !120! 
	sub	di,di                                                          !121! 
	sub	si,si                                                          !122! 
	mov 	cx,#0x8000                                                     !123! 
	rep                                                                    !124! 
	movsw                                                                  !125! 
	jmp	do_move                                                        !126! 
                                                                               !127! 
! then we load the segment descriptors                                         !128! 
                                                                               !129! 
end_move:                                                                      !130! 
	mov	ax,#SETUPSEG	! right, forgot this at first. didn't work :-) !131! [b;]131-134行用于加载预置的内容到IDTR和GDTR
	mov	ds,ax                                                          !132! 
	lidt	idt_48		! load idt with 0,0                            !133! 
	lgdt	gdt_48		! load gdt with whatever appropriate           !134! 
                                                                               !135! 
! that was painless, now we enable A20                                         !136! [b;]138-144行用于使能A20引脚(与现在PC开启A20的方式可能不同)，
                                                                               !137! [b;]详细内容请参考intel8042的文档资料
	call	empty_8042                                                     !138! 调用empty_8042检查键盘命令队列是否为空，当键盘的命令队列未满时，从empty_8042返回
	mov	al,#0xD1		! command write                        !139! 0xD1为写键盘控制器804x的输出端口P2的命令
	out	#0x64,al                                                       !140! 将命令0xD1写入键盘控制器804x的输入缓冲器中
	call	empty_8042                                                     !141! 调用empty_8042检查键盘命令队列是否为空，当键盘的命令队列未满时，从empty_8042返回
	mov	al,#0xDF		! A20 on                               !142! 0xDF为0xD1命令的参数
	out	#0x60,al                                                       !143! 通过键盘控制器804x的0x60端口将参数0xDF传递进去，使得输出端口P2的值为0xDF，从而使A20选通
	call	empty_8042                                                     !144! 调用empty_8042检查键盘命令队列是否为空，当键盘的命令队列未满时，从empty_8042返回
                                                                               !145! 
! well, that went ok, I hope. Now we have to reprogram the interrupts :-(      !146! 
! we put them right after the intel-reserved hardware interrupts, at           !147! 
! int 0x20-0x2F. There they won't mess up anything. Sadly IBM really           !148! 
! messed this up with the original PC, and they haven't been able to           !149! [b;]154-179行用于重新对中断控制器8259A进行编程，
! rectify it afterwards. Thus the bios puts interrupts at 0x08-0x0f,           !150! [b;]将中断设为int 0x20-0x2f
! which is used for the internal hardware interrupts as well. We just          !151! (通过指定0x20、0xA0间接使A0=0;指定0x21、0xA1间接使A0=1)
! have to reprogram the 8259's, and it isn't fun.                              !152! 
                                                                               !153! 
	mov	al,#0x11		! initialization sequence              !154! (往ICW1中写入0x11，可以看作是一个初始化的开始，这样后面会自动识别ICW234)
	out	#0x20,al		! send it to 8259A-1                   !155! 设置主片的ICW1为0x11，即边沿触发、多片级联、最后要发送ICW4
	.word	0x00eb,0x00eb		! jmp $+2, jmp $+2                     !156! 
	out	#0xA0,al		! and to 8259A-2                       !157! 设置从片的ICW1为0x11，即边沿触发、多片级联、最后要发送ICW4
	.word	0x00eb,0x00eb                                                  !158! 
	mov	al,#0x20		! start of hardware int's (0x20)       !159! 
	out	#0x21,al                                                       !160! 设置主片的ICW2为0x20，即中断号为0x20-0x27
	.word	0x00eb,0x00eb                                                  !161! 
	mov	al,#0x28		! start of hardware int's 2 (0x28)     !162! 
	out	#0xA1,al                                                       !163! 设置从片的ICW2为0x28，即中断号为0x28-0x2f
	.word	0x00eb,0x00eb                                                  !164! 
	mov	al,#0x04		! 8259-1 is master                     !165! 
	out	#0x21,al                                                       !166! 设置主片的ICW3为0x04(00000100B)，即主片中断引脚的第三个级联到从片
	.word	0x00eb,0x00eb                                                  !167! 
	mov	al,#0x02		! 8259-2 is slave                      !168! 
	out	#0xA1,al                                                       !169! 设置从片的ICW3为0x02，低三位的值用来指定从片的标识号为010(2)
	.word	0x00eb,0x00eb                                                  !170! (当8259的级联线CAS2-CAS0输入的值与从片的标识号相同，表示此片被选中，
	mov	al,#0x01		! 8086 mode for both                   !171! 此时该从片应该向数据总线发送从片当前选中中断请求的中断号)
	out	#0x21,al                                                       !172! 设置主片的ICW4为0x01，即普通全嵌套、非缓冲、非自动结束且用于8086及兼容系统
	.word	0x00eb,0x00eb                                                  !173! 
	out	#0xA1,al                                                       !174! 设置从片的ICW4为0x01，即普通全嵌套、非缓冲、非自动结束且用于8086及兼容系统
	.word	0x00eb,0x00eb                                                  !175! 
	mov	al,#0xFF		! mask off all interrupts for now      !176! 
	out	#0x21,al                                                       !177! 设置主片的OCW1为0xff(11111111B)，将8个中断全部屏蔽
	.word	0x00eb,0x00eb                                                  !178! 
	out	#0xA1,al                                                       !179! 设置从片的OCW1为0xff(11111111B)，将8个中断全部屏蔽
                                                                               !180! 
! well, that certainly wasn't fun :-(. Hopefully it works, and we don't        !181! 
! need no steenking BIOS anyway (except for the initial loading :-).           !182! 
! The BIOS-routine wants lots of unnecessary data, and it's less               !183! 
! "interesting" anyway. This is how REAL programmers do it.                    !184! 
!                                                                              !185! 
! Well, now's the time to actually move into protected mode. To make           !186! 
! things as simple as possible, we do no register set-up or anything,          !187! 
! we let the gnu-compiled 32-bit programs do that. We just jump to             !188! 
! absolute address 0x00000, in 32-bit protected mode.                          !189! 
                                                                               !190! 
	mov	ax,#0x0001	! protected mode (PE) bit                      !191! [b;]开启PE位，进入保护模式
	lmsw	ax		! This is it!                                  !192! lmsw指令用于将操作数加载进CR0的低16位，其他位不变，使用此命令只为兼容80286
	jmpi	0,8		! jmp offset 0 of segment 8 (cs)               !193! 
                                                                               !194! 
! This routine checks that the keyboard command queue is empty                 !195! 
! No timeout is used - if this hangs there is something wrong with             !196! 
! the machine, and we probably couldn't proceed anyway.                        !197! 
empty_8042:                                                                    !198! 198-203用于检测键盘队列是否为空，详细内容请参考intel8042手册
	.word	0x00eb,0x00eb                                                  !199! 延时一会
	in	al,#0x64	! 8042 status port                             !200! 读出键盘控制器的状态寄存器端口0x64的值
	test	al,#2		! is input buffer full?                        !201! 判断输入缓冲器是否已满
	jnz	empty_8042	! yes - loop                                   !202! 如果满了就跳转回去重新判断，直至有多余空间可以输入
	ret                                                                    !203! 
                                                                               !204! 
gdt:                                                                           !205! 205-216行为预置的代码段和数据段描述符的内容
	.word	0,0,0,0		! dummy                                        !206! 
                                                                               !207! 
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)             !208! 预置代码段，可读可执行，基址=0，偏移量最大值=8MB-1
	.word	0x0000		! base address=0                               !209! 
	.word	0x9A00		! code read/exec                               !210! 
	.word	0x00C0		! granularity=4096, 386                        !211! 
                                                                               !212! 
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)             !213! 预置数据段，可读写，基址=0，偏移量最大值=8MB-1
	.word	0x0000		! base address=0                               !214! 
	.word	0x9200		! data read/write                              !215! 
	.word	0x00C0		! granularity=4096, 386                        !216! 
                                                                               !217! 
idt_48:                                                                        !218! 
	.word	0			! idt limit=0                          !219! 
	.word	0,0			! idt base=0L                          !220! 
                                                                               !221! 
gdt_48:                                                                        !222! 
	.word	0x800		! gdt limit=2048, 256 GDT entries              !223! 可存放256项段描述符
	.word	512+gdt,0x9	! gdt base = 0X9xxxx                           !224! 即0x90200+gdt，因为代码段描述符和数据段描述符还在setup模块中
	                                                                       !225! 
.text                                                                          !226! 
endtext:                                                                       !227! 
.data                                                                          !228! 
enddata:                                                                       !229! 
.bss                                                                           !230! 
endbss:                                                                        !231! 

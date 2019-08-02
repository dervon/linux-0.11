!                                                                              !  1! 
! SYS_SIZE is the number of clicks (16 bytes) to be loaded.                    !  2! 
! 0x3000 is 0x30000 bytes = 196kB, more than enough for current                !  3! 
! versions of linux                                                            !  4! 
!                                                                              !  5! 
SYSSIZE = 0x3000                                                               !  6! [b;]SYSSIZE指示system模块长度的默认最大值，单位是节，1节即16B，
!                                                                              !  7! 所以共0x30000B=192KB
!	bootsect.s		(C) 1991 Linus Torvalds                        !  8! 
!                                                                              !  9! 
! bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves       ! 10! 
! iself out of the way to address 0x90000, and jumps there.                    ! 11! 
!                                                                              ! 12! 
! It then loads 'setup' directly after itself (0x90200), and the system        ! 13! 
! at 0x10000, using BIOS interrupts.                                           ! 14! 
!                                                                              ! 15! 
! NOTE! currently system is at most 8*65536 bytes long. This should be no      ! 16! 
! problem, even in the future. I want to keep it simple. This 512 kB           ! 17! 
! kernel size should be enough, especially as this doesn't contain the         ! 18! 
! buffer cache as in minix                                                     ! 19! 
!                                                                              ! 20! 
! The loader has been made as simple as possible, and continuos                ! 21! 
! read errors will result in a unbreakable loop. Reboot by hand. It            ! 22! 
! loads pretty fast by getting whole sectors at a time whenever possible.      ! 23! 
                                                                               ! 24! 
.globl begtext, begdata, begbss, endtext, enddata, endbss                      ! 25! 定义六个全局符号，即使不用也强制引入
.text                                                                          ! 26! 
begtext:                                                                       ! 27! 
.data                                                                          ! 28! 
begdata:                                                                       ! 29! 
.bss                                                                           ! 30! 
begbss:                                                                        ! 31! 
.text                                                                          ! 32! 
                                                                               ! 33! 
SETUPLEN = 4				! nr of setup-sectors                  ! 34! setup模块的扇区数
BOOTSEG  = 0x07c0			! original address of boot-sector      ! 35! bootsect模块被加载的原始地址的段地址
INITSEG  = 0x9000			! we move boot here - out of the way   ! 36! bootsect模块被加载的新地址的段地址
SETUPSEG = 0x9020			! setup starts here                    ! 37! setup模块被加载的地址的段地址
SYSSEG   = 0x1000			! system loaded at 0x10000 (65536).    ! 38! system模块被加载的地址的段地址
ENDSEG   = SYSSEG + SYSSIZE		! where to stop loading                ! 39! 停止加载的段地址
                                                                               ! 40! 
! ROOT_DEV:	0x000 - same type of floppy as boot.                           ! 41! 根设备号如果是0，就会在后面重新设置为与软驱一样的设备号
!		0x301 - first partition on first drive etc                     ! 42! 
ROOT_DEV = 0x306                                                               ! 43! [b;]指定根文件系统所在设备的设备号(此为第二个硬盘的第一个分区)，
                                                                               ! 44! [b;]存放在第509、510两字节
entry start                                                                    ! 45! 告诉链接程序，程序从start开始执行，可能是更改执行
start:                                                                         ! 46! 头部中的对应字段
	mov	ax,#BOOTSEG                                                    ! 47! [b;]47-56行是将bootsect自身的内容复制一份放到新地址
	mov	ds,ax                                                          ! 48! [b;]0x9000处，然后跳转过去到go处执行
	mov	ax,#INITSEG                                                    ! 49! 
	mov	es,ax                                                          ! 50! 
	mov	cx,#256                                                        ! 51! 
	sub	si,si                                                          ! 52! 
	sub	di,di                                                          ! 53! 
	rep                                                                    ! 54! 
	movw                                                                   ! 55! 
	jmpi	go,INITSEG                                                     ! 56! 
go:	mov	ax,cs                                                          ! 57! 将ds、es和ss都设置到新位置所在段地址0x9000，让sp指向
	mov	ds,ax                                                          ! 58! 0x9ff00处
	mov	es,ax                                                          ! 59! 
! put stack at 0x9ff00.                                                        ! 60! 
	mov	ss,ax                                                          ! 61! 
	mov	sp,#0xFF00		! arbitrary value >>512                ! 62! 
                                                                               ! 63! 
! load the setup-sectors directly after the bootblock.                         ! 64! 
! Note that 'es' is already set up.                                            ! 65! 
                                                                               ! 66! [b;]68-77行利用BIOS中断INT 0x13将setup模块从磁盘
load_setup:                                                                    ! 67! [b;]的第二个扇区开始的4个扇区数据读到0x90200开始处
	mov	dx,#0x0000		! drive 0, head 0                      ! 68! dh(磁头号)=0x00；dl(驱动器号)=0x00(这里应该是软盘，如果是硬盘，则位7要置位)；
	mov	cx,#0x0002		! sector 2, track 0                    ! 69! ch(柱面号的低8位)=0x00；cl=0x02，位0-5，开始扇区号，位6-7，柱面号高2位
	mov	bx,#0x0200		! address = 512, in INITSEG            ! 70! es：bx(指向数据缓冲区)=0x90200
	mov	ax,#0x0200+SETUPLEN	! service 2, nr of sectors             ! 71! ah=0x02表示读磁盘扇区到内存；al=0x04，是需要读出的扇区数量
	int	0x13			! read it                              ! 72! 
	jnc	ok_load_setup		! ok - continue                        ! 73! 执行完成则跳转到ok_load_setup
	mov	dx,#0x0000                                                     ! 74! 74-77行是出错处理
	mov	ax,#0x0000		! reset the diskette                   ! 75! 
	int	0x13                                                           ! 76! 
	j	load_setup                                                     ! 77! 
                                                                               ! 78! 
ok_load_setup:                                                                 ! 79! 
                                                                               ! 80! 
! Get disk drive parameters, specifically nr of sectors/track                  ! 81! 
                                                                               ! 82! [b;]83-90行利用BIOS中断INT 0x13取磁盘驱动器的参数之一-每磁道最大扇区数
	mov	dl,#0x00                                                       ! 83! dl(驱动器号)=0x00(这里应该是软盘，如果是硬盘，则位7要置位)；
	mov	ax,#0x0800		! AH=8 is get drive parameters         ! 84! ah=0x08表示取磁盘驱动器的参数
	int	0x13                                                           ! 85! 
	mov	ch,#0x00                                                       ! 86! 返回信息中cl的位0-5表示每磁道最大扇区数，位6-7表示最大磁道号高2位
	seg cs                                                                 ! 87! 使得下一条语句采用段超越
	mov	sectors,cx                                                     ! 88! 将每磁道的最大扇区数保存到sectors中
	mov	ax,#INITSEG                                                    ! 89! 上面的中断更改来es的值，现在恢复其为0x9000
	mov	es,ax                                                          ! 90! 
                                                                               ! 91! 
! Print some inane message                                                     ! 92! 
                                                                               ! 93! 
	mov	ah,#0x03		! read cursor pos                      ! 94! 94-96读光标位置
	xor	bh,bh                                                          ! 95! 
	int	0x10                                                           ! 96! 
	                                                                       ! 97! 
	mov	cx,#24                                                         ! 98! [b;]98-102在光标位置显示字符串‘Loading system...’
	mov	bx,#0x0007		! page 0, attribute 7 (normal)         ! 99! 
	mov	bp,#msg1                                                       !100! 
	mov	ax,#0x1301		! write string, move cursor            !101! 
	int	0x10                                                           !102! 
                                                                               !103! 
! ok, we've written the message, now                                           !104! 
! we want to load the system (at 0x10000)                                      !105! 
                                                                               !106! 
	mov	ax,#SYSSEG                                                     !107! 
	mov	es,ax		! segment of 0x010000                          !108! 
	call	read_it                                                        !109! [b;]将ststem模块加载到0x10000(64KB)开始处
	call	kill_motor                                                     !110! [b;]关闭驱动器马达，这样进入内核后就可以知道驱动器的状态了
                                                                               !111! 
! After that we check which root-device to use. If the device is               !112! 
! defined (!= 0), nothing is done and the given device is used.                !113! 
! Otherwise, either /dev/PS0 (2,28) or /dev/at0 (2,8), depending               !114! 
! on the number of sectors that the BIOS reports currently.                    !115! 
                                                                               !116! 
	seg cs                                                                 !117! 
	mov	ax,root_dev                                                    !118! 读存放在第509、510字节处的根文件系统所在设备号
	cmp	ax,#0                                                          !119! 将根设备号与0比较
	jne	root_defined                                                   !120! 如果人工定义过，则自动跳转，否则通过121-130行推断根
	seg cs                                                                 !121! 设备号并保存到第509、510字节处
	mov	bx,sectors                                                     !122! 
	mov	ax,#0x0208		! /dev/ps0 - 1.2Mb                     !123! 
	cmp	bx,#15                                                         !124! 如果每磁道扇区数=15，则根设备号是0x208，即1.2MB软驱
	je	root_defined                                                   !125! 
	mov	ax,#0x021c		! /dev/PS0 - 1.44Mb                    !126! 
	cmp	bx,#18                                                         !127! 如果每磁道扇区数=18，则根设备号是0x21c，即1.44MB软驱
	je	root_defined                                                   !128! 
undef_root:                                                                    !129! 
	jmp undef_root                                                         !130! 
root_defined:                                                                  !131! 
	seg cs                                                                 !132! 
	mov	root_dev,ax                                                    !133! 
                                                                               !134! 
! after that (everyting loaded), we jump to                                    !135! 
! the setup-routine loaded directly after                                      !136! 
! the bootblock:                                                               !137! 
                                                                               !138! 
	jmpi	0,SETUPSEG                                                     !139! [b;]跳转到setup模块开头继续执行
                                                                               !140! 
! This routine loads the system at address 0x10000, making sure                !141! 
! no 64kB boundaries are crossed. We try to load it as fast as                 !142! 
! possible, loading whole tracks whenever we can.                              !143! 
!                                                                              !144! 
! in:	es - starting address segment (normally 0x1000)                        !145! 
!                                                                              !146! 
sread:	.word 1+SETUPLEN	! sectors read of current track                !147! 
head:	.word 0			! current head                                 !148! 
track:	.word 0			! current track                                !149! 
                                                                               !150! 
read_it:                                                                       !151! 
	mov ax,es                                                              !152! 
	test ax,#0x0fff                                                        !153! 
die:	jne die			! es must be at 64kB boundary                  !154! 
	xor bx,bx		! bx is starting address within segment        !155! 
rp_read:                                                                       !156! 
	mov ax,es                                                              !157! 
	cmp ax,#ENDSEG		! have we loaded all yet?                      !158! 
	jb ok1_read                                                            !159! 
	ret                                                                    !160! 
ok1_read:                                                                      !161! 
	seg cs                                                                 !162! 
	mov ax,sectors                                                         !163! 
	sub ax,sread                                                           !164! 
	mov cx,ax                                                              !165! 
	shl cx,#9                                                              !166! 
	add cx,bx                                                              !167! 
	jnc ok2_read                                                           !168! 
	je ok2_read                                                            !169! 
	xor ax,ax                                                              !170! 
	sub ax,bx                                                              !171! 
	shr ax,#9                                                              !172! 
ok2_read:                                                                      !173! 
	call read_track                                                        !174! 
	mov cx,ax                                                              !175! 
	add ax,sread                                                           !176! 
	seg cs                                                                 !177! 
	cmp ax,sectors                                                         !178! 
	jne ok3_read                                                           !179! 
	mov ax,#1                                                              !180! 
	sub ax,head                                                            !181! 
	jne ok4_read                                                           !182! 
	inc track                                                              !183! 
ok4_read:                                                                      !184! 
	mov head,ax                                                            !185! 
	xor ax,ax                                                              !186! 
ok3_read:                                                                      !187! 
	mov sread,ax                                                           !188! 
	shl cx,#9                                                              !189! 
	add bx,cx                                                              !190! 
	jnc rp_read                                                            !191! 
	mov ax,es                                                              !192! 
	add ax,#0x1000                                                         !193! 
	mov es,ax                                                              !194! 
	xor bx,bx                                                              !195! 
	jmp rp_read                                                            !196! 
                                                                               !197! 
read_track:                                                                    !198! 
	push ax                                                                !199! 
	push bx                                                                !200! 
	push cx                                                                !201! 
	push dx                                                                !202! 
	mov dx,track                                                           !203! 
	mov cx,sread                                                           !204! 
	inc cx                                                                 !205! 
	mov ch,dl                                                              !206! 
	mov dx,head                                                            !207! 
	mov dh,dl                                                              !208! 
	mov dl,#0                                                              !209! 
	and dx,#0x0100                                                         !210! 
	mov ah,#2                                                              !211! 
	int 0x13                                                               !212! 
	jc bad_rt                                                              !213! 
	pop dx                                                                 !214! 
	pop cx                                                                 !215! 
	pop bx                                                                 !216! 
	pop ax                                                                 !217! 
	ret                                                                    !218! 
bad_rt:	mov ax,#0                                                              !219! 
	mov dx,#0                                                              !220! 
	int 0x13                                                               !221! 
	pop dx                                                                 !222! 
	pop cx                                                                 !223! 
	pop bx                                                                 !224! 
	pop ax                                                                 !225! 
	jmp read_track                                                         !226! 
                                                                               !227! 
/*                                                                             !228! 
 * This procedure turns off the floppy drive motor, so                         !229! 
 * that we enter the kernel in a known state, and                              !230! 
 * don't have to worry about it later.                                         !231! 
 */                                                                            !232! 
kill_motor:                                                                    !233! 
	push dx                                                                !234! 
	mov dx,#0x3f2                                                          !235! 0x3f2-软驱控制卡的数字输出寄存器端口，只写
	mov al,#0                                                              !236! [b;]往0x3f2端口写0表示-A驱动取、关闭FDC、禁止DMA和中断请求、关闭马达。
	outb                                                                   !237! 将al的内容输出到dx指定的端口去
	pop dx                                                                 !238! 
	ret                                                                    !239! 
                                                                               !240! 
sectors:                                                                       !241! 
	.word 0                                                                !242! 存放当前启动软盘每磁道的扇区数
                                                                               !243! 
msg1:                                                                          !244! 
	.byte 13,10                                                            !245! 
	.ascii "Loading system ..."                                            !246! 
	.byte 13,10,13,10                                                      !247! 
                                                                               !248! 
.org 508                                                                       !249! 
root_dev:                                                                      !250! 存放根文件系统的所在设备的设备号
	.word ROOT_DEV                                                         !251! (默认为0x306，即第二个硬盘的第一个分区，在init/main.c中会用到)
boot_flag:                                                                     !252! 
	.word 0xAA55                                                           !253! 
                                                                               !254! 
.text                                                                          !255! 
endtext:                                                                       !256! 
.data                                                                          !257! 
enddata:                                                                       !258! 
.bss                                                                           !259! 
endbss:                                                                        !260! 

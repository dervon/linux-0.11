/*                                                                            //  1/ 
 *  linux/kernel/fork.c                                                       //  2/ 
 *                                                                            //  3/ 
 *  (C) 1991  Linus Torvalds                                                  //  4/ 
 */                                                                           //  5/ 
                                                                              //  6/ 
/*                                                                            //  7/ 
 *  'fork.c' contains the help-routines for the 'fork' system call            //  8/ 
 * (see also system_call.s), and some misc functions ('verify_area').         //  9/ 
 * Fork is rather simple, once you get the hang of it, but the memory         // 10/ 
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'             // 11/ 
 */                                                                           // 12/ 
#include <errno.h>                                                            // 13/ 
                                                                              // 14/ 
#include <linux/sched.h>                                                      // 15/ 
#include <linux/kernel.h>                                                     // 16/ 
#include <asm/segment.h>                                                      // 17/ 
#include <asm/system.h>                                                       // 18/ 
                                                                              // 19/ 
extern void write_verify(unsigned long address);                              // 20/ 
                                                                              // 21/ 
long last_pid=0;                                                              // 22/ 最新进程的进程号，其值由get_empty_process()生成当前任务的逻辑地址( nr*64MB —— (nr+1)*64MB-1 )
                                                                              // 23/ 
void verify_area(void * addr,int size)                                        // 24/ [b;]对当前任务的逻辑地址中从addr到addr+size这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
{                                                                             // 25/ 
	unsigned long start;                                                  // 26/ 
                                                                              // 27/ 
	start = (unsigned long) addr;                                         // 28/ 
	size += start & 0xfff;                                                // 29/ 取逻辑地址addr的页内偏移量，并将size大小调整为从页面首地址算起的长度
	start &= 0xfffff000;                                                  // 30/ 将start调整为addr页对齐后的地址值
	start += get_base(current->ldt[2]);                                   // 31/ 将start加上用户任务数据段的段基地址值，每个任务的逻辑空间段基地址值不同
	while (size>0) {                                                      // 32/ 依次对从start到start+size用到的页面进行检测是否可写，若不可写则执行共享检验和复制页面操作
		size -= 4096;                                                 // 33/ 
		write_verify(start);                                          // 34/ 
		start += 4096;                                                // 35/ 
	}                                                                     // 36/ 
}                                                                             // 37/ 
                                                                              // 38/ 
int copy_mem(int nr,struct task_struct * p)                                   // 39/ [b;]将p指向的新任务(即任务nr)在线性地址空间中的代码段和数据段基址以及新任务的任务数据结构中代码段开始位置属性值start_code都设为nr*64MB，并建立相关的页目录项、页表及页表项，使新任务的数据段线性地址空间与当前任务的数据段线性地址空间能映射到相同的物理地址(可能代码段和数据段未分离)
{                                                                             // 40/ 
	unsigned long old_data_base,new_data_base,data_limit;                 // 41/ 
	unsigned long old_code_base,new_code_base,code_limit;                 // 42/ 
                                                                              // 43/ 
	code_limit=get_limit(0x0f);                                           // 44/ 将当前任务的代码段描述符中的段界限字段值加一后赋给code_limit
	data_limit=get_limit(0x17);                                           // 45/ 将当前任务的数据段描述符中的段界限字段值加一后赋给data_limit
	old_code_base = get_base(current->ldt[1]);                            // 46/ 将当前任务的代码段描述符中的段基址字段值赋给old_code_base
	old_data_base = get_base(current->ldt[2]);                            // 47/ 将当前任务的数据段描述符中的段基址字段值赋给old_data_base
	if (old_data_base != old_code_base)                                   // 48/ 如果当前任务的代码段基址与数据段基址不同，直接死机
		panic("We don't support separate I&D");                       // 49/ 
	if (data_limit < code_limit)                                          // 50/ 如果当前任务的数据段限长小于代码段限长，直接死机
		panic("Bad data_limit");                                      // 51/ 
	new_data_base = new_code_base = nr * 0x4000000;                       // 52/ 
	p->start_code = new_code_base;                                        // 53/ 将新任务的任务数据结构中代码段开始位置属性值start_code设为nr*64MB
	set_base(p->ldt[1],new_code_base);                                    // 54/ 将新任务的代码段描述符中的段基地值字段设为nr*64MB
	set_base(p->ldt[2],new_data_base);                                    // 55/ 将新任务的数据段描述符中的段基地值字段设为nr*64MB
	if (copy_page_tables(old_data_base,new_data_base,data_limit)) {       // 56/ 尝试为线性地址new_data_base到new_data_base+data_limit建立相关的页目录项、页表及页表项，使其与线性地址old_data_base到old_data_base+data_limit能映射到相同的物理地址，如果成功直接返回0，失败就执行下面的操作
		free_page_tables(new_data_base,data_limit);                   // 57/ 如果将从线性地址new_data_base(要求4MB对齐)到线性地址new_data_base+data_limit(字节长度)映射时用到的页目录项清空，并将这些页目录项指向的页表中所有页表项清空，并将这些页表项映射的物理页面全部释放
		return -ENOMEM;                                               // 58/ 返回出错码(内存不足)
	}                                                                     // 59/ 
	return 0;                                                             // 60/ 
}                                                                             // 61/ 
                                                                              // 62/ 
/*                                                                            // 63/ 
 *  Ok, this is the main fork-routine. It copies the system process           // 64/ 
 * information (task[nr]) and sets up the necessary registers. It             // 65/ 
 * also copies the data segment in it's entirety.                             // 66/ 
 */                                                                           // 67/ 
int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,         // 68/ [b;]为新任务(即任务nr)的任务数据结构申请一个物理页，将当前任务的任务数据结构内容复制进去，对新任务的TSS内容作初始化，将新任务(即任务nr)在线性地址空间中的代码段和数据段基址以及新任务的任务数据结构中代码段开始位置属性值start_code都设为nr*64MB，并建立相关的页目录项、页表及页表项，使新任务的数据段线性地址空间与当前任务的数据段线性地址空间能映射到相同的物理地址(可能代码段和数据段未分离)，“文件相关，以后添加”，在GDT为新任务建立LDT和TSS段描述符，最后将新任务置为就绪态
		long ebx,long ecx,long edx,                                   // 69/ 
		long fs,long es,long ds,                                      // 70/ 
		long eip,long cs,long eflags,long esp,long ss)                // 71/ 
{                                                                             // 72/ 
	struct task_struct *p;                                                // 73/ 
	int i;                                                                // 74/ 
	struct file *f;                                                       // 75/ 
                                                                              // 76/ 
	p = (struct task_struct *) get_free_page();                           // 77/ 在主内存中申请一个物理页
	if (!p)                                                               // 78/ 
		return -EAGAIN;                                               // 79/ 
	task[nr] = p;                                                         // 80/ 将新任务的任务数据结构指针指向申请的物理页
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */    // 81/ 将当前任务的任务数据结构内容复制到申请的物理页中(从页基址开始存放)，作为新任务的任务数据结构
	p->state = TASK_UNINTERRUPTIBLE;                                      // 82/ 将新任务的状态置为不可中断睡眠状态，防止内核调度执行
	p->pid = last_pid;                                                    // 83/ 将新任务的进程号置为新进程号last_pid
	p->father = current->pid;                                             // 84/ 将新任务的父进程号置为当前进程的进程号
	p->counter = p->priority;                                             // 85/ 将新任务的可运行时间滴答数置为初始的优先权值
	p->signal = 0;                                                        // 86/ 将新任务的信号位图复位
	p->alarm = 0;                                                         // 87/ 将新任务的报警定时值置0
	p->leader = 0;		/* process leadership doesn't inherit */      // 88/ 将新任务置为会话的非首进程
	p->utime = p->stime = 0;                                              // 89/ 将新任务的用户态和内核态运行时间都置0
	p->cutime = p->cstime = 0;                                            // 90/ 将新任务的子进程用户态和内核态时间都置0
	p->start_time = jiffies;                                              // 91/ 将新任务新建开始运行的时刻置为从开机开始算起到目前的滴答数
	p->tss.back_link = 0;                                                 // 92/ 将新任务的前一个任务的TSS段选择符置0
	p->tss.esp0 = PAGE_SIZE + (long) p;                                   // 93/ 将新任务内核栈的栈指针指向申请页的顶端
	p->tss.ss0 = 0x10;                                                    // 94/ 将新任务内核栈的栈段选择符导向内核数据段
	p->tss.eip = eip;                                                     // 95/ 将新任务的eip置为中断返回后的代码位置
	p->tss.eflags = eflags;                                               // 96/ 将新任务的eflags置为中断之前的eflags值
	p->tss.eax = 0;                                                       // 97/ 当fork()返回时，新任务返回0
	p->tss.ecx = ecx;                                                     // 98/ 将新任务的寄存器置为中断之前的状态
	p->tss.edx = edx;                                                     // 99/ 将新任务的寄存器置为中断之前的状态
	p->tss.ebx = ebx;                                                     //100/ 将新任务的寄存器置为中断之前的状态
	p->tss.esp = esp;                                                     //101/ 将新任务的寄存器置为中断之前的状态
	p->tss.ebp = ebp;                                                     //102/ 将新任务的寄存器置为中断之前的状态
	p->tss.esi = esi;                                                     //103/ 将新任务的寄存器置为中断之前的状态
	p->tss.edi = edi;                                                     //104/ 将新任务的寄存器置为中断之前的状态
	p->tss.es = es & 0xffff;                                              //105/ 将新任务的寄存器置为中断之前的状态
	p->tss.cs = cs & 0xffff;                                              //106/ 将新任务的寄存器置为中断之前的状态
	p->tss.ss = ss & 0xffff;                                              //107/ 将新任务的寄存器置为中断之前的状态
	p->tss.ds = ds & 0xffff;                                              //108/ 将新任务的寄存器置为中断之前的状态
	p->tss.fs = fs & 0xffff;                                              //109/ 将新任务的寄存器置为中断之前的状态
	p->tss.gs = gs & 0xffff;                                              //110/ 将新任务的寄存器置为中断之前的状态
	p->tss.ldt = _LDT(nr);                                                //111/ 置将新任务的LDT段选择符值(LDT段在GDT中的偏移值+0+00)
	p->tss.trace_bitmap = 0x80000000;                                     //112/ 将新任务的T为置0，IO映射区基地址置为0x8000(因为大于TSS段限长104，所以新任务没有IO许可映射区)
	if (last_task_used_math == current)                                   //113/ 如果当前任务使用了协处理器
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));               //114/ 则清除CR0中的任务切换标志TS，并将当前任务的协处理器状态保存到新任务的tss.i387结构中
	if (copy_mem(nr,p)) {                                                 //115/ 将p指向的新任务(即任务nr)在线性地址空间中的代码段和数据段基址以及新任务的任务数据结构中代码段开始位置属性值start_code都设为nr*64MB，并建立相关的页目录项、页表及页表项，使新任务的数据段线性地址空间与当前任务的数据段线性地址空间能映射到相同的物理地址(可能代码段和数据段未分离)
		task[nr] = NULL;                                              //116/ 清空分配的任务槽
		free_page((long) p);                                          //117/ 释放物理地址p指向的新任务的任务数据结构所在页面
		return -EAGAIN;                                               //118/ 返回错误码(资源暂时不可用)
	}                                                                     //119/ 
	for (i=0; i<NR_OPEN;i++)                                              //120/ [r;]120-128文件有关
		if (f=p->filp[i])                                             //121/ 
			f->f_count++;                                         //122/ 
	if (current->pwd)                                                     //123/ 
		current->pwd->i_count++;                                      //124/ 
	if (current->root)                                                    //125/ 
		current->root->i_count++;                                     //126/ 
	if (current->executable)                                              //127/ 
		current->executable->i_count++;                               //128/ 
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));                  //129/ 将新任务的TSS段基地址和固定的值(段界限=104、G=0、P=1、DPL=0、B(忙位)=0)组成的新任务的TSS段描述符写入GDT中的对应位置
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));                  //130/ 将新任务的LDT段基地址和固定的值(段界限=104、G=0、P=1、DPL=0)组成的新任务的LDT段描述符写入GDT中的对应位置
	p->state = TASK_RUNNING;	/* do this last, just in case */      //131/ 将新任务的状态置为就绪态
	return last_pid;                                                      //132/ 返回新任务的进程号
}                                                                             //133/ 
                                                                              //134/ 
int find_empty_process(void)                                                  //135/ [b;]为新进程取得不重复的进程号last_pid，返回一个空的任务槽的序号(0-63中的一个)
{                                                                             //136/ 
	int i;                                                                //137/ 
                                                                              //138/ 
	repeat:                                                               //139/ 
		if ((++last_pid)<0) last_pid=1;                               //140/ 将last_pid加一，如果last_pid(有符号型)<0，表示last_pid超过了界限，则将其置为1
		for(i=0 ; i<NR_TASKS ; i++)                                   //141/ 遍寻64(0-63)个任务槽
			if (task[i] && task[i]->pid == last_pid) goto repeat; //142/ 如果其中有某个任务已经将last_pid作为了进程号，则返回去重取新的进程号
	for(i=1 ; i<NR_TASKS ; i++)                                           //143/ 遍寻64(0-63)个任务槽
		if (!task[i])                                                 //144/ 如果有个空槽，则将该槽的序号返回；[b;](返回值是一个整数或一个指针，默认用eax来传递)
			return i;                                             //145/ 
	return -EAGAIN;                                                       //146/ 如果没有空槽，则返回错误码(资源暂时不可用)[b;](返回值是一个整数或一个指针，默认用eax来传递)
}                                                                             //147/ 

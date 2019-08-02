/*                                                                                 //  1/ 
 *  linux/kernel/traps.c                                                           //  2/ 
 *                                                                                 //  3/ 
 *  (C) 1991  Linus Torvalds                                                       //  4/ 
 */                                                                                //  5/ 
                                                                                   //  6/ 
/*                                                                                 //  7/ 
 * 'Traps.c' handles hardware traps and faults after we have saved some            //  8/ 
 * state in 'asm.s'. Currently mostly a debugging-aid, will be extended            //  9/ 
 * to mainly kill the offending process (probably by giving it a signal,           // 10/ 
 * but possibly by killing it outright if necessary).                              // 11/ 
 */                                                                                // 12/ 
#include <string.h>                                                                // 13/ 
                                                                                   // 14/ 
#include <linux/head.h>                                                            // 15/ 
#include <linux/sched.h>                                                           // 16/ 
#include <linux/kernel.h>                                                          // 17/ 
#include <asm/system.h>                                                            // 18/ 
#include <asm/segment.h>                                                           // 19/ 
#include <asm/io.h>                                                                // 20/ 
                                                                                   // 21/ 
#define get_seg_byte(seg,addr) ({ \                                                // 22/ 将段地址seg:偏移地址addr指定的地址处的一个字节写入al寄存器中，并返回该字节
register char __res; \                                                             // 23/ 
__asm__("push %%fs;mov %%ax,%%fs;movb %%fs:%2,%%al;pop %%fs" \                     // 24/ 
	:"=a" (__res):"0" (seg),"m" (*(addr))); \                                  // 25/ 
__res;})                                                                           // 26/ 
                                                                                   // 27/ 
#define get_seg_long(seg,addr) ({ \                                                // 28/ 将段地址seg:偏移地址addr指定的地址处的一个字写入eax寄存器中，并返回该字
register unsigned long __res; \                                                    // 29/ 
__asm__("push %%fs;mov %%ax,%%fs;movl %%fs:%2,%%eax;pop %%fs" \                    // 30/ 
	:"=a" (__res):"0" (seg),"m" (*(addr))); \                                  // 31/ 
__res;})                                                                           // 32/ 
                                                                                   // 33/ 
#define _fs() ({ \                                                                 // 34/ 取出段寄存器fs中的内容
register unsigned short __res; \                                                   // 35/ 
__asm__("mov %%fs,%%ax":"=a" (__res):); \                                          // 36/ 
__res;})                                                                           // 37/ 
                                                                                   // 38/ 
int do_exit(long code);                                                            // 39/ 
                                                                                   // 40/ 
void page_exception(void);                                                         // 41/ 
                                                                                   // 42/ 
void divide_error(void);                                                           // 43/ int 0(kernel/asm.s)——43-61行用于在trap_init()中设置相应中断门描述符
void debug(void);                                                                  // 44/ int 1(kernel/asm.s)
void nmi(void);                                                                    // 45/ int 2(kernel/asm.s)
void int3(void);                                                                   // 46/ int 3(kernel/asm.s)
void overflow(void);                                                               // 47/ int 4(kernel/asm.s)
void bounds(void);                                                                 // 48/ int 5(kernel/asm.s)
void invalid_op(void);                                                             // 49/ int 6(kernel/asm.s)
void device_not_available(void);                                                   // 50/ int 7(kernel/asm.s)
void double_fault(void);                                                           // 51/ int 8(kernel/asm.s)
void coprocessor_segment_overrun(void);                                            // 52/ int 9(kernel/asm.s)
void invalid_TSS(void);                                                            // 53/ int 10(kernel/asm.s)
void segment_not_present(void);                                                    // 54/ int 11(kernel/asm.s)
void stack_segment(void);                                                          // 55/ int 12(kernel/asm.s)
void general_protection(void);                                                     // 56/ int 13(kernel/asm.s)
void page_fault(void);                                                             // 57/ int 14(mm/page.s)
void coprocessor_error(void);                                                      // 58/ int 16(kernel/system_call.s)
void reserved(void);                                                               // 59/ int 15(kernel/asm.s)
void parallel_interrupt(void);                                                     // 60/ int 39(kernel/system_call.s)
void irq13(void);                                                                  // 61/ int 45(kernel/asm.s)
                                                                                   // 62/ 
static void die(char * str,long esp_ptr,long nr)                                   // 63/ [b;]打印出错中断的名称、出错号、调用程序的EIP、EFLAGS、ESP、fs段寄存器值、段的基址、段的长度、进程号pid、任务号、10字节指令码及16字节堆栈内容(str为固定的字符串，esp_ptr为指向eip的指针，nr为错误号)
{                                                                                  // 64/ 
	long * esp = (long *) esp_ptr;                                             // 65/ 
	int i;                                                                     // 66/ 
                                                                                   // 67/ 
	printk("%s: %04x\n\r",str,nr&0xffff);                                      // 68/ 打印出错中断的名称、出错号
	printk("EIP:\t%04x:%p\nEFLAGS:\t%p\nESP:\t%04x:%p\n",                      // 69/ 打印压入栈中的当前调用进程自身的CS、EIP、EFLAGS、用户栈SS和ESP的值
		esp[1],esp[0],esp[2],esp[4],esp[3]);                               // 70/ 
	printk("fs: %04x\n",_fs());                                                // 71/ 打印fs的值，此时估计是内核数据段选择符
	printk("base: %p, limit: %p\n",get_base(current->ldt[1]),get_limit(0x17)); // 72/ [r;]打印当前进程自身代码段的基地址和数据段的长度
	if (esp[4] == 0x17) {                                                      // 73/ 如果当前调用进程的用户栈段就是用户数据段，则打印16字节的堆栈内容
		printk("Stack: ");                                                 // 74/ 
		for (i=0;i<4;i++)                                                  // 75/ 
			printk("%p ",get_seg_long(0x17,i+(long *)esp[3]));         // 76/ 
		printk("\n");                                                      // 77/ 
	}                                                                          // 78/ 
	str(i);                                                                    // 79/ [r;]取当前运行任务的任务号
	printk("Pid: %d, process nr: %d\n\r",current->pid,0xffff & i);             // 80/ 打印当前任务的进程标识号和任务号
	for(i=0;i<10;i++)                                                          // 81/ 
		printk("%02x ",0xff & get_seg_byte(esp[1],(i+(char *)esp[0])));    // 82/ 将中断返回后的位置处的后面10字节打印出来
	printk("\n\r");                                                            // 83/ 
	do_exit(11);		/* play segment exception */                       // 84/ [r;]退出返回
}                                                                                  // 85/ 
                                                                                   // 86/ 
void do_double_fault(long esp, long error_code)                                    // 87/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  // 88/ 
	die("double fault",esp,error_code);                                        // 89/ 
}                                                                                  // 90/ 
                                                                                   // 91/ 
void do_general_protection(long esp, long error_code)                              // 92/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  // 93/ 
	die("general protection",esp,error_code);                                  // 94/ 
}                                                                                  // 95/ 
                                                                                   // 96/ 
void do_divide_error(long esp, long error_code)                                    // 97/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  // 98/ 
	die("divide error",esp,error_code);                                        // 99/ 
}                                                                                  //100/ 
                                                                                   //101/ 
void do_int3(long * esp, long error_code,                                          //102/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
		long fs,long es,long ds,                                           //103/ 
		long ebp,long esi,long edi,                                        //104/ 
		long edx,long ecx,long ebx,long eax)                               //105/ 
{                                                                                  //106/ 
	int tr;                                                                    //107/ 
                                                                                   //108/ 
	__asm__("str %%ax":"=a" (tr):"0" (0));                                     //109/ 取出当前任务寄存器TR的值保存到tr中
	printk("eax\t\tebx\t\tecx\t\tedx\n\r%8x\t%8x\t%8x\t%8x\n\r",               //110/ 110-116行打印各个寄存器的值
		eax,ebx,ecx,edx);                                                  //111/ 
	printk("esi\t\tedi\t\tebp\t\tesp\n\r%8x\t%8x\t%8x\t%8x\n\r",               //112/ 
		esi,edi,ebp,(long) esp);                                           //113/ 
	printk("\n\rds\tes\tfs\ttr\n\r%4x\t%4x\t%4x\t%4x\n\r",                     //114/ 
		ds,es,fs,tr);                                                      //115/ 
	printk("EIP: %8x   CS: %4x  EFLAGS: %8x\n\r",esp[0],esp[1],esp[2]);        //116/ 
}                                                                                  //117/ 
                                                                                   //118/ 
void do_nmi(long esp, long error_code)                                             //119/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //120/ 
	die("nmi",esp,error_code);                                                 //121/ 
}                                                                                  //122/ 
                                                                                   //123/ 
void do_debug(long esp, long error_code)                                           //124/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //125/ 
	die("debug",esp,error_code);                                               //126/ 
}                                                                                  //127/ 
                                                                                   //128/ 
void do_overflow(long esp, long error_code)                                        //129/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //130/ 
	die("overflow",esp,error_code);                                            //131/ 
}                                                                                  //132/ 
                                                                                   //133/ 
void do_bounds(long esp, long error_code)                                          //134/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //135/ 
	die("bounds",esp,error_code);                                              //136/ 
}                                                                                  //137/ 
                                                                                   //138/ 
void do_invalid_op(long esp, long error_code)                                      //139/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //140/ 
	die("invalid operand",esp,error_code);                                     //141/ 
}                                                                                  //142/ 
                                                                                   //143/ 
void do_device_not_available(long esp, long error_code)                            //144/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //145/ 
	die("device not available",esp,error_code);                                //146/ 
}                                                                                  //147/ 
                                                                                   //148/ 
void do_coprocessor_segment_overrun(long esp, long error_code)                     //149/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //150/ 
	die("coprocessor segment overrun",esp,error_code);                         //151/ 
}                                                                                  //152/ 
                                                                                   //153/ 
void do_invalid_TSS(long esp,long error_code)                                      //154/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //155/ 
	die("invalid TSS",esp,error_code);                                         //156/ 
}                                                                                  //157/ 
                                                                                   //158/ 
void do_segment_not_present(long esp,long error_code)                              //159/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //160/ 
	die("segment not present",esp,error_code);                                 //161/ 
}                                                                                  //162/ 
                                                                                   //163/ 
void do_stack_segment(long esp,long error_code)                                    //164/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //165/ 
	die("stack segment",esp,error_code);                                       //166/ 
}                                                                                  //167/ 
                                                                                   //168/ 
void do_coprocessor_error(long esp, long error_code)                               //169/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //170/ 
	if (last_task_used_math != current)                                        //171/ 
		return;                                                            //172/ 
	die("coprocessor error",esp,error_code);                                   //173/ 
}                                                                                  //174/ 
                                                                                   //175/ 
void do_reserved(long esp, long error_code)                                        //176/ [b;]以do_开头的函数是asm.s中对应中断处理程序调用的C函数
{                                                                                  //177/ 
	die("reserved (15,17-47) error",esp,error_code);                           //178/ 
}                                                                                  //179/ 
                                                                                   //180/ 
void trap_init(void)                                                               //181/ [b;]在中断描述符表IDT[0-47]中建立一些陷阱门描述符,
{                                                                                  //182/ 
	int i;                                                                     //183/ 
                                                                                   //184/ 
	set_trap_gate(0,&divide_error);                                            //185/ 在idt[0]处放一个陷阱门描述符(段选择子0x0008;偏移&divide_error;P=1;DPL=0)
	set_trap_gate(1,&debug);                                                   //186/ 在idt[1]处放一个陷阱门描述符(段选择子0x0008;偏移&debug;P=1;DPL=0)
	set_trap_gate(2,&nmi);                                                     //187/ 在idt[2]处放一个陷阱门描述符(段选择子0x0008;偏移&nmi;P=1;DPL=0)
	set_system_gate(3,&int3);	/* int3-5 can be called from all */        //188/ 在idt[3]处放一个陷阱门描述符(段选择子0x0008;偏移&int3;P=1;DPL=3,可被所有程序执行)
	set_system_gate(4,&overflow);                                              //189/ 在idt[4]处放一个陷阱门描述符(段选择子0x0008;偏移&overflow;P=1;DPL=3,可被所有程序执行)
	set_system_gate(5,&bounds);                                                //190/ 在idt[5]处放一个陷阱门描述符(段选择子0x0008;偏移&bounds;P=1;DPL=3,可被所有程序执行)
	set_trap_gate(6,&invalid_op);                                              //191/ 在idt[6]处放一个陷阱门描述符(段选择子0x0008;偏移&invalid_op;P=1;DPL=0)
	set_trap_gate(7,&device_not_available);                                    //192/ 在idt[7]处放一个陷阱门描述符(段选择子0x0008;偏移&device_not_available;P=1;DPL=0)
	set_trap_gate(8,&double_fault);                                            //193/ 在idt[8]处放一个陷阱门描述符(段选择子0x0008;偏移&double_fault;P=1;DPL=0)
	set_trap_gate(9,&coprocessor_segment_overrun);                             //194/ 在idt[9]处放一个陷阱门描述符(段选择子0x0008;偏移&coprocessor_segment_overrun;P=1;DPL=0)
	set_trap_gate(10,&invalid_TSS);                                            //195/ 在idt[10]处放一个陷阱门描述符(段选择子0x0008;偏移&invalid_TSS;P=1;DPL=0)
	set_trap_gate(11,&segment_not_present);                                    //196/ 在idt[11]处放一个陷阱门描述符(段选择子0x0008;偏移&segment_not_present;P=1;DPL=0)
	set_trap_gate(12,&stack_segment);                                          //197/ 在idt[12]处放一个陷阱门描述符(段选择子0x0008;偏移&stack_segment;P=1;DPL=0)
	set_trap_gate(13,&general_protection);                                     //198/ 在idt[13]处放一个陷阱门描述符(段选择子0x0008;偏移&general_protection;P=1;DPL=0)
	set_trap_gate(14,&page_fault);                                             //199/ 在idt[14]处放一个陷阱门描述符(段选择子0x0008;偏移&page_fault;P=1;DPL=0)
	set_trap_gate(15,&reserved);                                               //200/ 在idt[15]处放一个陷阱门描述符(段选择子0x0008;偏移&reserved;P=1;DPL=0)
	set_trap_gate(16,&coprocessor_error);                                      //201/ 在idt[16]处放一个陷阱门描述符(段选择子0x0008;偏移&coprocessor_error;P=1;DPL=0)
	for (i=17;i<48;i++)                                                        //202/ 
		set_trap_gate(i,&reserved);                                        //203/ 在idt[17-47]处都放陷阱门描述符(段选择子0x0008;偏移&reserved;P=1;DPL=0)
	set_trap_gate(45,&irq13);                                                  //204/ 将idt[45](协处理器中断)处的保留描述符覆盖为一个陷阱门描述符(段选择子0x0008;偏移&irq13;P=1;DPL=0)
	outb_p(inb_p(0x21)&0xfb,0x21);                                             //205/ 允许8259A主芯片的IRQ2中断请求
	outb(inb_p(0xA1)&0xdf,0xA1);                                               //206/ 允许8259A从芯片的IRQ13(协处理器)中断请求
	set_trap_gate(39,&parallel_interrupt);                                     //207/ 将idt[39](并行口1中断)处的保留描述符覆盖为一个陷阱门描述符(段选择子0x0008;偏移&parallel_interrupt;P=1;DPL=0)
}                                                                                  //208/ 

/*                                                                 // 1/ 
 * linux/kernel/math/math_emulate.c                                // 2/ 
 *                                                                 // 3/ 
 * (C) 1991 Linus Torvalds                                         // 4/ 
 */                                                                // 5/ 
                                                                   // 6/ 
/*                                                                 // 7/ 
 * This directory should contain the math-emulation code.          // 8/ 
 * Currently only results in a signal.                             // 9/ 
 */                                                                //10/ 
                                                                   //11/ 
#include <signal.h>                                                //12/ 
                                                                   //13/ 
#include <linux/sched.h>                                           //14/ 
#include <linux/kernel.h>                                          //15/ 
#include <asm/segment.h>                                           //16/ 
                                                                   //17/ 
void math_emulate(long edi, long esi, long ebp, long sys_call_ret, //18/ [b;]协处理器仿真函数——中断处理程序调用的C函数
	long eax,long ebx,long ecx,long edx,                       //19/ 
	unsigned short fs,unsigned short es,unsigned short ds,     //20/ 
	unsigned long eip,unsigned short cs,unsigned long eflags,  //21/ 
	unsigned short ss, unsigned long esp)                      //22/ [r;]ss和esp的位置应该互换
{                                                                  //23/ 
	unsigned char first, second;                               //24/ 
                                                                   //25/ 
/* 0x0007 means user code space */                                 //26/ 
	if (cs != 0x000F) {                                        //27/ [r;]如果cs不等于0x000F(表示局部描述符表中的第1项，即用户数据段吗，为什么不是0x0007)，表示cs一定是内核代码段选择符
		printk("math_emulate: %04x:%08x\n\r",cs,eip);      //28/ 则打印此时的cs:eip值
		panic("Math emulation needed in kernel");          //29/ 显示信息‘内核中需要数学仿真’，然后死机
	}                                                          //30/ 
	first = get_fs_byte((char *)((*&eip)++));                  //31/ 
	second = get_fs_byte((char *)((*&eip)++));                 //32/ 
	printk("%04x:%08x %02x %02x\n\r",cs,eip-2,first,second);   //33/ 
	current->signal |= 1<<(SIGFPE-1);                          //34/ 给当前进程发送浮点异常信号SIGFPE
}                                                                  //35/ 
                                                                   //36/ 
void math_error(void)                                              //37/ [b;]协处理器出错处理函数
{                                                                  //38/ 
	__asm__("fnclex");                                         //39/ 协处理器指令——清楚所有异常标志、忙标志和状态字位7
	if (last_task_used_math)                                   //40/ 如果上个任务使用过协处理器
		last_task_used_math->signal |= 1<<(SIGFPE-1);      //41/ 则向上个任务发送协处理器异常信号
}                                                                  //42/ 

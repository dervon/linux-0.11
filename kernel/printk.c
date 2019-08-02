/*                                                                  // 1/ 
 *  linux/kernel/printk.c                                           // 2/ 
 *                                                                  // 3/ 
 *  (C) 1991  Linus Torvalds                                        // 4/ 
 */                                                                 // 5/ 
                                                                    // 6/ 
/*                                                                  // 7/ 
 * When in kernel-mode, we cannot use printf, as fs is liable to    // 8/ 
 * point to 'interesting' things. Make a printf with fs-saving, and // 9/ 
 * all is well.                                                     //10/ 
 */                                                                 //11/ 
#include <stdarg.h>                                                 //12/ 
#include <stddef.h>                                                 //13/ 
                                                                    //14/ 
#include <linux/kernel.h>                                           //15/ 
                                                                    //16/ 
static char buf[1024];                                              //17/ 显示用临时缓冲区
                                                                    //18/ 
extern int vsprintf(char * buf, const char * fmt, va_list args);    //19/ 
                                                                    //20/ 
int printk(const char *fmt, ...)                                    //21/ [b;]内核使用的显示函数，只能在内核代码中使用
{                                                                   //22/ 
	va_list args;                                               //23/ va_list是字符指针类型
	int i;                                                      //24/ 
                                                                    //25/ 
	va_start(args, fmt);                                        //26/ 使指针args指向printk的可变参数表的第一个参数
	i=vsprintf(buf,fmt,args);                                   //27/ 调用vsprintf，返回值i为输出字符串的长度
	va_end(args);                                               //28/ 修改args使其在重新调用va_start之前不能被使用
	__asm__("push %%fs\n\t"                                     //29/ 压栈保存fs的值
		"push %%ds\n\t"                                     //30/ 30-31行将ds的值复制到fs中
		"pop %%fs\n\t"                                      //31/ 
		"pushl %0\n\t"                                      //32/ 压入输出字符串的长度
		"pushl $_buf\n\t"                                   //33/ 压入上面定义的显示用临时缓冲区的地址
		"pushl $0\n\t"                                      //34/ 压入数值0，是显示通道号channel
		"call _tty_write\n\t"                               //35/ [r;]调用tty_write函数
		"addl $8,%%esp\n\t"                                 //36/ 丢弃两个入栈参数(buf,channel)
		"popl %0\n\t"                                       //37/ 恢复原fs寄存器的值
		"pop %%fs"                                          //38/ 
		::"r" (i):"ax","cx","dx");                          //39/ 告诉编译器，寄存器ax、cx、dx的值可能已经改变
	return i;                                                   //40/ 返回输出字符串的长度
}                                                                   //41/ 

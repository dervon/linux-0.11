/*                                                                     // 1/ 
 *  linux/kernel/panic.c                                               // 2/ 
 *                                                                     // 3/ 
 *  (C) 1991  Linus Torvalds                                           // 4/ 
 */                                                                    // 5/ 
                                                                       // 6/ 
/*                                                                     // 7/ 
 * This function is used through-out the kernel (includeinh mm and fs) // 8/ 
 * to indicate a major problem.                                        // 9/ 
 */                                                                    //10/ 
#include <linux/kernel.h>                                              //11/ 
#include <linux/sched.h>                                               //12/ 
                                                                       //13/ 
void sys_sync(void);	/* it's really int */                          //14/ 
                                                                       //15/ 
volatile void panic(const char * s)                                    //16/ [b;]显示内核错误信息并使系统进入死循环
{                                                                      //17/ 
	printk("Kernel panic: %s\n\r",s);                              //18/ 
	if (current == task[0])                                        //19/ 如果当前任务就是任务0
		printk("In swapper task - not syncing\n\r");           //20/ 打印信息
	else                                                           //21/ 如果当前任务不是任务0
		sys_sync();                                            //22/ [r;]文件有关
	for(;;);                                                       //23/ 死机
}                                                                      //24/ 

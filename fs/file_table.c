/*                               //1/ 
 *  linux/fs/file_table.c        //2/ 
 *                               //3/ 
 *  (C) 1991  Linus Torvalds     //4/ 
 */                              //5/ 
                                 //6/ 
#include <linux/fs.h>            //7/ 
                                 //8/ 
struct file file_table[NR_FILE]; //9/ 文件表(数组)(NR_FILE = 64个表项，即系统同时只能打开64个文件)

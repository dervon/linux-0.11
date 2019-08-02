#ifndef _SYS_UTSNAME_H                     // 1/ 
#define _SYS_UTSNAME_H                     // 2/ 
                                           // 3/ 
#include <sys/types.h>                     // 4/ 
                                           // 5/ 
struct utsname {                           // 6/ 
	char sysname[9];                   // 7/ 当前运行系统的名称
	char nodename[9];                  // 8/ 与实现相关的网络中节点名称(主机名称)
	char release[9];                   // 9/ 本操作系统实现的当前发行级别
	char version[9];                   //10/ 本次发行的操作系统版本级别
	char machine[9];                   //11/ 系统运行的硬件类型名称
};                                         //12/ 
                                           //13/ 
extern int uname(struct utsname * utsbuf); //14/ 
                                           //15/ 
#endif                                     //16/ 

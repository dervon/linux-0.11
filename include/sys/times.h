#ifndef _TIMES_H                      // 1/ 
#define _TIMES_H                      // 2/ 
                                      // 3/ 
#include <sys/types.h>                // 4/ 
                                      // 5/ 
struct tms {                          // 6/ 
	time_t tms_utime;             // 7/ 进程用户态使用的CPU时间
	time_t tms_stime;             // 8/ 进程内核态使用的CPU时间
	time_t tms_cutime;            // 9/ 已终止的子进程使用的用户态CPU时间总和
	time_t tms_cstime;            //10/ 已终止的子进程使用的内核态CPU时间总和
};                                    //11/ 
                                      //12/ 
extern time_t times(struct tms * tp); //13/ 
                                      //14/ 
#endif                                //15/ 

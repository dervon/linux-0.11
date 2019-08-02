#ifndef _SYS_WAIT_H                                             // 1/ 
#define _SYS_WAIT_H                                             // 2/ 
                                                                // 3/ 
#include <sys/types.h>                                          // 4/ 
                                                                // 5/ 
#define _LOW(v)		( (v) & 0377)                           // 6/ 
#define _HIGH(v)	( ((v) >> 8) & 0377)                    // 7/ 
                                                                // 8/ 
/* options for waitpid, WUNTRACED not supported */              // 9/ 
#define WNOHANG		1                                       //10/ 表示若没有子进程处于退出或终止状态就立刻返回
#define WUNTRACED	2                                       //11/ 表示子进程是停止的也马上返回，报告停止执行的子进程状态
                                                                //12/ 
#define WIFEXITED(s)	(!((s)&0xFF)                            //13/ 
#define WIFSTOPPED(s)	(((s)&0xFF)==0x7F)                      //14/ 
#define WEXITSTATUS(s)	(((s)>>8)&0xFF)                         //15/ 
#define WTERMSIG(s)	((s)&0x7F)                              //16/ 
#define WSTOPSIG(s)	(((s)>>8)&0xFF)                         //17/ 
#define WIFSIGNALED(s)	(((unsigned int)(s)-1 & 0xFFFF) < 0xFF) //18/ 
                                                                //19/ 
pid_t wait(int *stat_loc);                                      //20/ 
pid_t waitpid(pid_t pid, int *stat_loc, int options);           //21/ 
                                                                //22/ 
#endif                                                          //23/ 

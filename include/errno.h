#ifndef _ERRNO_H                                                      // 1/ 
#define _ERRNO_H                                                      // 2/ 
                                                                      // 3/ 
/*                                                                    // 4/ 
 * ok, as I hadn't got any other source of information about          // 5/ 
 * possible error numbers, I was forced to use the same numbers       // 6/ 
 * as minix.                                                          // 7/ 
 * Hopefully these are posix or something. I wouldn't know (and posix // 8/ 
 * isn't telling me - they want $$$ for their f***ing standard).      // 9/ 
 *                                                                    //10/ 
 * We don't use the _SIGN cludge of minix, so kernel returns must     //11/ 
 * see to the sign by themselves.                                     //12/ 
 *                                                                    //13/ 
 * NOTE! Remember to change strerror() if you change this file!       //14/ 
 */                                                                   //15/ 
                                                                      //16/ 
extern int errno;                                                     //17/ 
                                                                      //18/ 
#define ERROR		99                                            //19/ 一般错误
#define EPERM		 1                                            //20/ 操作没有许可
#define ENOENT		 2                                            //21/ 文件或目录不存在
#define ESRCH		 3                                            //22/ 指定的进程不存在
#define EINTR		 4                                            //23/ 中断的系统调用
#define EIO		 5                                            //24/ 输入/输出错
#define ENXIO		 6                                            //25/ 指定设备或地址不存在
#define E2BIG		 7                                            //26/ 参数列表太长
#define ENOEXEC		 8                                            //27/ 执行程序格式错误
#define EBADF		 9                                            //28/ 文件句柄(描述符)错误
#define ECHILD		10                                            //29/ 子进程不存在
#define EAGAIN		11                                            //30/ 资源暂时不可用
#define ENOMEM		12                                            //31/ 内存不足
#define EACCES		13                                            //32/ 没有许可权限
#define EFAULT		14                                            //33/ 地址错
#define ENOTBLK		15                                            //34/ 不是块设备 
#define EBUSY		16                                            //35/ 资源正忙
#define EEXIST		17                                            //36/ 文件已存在
#define EXDEV		18                                            //37/ 非法连接
#define ENODEV		19                                            //38/ 设备不存在
#define ENOTDIR		20                                            //39/ 不是目录文件
#define EISDIR		21                                            //40/ 是目录文件
#define EINVAL		22                                            //41/ 参数无效
#define ENFILE		23                                            //42/ 系统打开文件数太多
#define EMFILE		24                                            //43/ 打开文件数太多
#define ENOTTY		25                                            //44/ 不恰当的IO控制操作(tty终端)
#define ETXTBSY		26                                            //45/ 不再使用
#define EFBIG		27                                            //46/ 文件太大
#define ENOSPC		28                                            //47/ 设备已满(设备已经没有空间)
#define ESPIPE		29                                            //48/ 无效的文件指针重定位
#define EROFS		30                                            //49/ 文件系统只读
#define EMLINK		31                                            //50/ 连接太多
#define EPIPE		32                                            //51/ 管道错
#define EDOM		33                                            //52/ 域出错
#define ERANGE		34                                            //53/ 结果太大
#define EDEADLK		35                                            //54/ 避免资源死锁
#define ENAMETOOLONG	36                                            //55/ 文件名太长
#define ENOLCK		37                                            //56/ 没有锁定可用
#define ENOSYS		38                                            //57/ 功能还没有实现
#define ENOTEMPTY	39                                            //58/ 目录不空
                                                                      //59/ 
#endif                                                                //60/ 

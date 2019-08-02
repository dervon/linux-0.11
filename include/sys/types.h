#ifndef _SYS_TYPES_H                      // 1/ 
#define _SYS_TYPES_H                      // 2/ 
                                          // 3/ 
#ifndef _SIZE_T                           // 4/ 
#define _SIZE_T                           // 5/ 
typedef unsigned int size_t;              // 6/ 用于对象的大小(长度)
#endif                                    // 7/ 
                                          // 8/ 
#ifndef _TIME_T                           // 9/ 
#define _TIME_T                           //10/ 
typedef long time_t;                      //11/ 用于时间(以秒计)
#endif                                    //12/ 
                                          //13/ 
#ifndef _PTRDIFF_T                        //14/ 
#define _PTRDIFF_T                        //15/ 
typedef long ptrdiff_t;                   //16/ 
#endif                                    //17/ 
                                          //18/ 
#ifndef NULL                              //19/ 
#define NULL ((void *) 0)                 //20/ 用NULL来代替0指针，即空指针
#endif                                    //21/ 
                                          //22/ 
typedef int pid_t;                        //23/ 用于进程号和进程组号
typedef unsigned short uid_t;             //24/ 用于用户号(用户标识号)
typedef unsigned char gid_t;              //25/ 用于组号
typedef unsigned short dev_t;             //26/ 用于设备号
typedef unsigned short ino_t;             //27/ 用于文件序列号
typedef unsigned short mode_t;            //28/ 用于某些文件属性
typedef unsigned short umode_t;           //29/ 
typedef unsigned char nlink_t;            //30/ 用于连接计数
typedef int daddr_t;                      //31/ 
typedef long off_t;                       //32/ 用于文件大小(长度)
typedef unsigned char u_char;             //33/ 无符号字符类型
typedef unsigned short ushort;            //34/ 无符号短整型类型
                                          //35/ 
typedef struct { int quot,rem; } div_t;   //36/ 用于DIV操作
typedef struct { long quot,rem; } ldiv_t; //37/ 用于长DIV操作
                                          //38/ 
struct ustat {                            //39/ 文件系统参数结构，用于ustat()函数。最后两个字段未使用，总是返回NULL指针
	daddr_t f_tfree;                  //40/ 系统总空闲块数
	ino_t f_tinode;                   //41/ 总空闲i节点数
	char f_fname[6];                  //42/ 文件系统名称
	char f_fpack[6];                  //43/ 文件系统压缩名称
};                                        //44/ 
                                          //45/ 
#endif                                    //46/ 

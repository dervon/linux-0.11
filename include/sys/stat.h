#ifndef _SYS_STAT_H                                           // 1/ 
#define _SYS_STAT_H                                           // 2/ 
                                                              // 3/ 
#include <sys/types.h>                                        // 4/ 
                                                              // 5/ 
struct stat {                                                 // 6/ 
	dev_t	st_dev;                                       // 7/ 
	ino_t	st_ino;                                       // 8/ 
	umode_t	st_mode;                                      // 9/ 
	nlink_t	st_nlink;                                     //10/ 
	uid_t	st_uid;                                       //11/ 
	gid_t	st_gid;                                       //12/ 
	dev_t	st_rdev;                                      //13/ 
	off_t	st_size;                                      //14/ 
	time_t	st_atime;                                     //15/ 
	time_t	st_mtime;                                     //16/ 
	time_t	st_ctime;                                     //17/ 
};                                                            //18/ 
                                                              //19/ 
#define S_IFMT  00170000                                      //20/ 
#define S_IFREG  0100000                                      //21/ 
#define S_IFBLK  0060000                                      //22/ 
#define S_IFDIR  0040000                                      //23/ 
#define S_IFCHR  0020000                                      //24/ 
#define S_IFIFO  0010000                                      //25/ 
#define S_ISUID  0004000                                      //26/ 
#define S_ISGID  0002000                                      //27/ 
#define S_ISVTX  0001000                                      //28/ 
                                                              //29/ 
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)           //30/ 将m(i节点的文件类型和访问权限属性)中的文件类型字段(15-12位)取出来，判断是否是常规文件的i节点
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)           //31/ 将m(i节点的文件类型和访问权限属性)中的文件类型字段(15-12位)取出来，判断是否是目录文件的i节点
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)           //32/ 将m(i节点的文件类型和访问权限属性)中的文件类型字段(15-12位)取出来，判断是否是字符设备文件的i节点
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)           //33/ 将m(i节点的文件类型和访问权限属性)中的文件类型字段(15-12位)取出来，判断是否是块设备文件的i节点
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)           //34/ 将m(i节点的文件类型和访问权限属性)中的文件类型字段(15-12位)取出来，判断是否是FIFO(管道)文件的i节点
                                                              //35/ 
#define S_IRWXU 00700                                         //36/ 
#define S_IRUSR 00400                                         //37/ 
#define S_IWUSR 00200                                         //38/ 
#define S_IXUSR 00100                                         //39/ 
                                                              //40/ 
#define S_IRWXG 00070                                         //41/ 
#define S_IRGRP 00040                                         //42/ 
#define S_IWGRP 00020                                         //43/ 
#define S_IXGRP 00010                                         //44/ 
                                                              //45/ 
#define S_IRWXO 00007                                         //46/ 
#define S_IROTH 00004                                         //47/ 
#define S_IWOTH 00002                                         //48/ 
#define S_IXOTH 00001                                         //49/ 
                                                              //50/ 
extern int chmod(const char *_path, mode_t mode);             //51/ 
extern int fstat(int fildes, struct stat *stat_buf);          //52/ 
extern int mkdir(const char *_path, mode_t mode);             //53/ 
extern int mkfifo(const char *_path, mode_t mode);            //54/ 
extern int stat(const char *filename, struct stat *stat_buf); //55/ 
extern mode_t umask(mode_t mask);                             //56/ 
                                                              //57/ 
#endif                                                        //58/ 

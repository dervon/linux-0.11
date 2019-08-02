#ifndef _TIME_H                                                                 // 1/ 
#define _TIME_H                                                                 // 2/ 
                                                                                // 3/ 
#ifndef _TIME_T                                                                 // 4/ 
#define _TIME_T                                                                 // 5/ 
typedef long time_t;                                                            // 6/ 从GMT1970年1月1日午夜0时起开始计的时间(秒)
#endif                                                                          // 7/ 
                                                                                // 8/ 
#ifndef _SIZE_T                                                                 // 9/ 
#define _SIZE_T                                                                 //10/ 
typedef unsigned int size_t;                                                    //11/ 
#endif                                                                          //12/ 
                                                                                //13/ 
#define CLOCKS_PER_SEC 100                                                      //14/ 系统时钟滴答频率，100HZ，即每秒100个滴答
                                                                                //15/ 
typedef long clock_t;                                                           //16/ 从进程开始系统经过的滴答数
                                                                                //17/ 
struct tm {                                                                     //18/ 
	int tm_sec;                                                             //19/ 秒数【0-59】
	int tm_min;                                                             //20/ 分钟数【0-59】
	int tm_hour;                                                            //21/ 小时数【0-59】
	int tm_mday;                                                            //22/ 1个月的天数【0-31】
	int tm_mon;                                                             //23/ 1年中月份【0-11】
	int tm_year;                                                            //24/ 从1900年开始的年数
	int tm_wday;                                                            //25/ 1星期中的某天【0-6】(星期天 = 0)
	int tm_yday;                                                            //26/ 1年中的某天【0-365】
	int tm_isdst;                                                           //27/ 夏令时标志。正数-使用；0-没有使用；负数-无效
};                                                                              //28/ 
                                                                                //29/ 30-40行都是对标准C库提供的函数(内核本身不包括)的声明,默认有extern属性
clock_t clock(void);                                                            //30/ 确实处理器使用时间，返回程序所用处理器时间(滴答数)的近似值
time_t time(time_t * tp);                                                       //31/ 取时间(秒数)，返回从1970.1.1:0:0:0开始到此时经过的秒数(称为日历时间)
double difftime(time_t time2, time_t time1);                                    //32/ 计算时间差，返回时间time2与time1之间经过的秒数
time_t mktime(struct tm * tp);                                                  //33/ 将tm结构表示的时间转换为日历时间
                                                                                //34/ 
char * asctime(const struct tm * tp);                                           //35/ 将tm结构表示的时间转换成一个字符串，返回指向该串的指针
char * ctime(const time_t * tp);                                                //36/ 将日历时间转换成一个字符串形式，如“Web Jun 30 21:49:08:1993\n”
struct tm * gmtime(const time_t *tp);                                           //37/ 将日历时间转换成tm结构表示的UTC时间(UTC-世界时间代码Universal Time Code)
struct tm *localtime(const time_t * tp);                                        //38/ 将日历时间转换成tm结构表示的指定时区的时间
size_t strftime(char * s, size_t smax, const char * fmt, const struct tm * tp); //39/ 将tm结构表示的时间利用格式字符串fmt转换成最大长度为smax的字符串并将结果存在s中
void tzset(void);                                                               //40/ 初始化时间转换信息，使用环境变量TZ，对zname变量进行初始化
                                                                                //41/ 
#endif                                                                          //42/ 

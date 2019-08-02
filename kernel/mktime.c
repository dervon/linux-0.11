/*                                                                        // 1/ 
 *  linux/kernel/mktime.c                                                 // 2/ 
 *                                                                        // 3/ 
 *  (C) 1991  Linus Torvalds                                              // 4/ 
 */                                                                       // 5/ 
                                                                          // 6/ 
#include <time.h>                                                         // 7/ 
                                                                          // 8/ 
/*                                                                        // 9/ 
 * This isn't the library routine, it is only used in the kernel.         //10/ 
 * as such, we don't care about years<1970 etc, but assume everything     //11/ 
 * is ok. Similarly, TZ etc is happily ignored. We just do everything     //12/ 
 * as easily as possible. Let's find something public for the library     //13/ 
 * routines (although I think minix times is public).                     //14/ 
 */                                                                       //15/ 
/*                                                                        //16/ 
 * PS. I hate whoever though up the year 1970 - couldn't they have gotten //17/ 
 * a leap-year instead? I also hate Gregorius, pope or no. I'm grumpy.    //18/ 
 */                                                                       //19/ 
#define MINUTE 60                                                         //20/ 每分钟对应的秒数，给下面的kernel_mktime使用
#define HOUR (60*MINUTE)                                                  //21/ 每小时对应的秒数，给下面的kernel_mktime使用
#define DAY (24*HOUR)                                                     //22/ 每天对应的秒数，给下面的kernel_mktime使用
#define YEAR (365*DAY)                                                    //23/ 每年对应的秒数，给下面的kernel_mktime使用
                                                                          //24/ 
/* interestingly, we assume leap-years */                                 //25/ 
static int month[12] = {                                                  //26/ 月份对应的秒数，给下面的kernel_mktime使用
	0,                                                                //27/ 
	DAY*(31),                                                         //28/ 
	DAY*(31+29),                                                      //29/ 二月份都是以润年算的，所以后面还要每个平年减去一天
	DAY*(31+29+31),                                                   //30/ 
	DAY*(31+29+31+30),                                                //31/ 
	DAY*(31+29+31+30+31),                                             //32/ 
	DAY*(31+29+31+30+31+30),                                          //33/ 
	DAY*(31+29+31+30+31+30+31),                                       //34/ 
	DAY*(31+29+31+30+31+30+31+31),                                    //35/ 
	DAY*(31+29+31+30+31+30+31+31+30),                                 //36/ 
	DAY*(31+29+31+30+31+30+31+31+30+31),                              //37/ 
	DAY*(31+29+31+30+31+30+31+31+30+31+30)                            //38/ 
};                                                                        //39/ 
                                                                          //40/ 
long kernel_mktime(struct tm * tm)                                        //41/ [b;]返回从1970.1.1:0:0:0开始到此时经过的秒数
{                                                                         //42/ 
	long res;                                                         //43/ 
	int year;                                                         //44/ 
                                                                          //45/ 
	year = tm->tm_year - 70;                                          //46/ 算出1970年到现在的年数
/* magic offsets (y+1) needed to get leapyears right.*/                   //47/ 
	res = YEAR*year + DAY*((year+1)/4);                               //48/ 计算年对应的秒数，每个润年加一天时间(润年计算方法为1+(y-3)/4，即(y+1)/4)
	res += month[tm->tm_mon];                                         //49/ 计算月对应的秒数
/* and (y+2) here. If it wasn't a leap-year, we have to adjust */         //50/ 
	if (tm->tm_mon>1 && ((year+2)%4))                                 //51/ 求出当年是否是平年，如果是，二月过完要减去一天，因为上面二月份默认设为了29天
		res -= DAY;                                               //52/ 
	res += DAY*(tm->tm_mday-1);                                       //53/ 计算日对应的秒数
	res += HOUR*tm->tm_hour;                                          //54/ 计算时对应的秒数
	res += MINUTE*tm->tm_min;                                         //55/ 计算分对应的秒数
	res += tm->tm_sec;                                                //56/ 计算秒对应的秒数
	return res;                                                       //57/ 返回从1970.1.1:0:0:0开始到此时经过的秒数
}                                                                         //58/ 

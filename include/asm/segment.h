extern inline unsigned char get_fs_byte(const char * addr)                   // 1/ 将fs段中addr指定的内存地址处的一个字节存取出来，并返回
{                                                                            // 2/ 
	unsigned register char _v;                                           // 3/ 
                                                                             // 4/ 
	__asm__ ("movb %%fs:%1,%0":"=r" (_v):"m" (*addr));                   // 5/ 
	return _v;                                                           // 6/ 
}                                                                            // 7/ 
                                                                             // 8/ 
extern inline unsigned short get_fs_word(const unsigned short *addr)         // 9/ 将fs段中addr指定的内存地址处的一个字存取出来，并返回
{                                                                            //10/ 
	unsigned short _v;                                                   //11/ 
                                                                             //12/ 
	__asm__ ("movw %%fs:%1,%0":"=r" (_v):"m" (*addr));                   //13/ 
	return _v;                                                           //14/ 
}                                                                            //15/ 
                                                                             //16/ 
extern inline unsigned long get_fs_long(const unsigned long *addr)           //17/ 将fs段中addr指定的内存地址处的一个长字存取出来，并返回
{                                                                            //18/ 
	unsigned long _v;                                                    //19/ 
                                                                             //20/ 
	__asm__ ("movl %%fs:%1,%0":"=r" (_v):"m" (*addr)); \                 //21/ 
	return _v;                                                           //22/ 
}                                                                            //23/ 
                                                                             //24/ 
extern inline void put_fs_byte(char val,char *addr)                          //25/ 将一字节val存放在fs段中addr指定的内存地址处
{                                                                            //26/ 
__asm__ ("movb %0,%%fs:%1"::"r" (val),"m" (*addr));                          //27/ 
}                                                                            //28/ 
                                                                             //29/ 
extern inline void put_fs_word(short val,short * addr)                       //30/ 将一字val存放在fs段中addr指定的内存地址处
{                                                                            //31/ 
__asm__ ("movw %0,%%fs:%1"::"r" (val),"m" (*addr));                          //32/ 
}                                                                            //33/ 
                                                                             //34/ 
extern inline void put_fs_long(unsigned long val,unsigned long * addr)       //35/ 将一长字val存放在fs段中addr指定的内存地址处
{                                                                            //36/ 
__asm__ ("movl %0,%%fs:%1"::"r" (val),"m" (*addr));                          //37/ 
}                                                                            //38/ 
                                                                             //39/ 
/*                                                                           //40/ 
 * Someone who knows GNU asm better than I should double check the followig. //41/ 
 * It seems to work, but I don't know if I'm doing something subtly wrong.   //42/ 
 * --- TYT, 11/24/91                                                         //43/ 
 * [ nothing wrong here, Linus ]                                             //44/ 
 */                                                                          //45/ 
                                                                             //46/ 
extern inline unsigned long get_fs()                                         //47/ 
{                                                                            //48/ 
	unsigned short _v;                                                   //49/ 
	__asm__("mov %%fs,%%ax":"=a" (_v):);                                 //50/ 
	return _v;                                                           //51/ 
}                                                                            //52/ 
                                                                             //53/ 
extern inline unsigned long get_ds()                                         //54/ 
{                                                                            //55/ 
	unsigned short _v;                                                   //56/ 
	__asm__("mov %%ds,%%ax":"=a" (_v):);                                 //57/ 
	return _v;                                                           //58/ 
}                                                                            //59/ 
                                                                             //60/ 
extern inline void set_fs(unsigned long val)                                 //61/ 
{                                                                            //62/ 
	__asm__("mov %0,%%fs"::"a" ((unsigned short) val));                  //63/ 
}                                                                            //64/ 
                                                                             //65/ 

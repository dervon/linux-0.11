#ifndef _STRING_H_                                                              //  1/ 
#define _STRING_H_                                                              //  2/ 
                                                                                //  3/ 
#ifndef NULL                                                                    //  4/ 
#define NULL ((void *) 0)                                                       //  5/ 定义NULL为空指针
#endif                                                                          //  6/ 
                                                                                //  7/ 
#ifndef _SIZE_T                                                                 //  8/ 
#define _SIZE_T                                                                 //  9/ 
typedef unsigned int size_t;                                                    // 10/ 定义size_t为unsigned int型
#endif                                                                          // 11/ 
                                                                                // 12/ 
extern char * strerror(int errno);                                              // 13/ 
                                                                                // 14/ 
/*                                                                              // 15/ 
 * This string-include defines all string functions as inline                   // 16/ 
 * functions. Use gcc. It also assumes ds=es=data space, this should be         // 17/ 
 * normal. Most of the string-functions are rather heavily hand-optimized,      // 18/ 
 * see especially strtok,strstr,str[c]spn. They should work, but are not        // 19/ 
 * very easy to understand. Everything is done entirely within the register     // 20/ 
 * set, making the functions fast and clean. String instructions have been      // 21/ 
 * used through-out, making for "slightly" unclear code :-)                     // 22/ 
 *                                                                              // 23/ 
 *		(C) 1991 Linus Torvalds                                         // 24/ 
 */                                                                             // 25/ 
                                                                                // 26/ 
extern inline char * strcpy(char * dest,const char *src)                        // 27/ 
{                                                                               // 28/ 
__asm__("cld\n"                                                                 // 29/ 
	"1:\tlodsb\n\t"                                                         // 30/ 
	"stosb\n\t"                                                             // 31/ 
	"testb %%al,%%al\n\t"                                                   // 32/ 
	"jne 1b"                                                                // 33/ 
	::"S" (src),"D" (dest):"si","di","ax");                                 // 34/ 
return dest;                                                                    // 35/ 
}                                                                               // 36/ 
                                                                                // 37/ 
extern inline char * strncpy(char * dest,const char *src,int count)             // 38/ 
{                                                                               // 39/ 
__asm__("cld\n"                                                                 // 40/ 
	"1:\tdecl %2\n\t"                                                       // 41/ 
	"js 2f\n\t"                                                             // 42/ 
	"lodsb\n\t"                                                             // 43/ 
	"stosb\n\t"                                                             // 44/ 
	"testb %%al,%%al\n\t"                                                   // 45/ 
	"jne 1b\n\t"                                                            // 46/ 
	"rep\n\t"                                                               // 47/ 
	"stosb\n"                                                               // 48/ 
	"2:"                                                                    // 49/ 
	::"S" (src),"D" (dest),"c" (count):"si","di","ax","cx");                // 50/ 
return dest;                                                                    // 51/ 
}                                                                               // 52/ 
                                                                                // 53/ 
extern inline char * strcat(char * dest,const char * src)                       // 54/ 
{                                                                               // 55/ 
__asm__("cld\n\t"                                                               // 56/ 
	"repne\n\t"                                                             // 57/ 
	"scasb\n\t"                                                             // 58/ 
	"decl %1\n"                                                             // 59/ 
	"1:\tlodsb\n\t"                                                         // 60/ 
	"stosb\n\t"                                                             // 61/ 
	"testb %%al,%%al\n\t"                                                   // 62/ 
	"jne 1b"                                                                // 63/ 
	::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff):"si","di","ax","cx");   // 64/ 
return dest;                                                                    // 65/ 
}                                                                               // 66/ 
                                                                                // 67/ 
extern inline char * strncat(char * dest,const char * src,int count)            // 68/ 
{                                                                               // 69/ 
__asm__("cld\n\t"                                                               // 70/ 
	"repne\n\t"                                                             // 71/ 
	"scasb\n\t"                                                             // 72/ 
	"decl %1\n\t"                                                           // 73/ 
	"movl %4,%3\n"                                                          // 74/ 
	"1:\tdecl %3\n\t"                                                       // 75/ 
	"js 2f\n\t"                                                             // 76/ 
	"lodsb\n\t"                                                             // 77/ 
	"stosb\n\t"                                                             // 78/ 
	"testb %%al,%%al\n\t"                                                   // 79/ 
	"jne 1b\n"                                                              // 80/ 
	"2:\txorl %2,%2\n\t"                                                    // 81/ 
	"stosb"                                                                 // 82/ 
	::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)             // 83/ 
	:"si","di","ax","cx");                                                  // 84/ 
return dest;                                                                    // 85/ 
}                                                                               // 86/ 
                                                                                // 87/ 
extern inline int strcmp(const char * cs,const char * ct)                       // 88/ 
{                                                                               // 89/ 
register int __res __asm__("ax");                                               // 90/ 
__asm__("cld\n"                                                                 // 91/ 
	"1:\tlodsb\n\t"                                                         // 92/ 
	"scasb\n\t"                                                             // 93/ 
	"jne 2f\n\t"                                                            // 94/ 
	"testb %%al,%%al\n\t"                                                   // 95/ 
	"jne 1b\n\t"                                                            // 96/ 
	"xorl %%eax,%%eax\n\t"                                                  // 97/ 
	"jmp 3f\n"                                                              // 98/ 
	"2:\tmovl $1,%%eax\n\t"                                                 // 99/ 
	"jl 3f\n\t"                                                             //100/ 
	"negl %%eax\n"                                                          //101/ 
	"3:"                                                                    //102/ 
	:"=a" (__res):"D" (cs),"S" (ct):"si","di");                             //103/ 
return __res;                                                                   //104/ 
}                                                                               //105/ 
                                                                                //106/ 
extern inline int strncmp(const char * cs,const char * ct,int count)            //107/ 
{                                                                               //108/ 
register int __res __asm__("ax");                                               //109/ 
__asm__("cld\n"                                                                 //110/ 
	"1:\tdecl %3\n\t"                                                       //111/ 
	"js 2f\n\t"                                                             //112/ 
	"lodsb\n\t"                                                             //113/ 
	"scasb\n\t"                                                             //114/ 
	"jne 3f\n\t"                                                            //115/ 
	"testb %%al,%%al\n\t"                                                   //116/ 
	"jne 1b\n"                                                              //117/ 
	"2:\txorl %%eax,%%eax\n\t"                                              //118/ 
	"jmp 4f\n"                                                              //119/ 
	"3:\tmovl $1,%%eax\n\t"                                                 //120/ 
	"jl 4f\n\t"                                                             //121/ 
	"negl %%eax\n"                                                          //122/ 
	"4:"                                                                    //123/ 
	:"=a" (__res):"D" (cs),"S" (ct),"c" (count):"si","di","cx");            //124/ 
return __res;                                                                   //125/ 
}                                                                               //126/ 
                                                                                //127/ 
extern inline char * strchr(const char * s,char c)                              //128/ 
{                                                                               //129/ 
register char * __res __asm__("ax");                                            //130/ 
__asm__("cld\n\t"                                                               //131/ 
	"movb %%al,%%ah\n"                                                      //132/ 
	"1:\tlodsb\n\t"                                                         //133/ 
	"cmpb %%ah,%%al\n\t"                                                    //134/ 
	"je 2f\n\t"                                                             //135/ 
	"testb %%al,%%al\n\t"                                                   //136/ 
	"jne 1b\n\t"                                                            //137/ 
	"movl $1,%1\n"                                                          //138/ 
	"2:\tmovl %1,%0\n\t"                                                    //139/ 
	"decl %0"                                                               //140/ 
	:"=a" (__res):"S" (s),"0" (c):"si");                                    //141/ 
return __res;                                                                   //142/ 
}                                                                               //143/ 
                                                                                //144/ 
extern inline char * strrchr(const char * s,char c)                             //145/ 
{                                                                               //146/ 
register char * __res __asm__("dx");                                            //147/ 
__asm__("cld\n\t"                                                               //148/ 
	"movb %%al,%%ah\n"                                                      //149/ 
	"1:\tlodsb\n\t"                                                         //150/ 
	"cmpb %%ah,%%al\n\t"                                                    //151/ 
	"jne 2f\n\t"                                                            //152/ 
	"movl %%esi,%0\n\t"                                                     //153/ 
	"decl %0\n"                                                             //154/ 
	"2:\ttestb %%al,%%al\n\t"                                               //155/ 
	"jne 1b"                                                                //156/ 
	:"=d" (__res):"0" (0),"S" (s),"a" (c):"ax","si");                       //157/ 
return __res;                                                                   //158/ 
}                                                                               //159/ 
                                                                                //160/ 
extern inline int strspn(const char * cs, const char * ct)                      //161/ 
{                                                                               //162/ 
register char * __res __asm__("si");                                            //163/ 
__asm__("cld\n\t"                                                               //164/ 
	"movl %4,%%edi\n\t"                                                     //165/ 
	"repne\n\t"                                                             //166/ 
	"scasb\n\t"                                                             //167/ 
	"notl %%ecx\n\t"                                                        //168/ 
	"decl %%ecx\n\t"                                                        //169/ 
	"movl %%ecx,%%edx\n"                                                    //170/ 
	"1:\tlodsb\n\t"                                                         //171/ 
	"testb %%al,%%al\n\t"                                                   //172/ 
	"je 2f\n\t"                                                             //173/ 
	"movl %4,%%edi\n\t"                                                     //174/ 
	"movl %%edx,%%ecx\n\t"                                                  //175/ 
	"repne\n\t"                                                             //176/ 
	"scasb\n\t"                                                             //177/ 
	"je 1b\n"                                                               //178/ 
	"2:\tdecl %0"                                                           //179/ 
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)                //180/ 
	:"ax","cx","dx","di");                                                  //181/ 
return __res-cs;                                                                //182/ 
}                                                                               //183/ 
                                                                                //184/ 
extern inline int strcspn(const char * cs, const char * ct)                     //185/ 
{                                                                               //186/ 
register char * __res __asm__("si");                                            //187/ 
__asm__("cld\n\t"                                                               //188/ 
	"movl %4,%%edi\n\t"                                                     //189/ 
	"repne\n\t"                                                             //190/ 
	"scasb\n\t"                                                             //191/ 
	"notl %%ecx\n\t"                                                        //192/ 
	"decl %%ecx\n\t"                                                        //193/ 
	"movl %%ecx,%%edx\n"                                                    //194/ 
	"1:\tlodsb\n\t"                                                         //195/ 
	"testb %%al,%%al\n\t"                                                   //196/ 
	"je 2f\n\t"                                                             //197/ 
	"movl %4,%%edi\n\t"                                                     //198/ 
	"movl %%edx,%%ecx\n\t"                                                  //199/ 
	"repne\n\t"                                                             //200/ 
	"scasb\n\t"                                                             //201/ 
	"jne 1b\n"                                                              //202/ 
	"2:\tdecl %0"                                                           //203/ 
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)                //204/ 
	:"ax","cx","dx","di");                                                  //205/ 
return __res-cs;                                                                //206/ 
}                                                                               //207/ 
                                                                                //208/ 
extern inline char * strpbrk(const char * cs,const char * ct)                   //209/ 
{                                                                               //210/ 
register char * __res __asm__("si");                                            //211/ 
__asm__("cld\n\t"                                                               //212/ 
	"movl %4,%%edi\n\t"                                                     //213/ 
	"repne\n\t"                                                             //214/ 
	"scasb\n\t"                                                             //215/ 
	"notl %%ecx\n\t"                                                        //216/ 
	"decl %%ecx\n\t"                                                        //217/ 
	"movl %%ecx,%%edx\n"                                                    //218/ 
	"1:\tlodsb\n\t"                                                         //219/ 
	"testb %%al,%%al\n\t"                                                   //220/ 
	"je 2f\n\t"                                                             //221/ 
	"movl %4,%%edi\n\t"                                                     //222/ 
	"movl %%edx,%%ecx\n\t"                                                  //223/ 
	"repne\n\t"                                                             //224/ 
	"scasb\n\t"                                                             //225/ 
	"jne 1b\n\t"                                                            //226/ 
	"decl %0\n\t"                                                           //227/ 
	"jmp 3f\n"                                                              //228/ 
	"2:\txorl %0,%0\n"                                                      //229/ 
	"3:"                                                                    //230/ 
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)                //231/ 
	:"ax","cx","dx","di");                                                  //232/ 
return __res;                                                                   //233/ 
}                                                                               //234/ 
                                                                                //235/ 
extern inline char * strstr(const char * cs,const char * ct)                    //236/ 
{                                                                               //237/ 
register char * __res __asm__("ax");                                            //238/ 
__asm__("cld\n\t" \                                                             //239/ 
	"movl %4,%%edi\n\t"                                                     //240/ 
	"repne\n\t"                                                             //241/ 
	"scasb\n\t"                                                             //242/ 
	"notl %%ecx\n\t"                                                        //243/ 
	"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */ //244/ 
	"movl %%ecx,%%edx\n"                                                    //245/ 
	"1:\tmovl %4,%%edi\n\t"                                                 //246/ 
	"movl %%esi,%%eax\n\t"                                                  //247/ 
	"movl %%edx,%%ecx\n\t"                                                  //248/ 
	"repe\n\t"                                                              //249/ 
	"cmpsb\n\t"                                                             //250/ 
	"je 2f\n\t"		/* also works for empty string, see above */    //251/ 
	"xchgl %%eax,%%esi\n\t"                                                 //252/ 
	"incl %%esi\n\t"                                                        //253/ 
	"cmpb $0,-1(%%eax)\n\t"                                                 //254/ 
	"jne 1b\n\t"                                                            //255/ 
	"xorl %%eax,%%eax\n\t"                                                  //256/ 
	"2:"                                                                    //257/ 
	:"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)                //258/ 
	:"cx","dx","di","si");                                                  //259/ 
return __res;                                                                   //260/ 
}                                                                               //261/ 
                                                                                //262/ 
extern inline int strlen(const char * s)                                        //263/ 
{                                                                               //264/ 
register int __res __asm__("cx");                                               //265/ 
__asm__("cld\n\t"                                                               //266/ 
	"repne\n\t"                                                             //267/ 
	"scasb\n\t"                                                             //268/ 
	"notl %0\n\t"                                                           //269/ 
	"decl %0"                                                               //270/ 
	:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):"di");                   //271/ 
return __res;                                                                   //272/ 
}                                                                               //273/ 
                                                                                //274/ 
extern char * ___strtok;                                                        //275/ 
                                                                                //276/ 
extern inline char * strtok(char * s,const char * ct)                           //277/ 
{                                                                               //278/ 
register char * __res __asm__("si");                                            //279/ 
__asm__("testl %1,%1\n\t"                                                       //280/ 
	"jne 1f\n\t"                                                            //281/ 
	"testl %0,%0\n\t"                                                       //282/ 
	"je 8f\n\t"                                                             //283/ 
	"movl %0,%1\n"                                                          //284/ 
	"1:\txorl %0,%0\n\t"                                                    //285/ 
	"movl $-1,%%ecx\n\t"                                                    //286/ 
	"xorl %%eax,%%eax\n\t"                                                  //287/ 
	"cld\n\t"                                                               //288/ 
	"movl %4,%%edi\n\t"                                                     //289/ 
	"repne\n\t"                                                             //290/ 
	"scasb\n\t"                                                             //291/ 
	"notl %%ecx\n\t"                                                        //292/ 
	"decl %%ecx\n\t"                                                        //293/ 
	"je 7f\n\t"			/* empty delimeter-string */            //294/ 
	"movl %%ecx,%%edx\n"                                                    //295/ 
	"2:\tlodsb\n\t"                                                         //296/ 
	"testb %%al,%%al\n\t"                                                   //297/ 
	"je 7f\n\t"                                                             //298/ 
	"movl %4,%%edi\n\t"                                                     //299/ 
	"movl %%edx,%%ecx\n\t"                                                  //300/ 
	"repne\n\t"                                                             //301/ 
	"scasb\n\t"                                                             //302/ 
	"je 2b\n\t"                                                             //303/ 
	"decl %1\n\t"                                                           //304/ 
	"cmpb $0,(%1)\n\t"                                                      //305/ 
	"je 7f\n\t"                                                             //306/ 
	"movl %1,%0\n"                                                          //307/ 
	"3:\tlodsb\n\t"                                                         //308/ 
	"testb %%al,%%al\n\t"                                                   //309/ 
	"je 5f\n\t"                                                             //310/ 
	"movl %4,%%edi\n\t"                                                     //311/ 
	"movl %%edx,%%ecx\n\t"                                                  //312/ 
	"repne\n\t"                                                             //313/ 
	"scasb\n\t"                                                             //314/ 
	"jne 3b\n\t"                                                            //315/ 
	"decl %1\n\t"                                                           //316/ 
	"cmpb $0,(%1)\n\t"                                                      //317/ 
	"je 5f\n\t"                                                             //318/ 
	"movb $0,(%1)\n\t"                                                      //319/ 
	"incl %1\n\t"                                                           //320/ 
	"jmp 6f\n"                                                              //321/ 
	"5:\txorl %1,%1\n"                                                      //322/ 
	"6:\tcmpb $0,(%0)\n\t"                                                  //323/ 
	"jne 7f\n\t"                                                            //324/ 
	"xorl %0,%0\n"                                                          //325/ 
	"7:\ttestl %0,%0\n\t"                                                   //326/ 
	"jne 8f\n\t"                                                            //327/ 
	"movl %0,%1\n"                                                          //328/ 
	"8:"                                                                    //329/ 
	:"=b" (__res),"=S" (___strtok)                                          //330/ 
	:"0" (___strtok),"1" (s),"g" (ct)                                       //331/ 
	:"ax","cx","dx","di");                                                  //332/ 
return __res;                                                                   //333/ 
}                                                                               //334/ 
                                                                                //335/ 
extern inline void * memcpy(void * dest,const void * src, int n)                //336/ 从源地址src处开始复制n个字节到目的地址dest处(此时可能ds、es都指向了同一个段)
{                                                                               //337/ 
__asm__("cld\n\t"                                                               //338/ 
	"rep\n\t"                                                               //339/ 
	"movsb"                                                                 //340/ 
	::"c" (n),"S" (src),"D" (dest)                                          //341/ 
	:"cx","si","di");                                                       //342/ 
return dest;                                                                    //343/ 
}                                                                               //344/ 
                                                                                //345/ 
extern inline void * memmove(void * dest,const void * src, int n)               //346/ 
{                                                                               //347/ 
if (dest<src)                                                                   //348/ 
__asm__("cld\n\t"                                                               //349/ 
	"rep\n\t"                                                               //350/ 
	"movsb"                                                                 //351/ 
	::"c" (n),"S" (src),"D" (dest)                                          //352/ 
	:"cx","si","di");                                                       //353/ 
else                                                                            //354/ 
__asm__("std\n\t"                                                               //355/ 
	"rep\n\t"                                                               //356/ 
	"movsb"                                                                 //357/ 
	::"c" (n),"S" (src+n-1),"D" (dest+n-1)                                  //358/ 
	:"cx","si","di");                                                       //359/ 
return dest;                                                                    //360/ 
}                                                                               //361/ 
                                                                                //362/ 
extern inline int memcmp(const void * cs,const void * ct,int count)             //363/ 
{                                                                               //364/ 
register int __res __asm__("ax");                                               //365/ 
__asm__("cld\n\t"                                                               //366/ 
	"repe\n\t"                                                              //367/ 
	"cmpsb\n\t"                                                             //368/ 
	"je 1f\n\t"                                                             //369/ 
	"movl $1,%%eax\n\t"                                                     //370/ 
	"jl 1f\n\t"                                                             //371/ 
	"negl %%eax\n"                                                          //372/ 
	"1:"                                                                    //373/ 
	:"=a" (__res):"0" (0),"D" (cs),"S" (ct),"c" (count)                     //374/ 
	:"si","di","cx");                                                       //375/ 
return __res;                                                                   //376/ 
}                                                                               //377/ 
                                                                                //378/ 
extern inline void * memchr(const void * cs,char c,int count)                   //379/ 
{                                                                               //380/ 
register void * __res __asm__("di");                                            //381/ 
if (!count)                                                                     //382/ 
	return NULL;                                                            //383/ 
__asm__("cld\n\t"                                                               //384/ 
	"repne\n\t"                                                             //385/ 
	"scasb\n\t"                                                             //386/ 
	"je 1f\n\t"                                                             //387/ 
	"movl $1,%0\n"                                                          //388/ 
	"1:\tdecl %0"                                                           //389/ 
	:"=D" (__res):"a" (c),"D" (cs),"c" (count)                              //390/ 
	:"cx");                                                                 //391/ 
return __res;                                                                   //392/ 
}                                                                               //393/ 
                                                                                //394/ 
extern inline void * memset(void * s,char c,int count)                          //395/ 用字符c填写s指向的内存区域，共填写count个字节(此时可能ds、es都指向了同一个段)
{                                                                               //396/ 
__asm__("cld\n\t"                                                               //397/ 
	"rep\n\t"                                                               //398/ 
	"stosb"                                                                 //399/ 
	::"a" (c),"D" (s),"c" (count)                                           //400/ 
	:"cx","di");                                                            //401/ 
return s;                                                                       //402/ 
}                                                                               //403/ 
                                                                                //404/ 
#endif                                                                          //405/ 

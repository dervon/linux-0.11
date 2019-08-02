/*                                                                   // 1/ 
 * 'kernel.h' contains some often-used function prototypes etc       // 2/ 
 */                                                                  // 3/ 
void verify_area(void * addr,int count);                             // 4/ 
volatile void panic(const char * str);                               // 5/ 
int printf(const char * fmt, ...);                                   // 6/ 
int printk(const char * fmt, ...);                                   // 7/ 
int tty_write(unsigned ch,char * buf,int count);                     // 8/ 
void * malloc(unsigned int size);                                    // 9/ 
void free_s(void * obj, int size);                                   //10/ 
                                                                     //11/ 
#define free(x) free_s((x), 0)                                       //12/ 
                                                                     //13/ 
/*                                                                   //14/ 
 * This is defined as a macro, but at some point this might become a //15/ 
 * real subroutine that sets a flag if it returns true (to do        //16/ 
 * BSD-style accounting where the process is flagged if it uses root //17/ 
 * privs).  The implication of this is that you should do normal     //18/ 
 * permissions checks first, and check suser() last.                 //19/ 
 */                                                                  //20/ 
#define suser() (current->euid == 0)                                 //21/ 判断当前进程是否是超级用户进程
                                                                     //22/ 

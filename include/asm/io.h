#define outb(value,port) \                                 // 1/ 将value值放到al寄存器中写入port指定的端口
__asm__ ("outb %%al,%%dx"::"a" (value),"d" (port))         // 2/ 
                                                           // 3/ 
                                                           // 4/ 
#define inb(port) ({ \                                     // 5/ 将从port指定的端口读到al寄存器的值放入_v中，并返回_v
unsigned char _v; \                                        // 6/ 
__asm__ volatile ("inb %%dx,%%al":"=a" (_v):"d" (port)); \ // 7/ 
_v; \                                                      // 8/ 
})                                                         // 9/ 
                                                           //10/ 
#define outb_p(value,port) \                               //11/ 将value值放到al寄存器中写入port指定的端口，并等待一会
__asm__ ("outb %%al,%%dx\n" \                              //12/ 
		"\tjmp 1f\n" \                             //13/ 
		"1:\tjmp 1f\n" \                           //14/ 
		"1:"::"a" (value),"d" (port))              //15/ 
                                                           //16/ 
#define inb_p(port) ({ \                                   //17/ 将从port指定的端口读到al寄存器的值放入_v中，等待一会，并返回_v
unsigned char _v; \                                        //18/ 
__asm__ volatile ("inb %%dx,%%al\n" \                      //19/ 
	"\tjmp 1f\n" \                                     //20/ 
	"1:\tjmp 1f\n" \                                   //21/ 
	"1:":"=a" (_v):"d" (port)); \                      //22/ 
_v; \                                                      //23/ 
})                                                         //24/ 

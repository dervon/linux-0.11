#define move_to_user_mode() \                                             // 1/ 
__asm__ ("movl %%esp,%%eax\n\t" \                                         // 2/ 
	"pushl $0x17\n\t" \                                               // 3/ 
	"pushl %%eax\n\t" \                                               // 4/ 
	"pushfl\n\t" \                                                    // 5/ 
	"pushl $0x0f\n\t" \                                               // 6/ 
	"pushl $1f\n\t" \                                                 // 7/ 
	"iret\n" \                                                        // 8/ 
	"1:\tmovl $0x17,%%eax\n\t" \                                      // 9/ 
	"movw %%ax,%%ds\n\t" \                                            //10/ 
	"movw %%ax,%%es\n\t" \                                            //11/ 
	"movw %%ax,%%fs\n\t" \                                            //12/ 
	"movw %%ax,%%gs" \                                                //13/ 
	:::"ax")                                                          //14/ 
                                                                          //15/ 
#define sti() __asm__ ("sti"::)                                           //16/ 开中断，将处理器标志寄存器的中断标志位置1
#define cli() __asm__ ("cli"::)                                           //17/ 关中断，将处理器标志寄存器的中断标志位清0
#define nop() __asm__ ("nop"::)                                           //18/ 空指令，用于等待延时
                                                                          //19/ 
#define iret() __asm__ ("iret"::)                                         //20/ 
                                                                          //21/ 
#define _set_gate(gate_addr,type,dpl,addr) \                              //22/ 
__asm__ ("movw %%dx,%%ax\n\t" \                                           //23/ 
	"movw %0,%%dx\n\t" \                                              //24/ 
	"movl %%eax,%1\n\t" \                                             //25/ 
	"movl %%edx,%2" \                                                 //26/ 
	: \                                                               //27/ 
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \                   //28/ 
	"o" (*((char *) (gate_addr))), \                                  //29/ 
	"o" (*(4+(char *) (gate_addr))), \                                //30/ 
	"d" ((char *) (addr)),"a" (0x00080000))                           //31/ 
                                                                          //32/ 
#define set_intr_gate(n,addr) \                                           //33/ [b;]在idt表中的第n(从0算起)个描述符的位置放置一个中断门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为addr;P=1;DPL=0;TYPE=14->1110)
	_set_gate(&idt[n],14,0,addr)                                      //34/ 
                                                                          //35/ 
#define set_trap_gate(n,addr) \                                           //36/ [b;]在idt表中的第n(从0算起)个描述符的位置放置一个陷阱门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为addr;P=1;DPL=0;TYPE=15->1111)
	_set_gate(&idt[n],15,0,addr)                                      //37/ 
                                                                          //38/ 
#define set_system_gate(n,addr) \                                         //39/ [b;]在idt表中的第n(从0算起)个描述符的位置放置一个陷阱门描述符(目标代码段选择子0x0008;中断处理程序的段内偏移为addr;P=1;DPL=3;TYPE=15->1111)
	_set_gate(&idt[n],15,3,addr)                                      //40/ 
                                                                          //41/ 
#define _set_seg_desc(gate_addr,type,dpl,base,limit) {\                   //42/ 
	*(gate_addr) = ((base) & 0xff000000) | \                          //43/ 
		(((base) & 0x00ff0000)>>16) | \                           //44/ 
		((limit) & 0xf0000) | \                                   //45/ 
		((dpl)<<13) | \                                           //46/ 
		(0x00408000) | \                                          //47/ 
		((type)<<8); \                                            //48/ 
	*((gate_addr)+1) = (((base) & 0x0000ffff)<<16) | \                //49/ 
		((limit) & 0x0ffff); }                                    //50/ 
                                                                          //51/ 
#define _set_tssldt_desc(n,addr,type) \                                   //52/ 被下面的两个宏set_tss_desc和set_ldt_desc调用
__asm__ ("movw $104,%1\n\t" \                                             //53/ 
	"movw %%ax,%2\n\t" \                                              //54/ 
	"rorl $16,%%eax\n\t" \                                            //55/ 
	"movb %%al,%3\n\t" \                                              //56/ 
	"movb $" type ",%4\n\t" \                                         //57/ 
	"movb $0x00,%5\n\t" \                                             //58/ 
	"movb %%ah,%6\n\t" \                                              //59/ 
	"rorl $16,%%eax" \                                                //60/ 
	::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \           //61/ 
	 "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \                       //62/ 
	)                                                                 //63/ 
                                                                          //64/ 
#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x89") //65/ 将参数addr给定的某个任务的TSS段基地址和固定的值(段界限=104、G=0、P=1、DPL=0、B(忙位)=0)写入参数n指定的地址处的8个字节
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x82") //66/ 将参数addr给定的某个任务的LDT段基地址和固定的值(段界限=104、G=0、P=1、DPL=0)写入参数n指定的地址处的8个字节

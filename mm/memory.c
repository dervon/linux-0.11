/*                                                                            //  1/ 
 *  linux/mm/memory.c                                                         //  2/ 
 *                                                                            //  3/ 
 *  (C) 1991  Linus Torvalds                                                  //  4/ 
 */                                                                           //  5/ 
                                                                              //  6/ 
/*                                                                            //  7/ 
 * demand-loading started 01.12.91 - seems it is high on the list of          //  8/ 
 * things wanted, and it should be easy to implement. - Linus                 //  9/ 
 */                                                                           // 10/ 
                                                                              // 11/ 
/*                                                                            // 12/ 
 * Ok, demand-loading was easy, shared pages a little bit tricker. Shared     // 13/ 
 * pages started 02.12.91, seems to work. - Linus.                            // 14/ 
 *                                                                            // 15/ 
 * Tested sharing by executing about 30 /bin/sh: under the old kernel it      // 16/ 
 * would have taken more than the 6M I have free, but it worked well as       // 17/ 
 * far as I could see.                                                        // 18/ 
 *                                                                            // 19/ 
 * Also corrected some "invalidate()"s - I wasn't doing enough of them.       // 20/ 
 */                                                                           // 21/ 
                                                                              // 22/ 
#include <signal.h>                                                           // 23/ 信号头文件。定义信号符号常量，信号结构以及信号操作函数原型
                                                                              // 24/ 
#include <asm/system.h>                                                       // 25/ 系统头文件。定义了设置或修改描述符/中断门等的嵌入式汇编宏
                                                                              // 26/ 
#include <linux/sched.h>                                                      // 27/ 
#include <linux/head.h>                                                       // 28/ 
#include <linux/kernel.h>                                                     // 29/ 
                                                                              // 30/ 
volatile void do_exit(long code);                                             // 31/ 
                                                                              // 32/ 
static inline volatile void oom(void)                                         // 33/ [b;]显示内存已用完的出错信息，并退出当前进程
{                                                                             // 34/ 
	printk("out of memory\n\r");                                          // 35/ 
	do_exit(SIGSEGV);                                                     // 36/ 
}                                                                             // 37/ 
                                                                              // 38/ 
#define invalidate() \                                                        // 39/ [b;]将页目录表的基地址0重新加载进cr3，以刷新页高速缓存TLB
__asm__("movl %%eax,%%cr3"::"a" (0))                                          // 40/ 
                                                                              // 41/ 
/* these are not to be changed without changing head.s etc */                 // 42/ 
#define LOW_MEM 0x100000                                                      // 43/ 物理内存低端(1MB)
#define PAGING_MEMORY (15*1024*1024)                                          // 44/ 可映射页面的内存大小(从1MB到16MB，共15MB)
#define PAGING_PAGES (PAGING_MEMORY>>12)                                      // 45/ 可映射页面(从1MB到16MB)的总页数
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)                                   // 46/ 将指定的物理地址addr转换为从内存低端开始对应的页号
#define USED 100                                                              // 47/ 页面被占用标志
                                                                              // 48/ 
#define CODE_SPACE(addr) ((((addr)+4095)&~4095) < \                           // 49/ 
current->start_code + current->end_code)                                      // 50/ 
                                                                              // 51/ 
static long HIGH_MEMORY = 0;                                                  // 52/ 用于存放系统所含物理内存最高端
                                                                              // 53/ 
#define copy_page(from,to) \                                                  // 54/ [b;]从物理地址from处开始复制一页内存到物理地址to处
__asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024):"cx","di","si")   // 55/ 
                                                                              // 56/ 
static unsigned char mem_map [ PAGING_PAGES ] = {0,};                         // 57/ 为可映射页面(从1MB到16MB)建立一个字节数组，每个字节表示一个页面的占用次数
                                                                              // 58/ 
/*                                                                            // 59/ 
 * Get physical address of first (actually last :-) free page, and mark it    // 60/ 
 * used. If no free pages left, return 0.                                     // 61/ 
 */                                                                           // 62/ 
unsigned long get_free_page(void)                                             // 63/ [b;]在主内存中(1MB到16MB中除高速缓冲区和虚拟盘外的部分)获取一个空闲物理页面，返回该页面起始物理地址，若无空闲页则返回0
{                                                                             // 64/ 
register unsigned long __res asm("ax");                                       // 65/ 
                                                                              // 66/ 
__asm__("std ; repne ; scasb\n\t"                                             // 67/ 置方向位；将al(值为0)与es:edi处的字节进行循环比较，
	"jne 1f\n\t"                                                          // 68/ 如果内存位图中没有空闲页(即mem_map数组的元素都不为0)，直接返回0
	"movb $1,1(%%edi)\n\t"                                                // 69/ 如果找到了空闲页，则将其对应内存位图mem_map中的字节置1
	"sall $12,%%ecx\n\t"                                                  // 70/ 取得该空闲页相对低端内存地址的起始地址
	"addl %2,%%ecx\n\t"                                                   // 71/ 取得该空闲页相对物理地址0的实际起始地址
	"movl %%ecx,%%edx\n\t"                                                // 72/ 将该空闲页相对物理地址0的实际起始地址放入edx
	"movl $1024,%%ecx\n\t"                                                // 73/ 73-75行用于将空闲页的4KB内容都清零
	"leal 4092(%%edx),%%edi\n\t"                                          // 74/ 
	"rep ; stosl\n\t"                                                     // 75/ 
	"movl %%edx,%%eax\n"                                                  // 76/ 最后将该空闲页相对物理地址0的实际起始地址放入eax中返回
	"1:"                                                                  // 77/ 
	:"=a" (__res)                                                         // 78/ 
	:"0" (0),"i" (LOW_MEM),"c" (PAGING_PAGES),                            // 79/ 
	"D" (mem_map+PAGING_PAGES-1)                                          // 80/ 
	:"di","cx","dx");                                                     // 81/ 告诉编译器di、cx、dx寄存器的内容被嵌入汇编程序改变了
return __res;                                                                 // 82/ 
}                                                                             // 83/ 
                                                                              // 84/ 
/*                                                                            // 85/ 
 * Free a page of memory at physical address 'addr'. Used by                  // 86/ 
 * 'free_page_tables()'                                                       // 87/ 
 */                                                                           // 88/ 
void free_page(unsigned long addr)                                            // 89/ [b;]释放物理地址addr指向的位置所在处的1页面内存
{                                                                             // 90/ 
	if (addr < LOW_MEM) return;                                           // 91/ 如果add小于内存低端(1MB)，表示页面在内核程序或高速缓存中，直接返回
	if (addr >= HIGH_MEMORY)                                              // 92/ 如果add超过了系统所含物理内存最高端，直接死机
		panic("trying to free nonexistent page");                     // 93/ 
	addr -= LOW_MEM;                                                      // 94/ 94-95行用于将指定的物理地址addr转换为从内存低端开始对应的页号
	addr >>= 12;                                                          // 95/ 
	if (mem_map[addr]--) return;                                          // 96/ 如果该页号对应的页的占用次数不为0，说明有其他进程在占用，直接减一返回
	mem_map[addr]=0;                                                      // 97/ 否则原来就为0说明本身就是空闲页，释放空闲页，则直接死机
	panic("trying to free free page");                                    // 98/ 
}                                                                             // 99/ 
                                                                              //100/ 
/*                                                                            //101/ 
 * This function frees a continuos block of page tables, as needed            //102/ 
 * by 'exit()'. As does copy_page_tables(), this handles only 4Mb blocks.     //103/ 
 */                                                                           //104/ 
int free_page_tables(unsigned long from,unsigned long size)                   //105/ [b;]将从线性地址from(要求4MB对齐)到线性地址from+size(字节长度)映射时用到的页目录项清空，并将这些页目录项指向的页表中所有页表项清空，并将这些页表项映射的物理页面全部释放
{                                                                             //106/ 
	unsigned long *pg_table;                                              //107/ 
	unsigned long * dir, nr;                                              //108/ 
                                                                              //109/ 
	if (from & 0x3fffff)                                                  //110/ 先保证线性基地址from是4MB对齐的，否则直接死机
		panic("free_page_tables called with wrong alignment");        //111/ 
	if (!from)                                                            //112/ 若from==0，表明要释放的是内核和高速缓冲所在空间，所以也直接死机
		panic("Trying to free up swapper memory space");              //113/ 
	size = (size + 0x3fffff) >> 22;                                       //114/ 计算出长度size所占的页目录项数赋给size(即用多少个4MB可以装下size个字节)
	dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */       //115/ size所占的那些页目录项中的起始页目录项的物理地址
	for ( ; size-->0 ; dir++) {                                           //116/ 遍寻size所占的那些页目录项
		if (!(1 & *dir))                                              //117/ 如果该页目录项的P==0，说明未为该目录项分配页表，则直接跳过
			continue;                                             //118/ 
		pg_table = (unsigned long *) (0xfffff000 & *dir);             //119/ 否则说明已为该目录项分配了页表，取出页表的物理地址
		for (nr=0 ; nr<1024 ; nr++) {                                 //120/ 遍寻页表中的1024项
			if (1 & *pg_table)                                    //121/ 如果页表项中的P==1，说明为该页表项分配了物理页，则释放该页
				free_page(0xfffff000 & *pg_table);            //122/ 		
			*pg_table = 0;                                        //123/ 将该页表项的内容清空
			pg_table++;                                           //124/ 继续处理下一个页表项
		}                                                             //125/ 	
		free_page(0xfffff000 & *dir);                                 //126/ 释放该页表所占的物理页面
		*dir = 0;                                                     //127/ 
	}                                                                     //128/ 
	invalidate();                                                         //129/ 将页目录表的基地址0重新加载进cr3，以刷新页高速缓存TLB
	return 0;                                                             //130/ 
}                                                                             //131/ 
                                                                              //132/ 
/*                                                                            //133/ 
 *  Well, here is one of the most complicated functions in mm. It             //134/ 
 * copies a range of linerar addresses by copying only the pages.             //135/ 
 * Let's hope this is bug-free, 'cause this one I don't want to debug :-)     //136/ 
 *                                                                            //137/ 
 * Note! We don't copy just any chunks of memory - addresses have to          //138/ 
 * be divisible by 4Mb (one page-directory entry), as this makes the          //139/ 
 * function easier. It's used only by fork anyway.                            //140/ 
 *                                                                            //141/ 注:size = ((unsigned) (size(由参数三指定)+0x3fffff)) >> 22; 
 * NOTE 2!! When from==0 we are copying kernel space for the first            //142/ 	size个源页目录项——从from_dir指向的页目录项开始的size个页目录项
 * fork(). Then we DONT want to copy a full page-directory entry, as          //143/ 	size个目的页目录项——to_dir指向的页目录项开始的size个页目录项
 * that would lead to some serious memory waste - we just copy the            //144/    源页表项——源页目录项指向的源页表中的页表项
 * first 160 pages - 640kB. Even that is more than we need, but it            //145/ 	目的页表项——目的页目录项指向的目的页表中的页表项
 * doesn't take any more memory - we don't copy-on-write in the low           //146/ 	源页——源页表项指向的页(函数执行后，如果源页的起始物理地址在1MB以下，则保持源进程对源页的读写属性;如果源页的起始物理地址在1MB以上，则源进程对源页只读([r;]感觉应该包含等于1MB的情况，可能是因为该函数用于创建进程时不会出现临界时的情况，所以不影响);)
 * 1 Mb-range, so the pages can be shared with the kernel. Thus the           //147/ 	目的页——目的页表项指向的页(函数执行后，目的进程对目的页都为只读)
 * special case for nr=xxxx.                                                  //148/ 	源页和目的页是同一个物理页面，只是使用该页的进程不同，一个源进程，一个目的进程
 */                                                                           //149/ 	
int copy_page_tables(unsigned long from,unsigned long to,long size)           //150/ [b;]为线性地址to到to+size在页目录表中建立页目录项，并为新建的每个页目录项指向的页表申请一个物理页面(作为页表使用)，再‘新建’所有新建页表中的所有页表项，使得线性地址from到from+size与线性地址to到to+size能映射到相同的物理地址(如果from==0，表示是在内核空间，针对该页表项指向的页表则只需复制160(0xA0)个页表项，可能是任务0只覆盖开始的640KB空间)
{                                                                             //151/ 
	unsigned long * from_page_table;                                      //152/ 
	unsigned long * to_page_table;                                        //153/ 
	unsigned long this_page;                                              //154/ 
	unsigned long * from_dir, * to_dir;                                   //155/ 
	unsigned long nr;                                                     //156/ 
                                                                              //157/ 
	if ((from&0x3fffff) || (to&0x3fffff))                                 //158/ 因为一个页表的1024项可管理4MB内存，所以要保证源地址from和目的地址to都是
		panic("copy_page_tables called with wrong alignment");        //159/ 4MB对齐的，否则直接死机
	from_dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */  //160/ 取源地址from的页目录项指针赋给from_dir
	to_dir = (unsigned long *) ((to>>20) & 0xffc);                        //161/ 取目的地址to的页目录项指针赋给to_dir
	size = ((unsigned) (size+0x3fffff)) >> 22;                            //162/ 计算size字节长度需要占用的页表数(即页目录项数)赋给size
	for( ; size-->0 ; from_dir++,to_dir++) {                              //163/ 163-186行先为size个目的页目录项指向的每个页表申请一个页面，并将新申请页面的起始物理地址加上一些属性值(US=1;RW=1;P=1;其他属性全0)填入size个目的页目录项中，然后将size个源页目录项指向的size个源页表中的所有源页表项复制到size个目的页目录项指向的size个目的页表中(如果from==0，表示是在内核空间，针对该页表项指向的页表则只需复制160(0xA0)个页表项，可能是任务0只覆盖开始的640KB空间)
		if (1 & *to_dir)                                              //164/ 如果目的页目录项的P==1(即已映射了页表)，则直接死机
			panic("copy_page_tables: already exist");             //165/ 
		if (!(1 & *from_dir))                                         //166/ 如果源页目录项无效，则跳过
			continue;                                             //167/ 
		from_page_table = (unsigned long *) (0xfffff000 & *from_dir); //168/ 取源页目录项中的页表起始物理地址字段赋给from_page_table
		if (!(to_page_table = (unsigned long *) get_free_page()))     //169/ 在主内存中(1MB到16MB中除高速缓冲区和虚拟盘外的部分)获取一个空闲物理页面给页表用，将该页表起始物理地址
			return -1;	/* Out of memory, see freeing */      //170/ 赋给to_page_table，若无空闲页则返回
		*to_dir = ((unsigned long) to_page_table) | 7;                //171/ 将新申请页面的起始物理地址加上属性值(US=1;RW=1;P=1;其他属性全0)填入目的页目录项
		nr = (from==0)?0xA0:1024;                                     //172/ 针对每个源页目录项确定其指向的源页表中需要被复制的源页表项数，赋给nr(如果from==0，表示是在内核空间，则只需复制160(0xA0)个页表项，可能是任务0只覆盖开始的640KB空间)
		for ( ; nr-- > 0 ; from_page_table++,to_page_table++) {       //173/ 173-185行将指定的源页目录项指向的源页表中的nr个源页表项复制到对应的目的页目录项指向的目的页表中
			this_page = *from_page_table;                         //174/ 将指定的源页目录项指向的源页表的源页表项取出来赋给this_page
			if (!(1 & this_page))                                 //175/ 如果该源页表项的P==0，说明系统没有为该源页表项映射一页内存，所以跳过
				continue;                                     //176/ 
			this_page &= ~2;                                      //177/ 将该源页表项的RW置0,即保证映射的一页内存只读，因为要共享页面实现写时复制
			*to_page_table = this_page;                           //178/ 将修改过的源页表项复制到对应的目的页表项(RW=0,目的页只读)中
			if (this_page > LOW_MEM) {                            //179/ 如果该源页表项映射的源页的起始物理地址在1MB以上，则让源进程对源页也只读，并将该页对应的引用计数加一。[r;]感觉应该包含相等的情况，可能是因为该函数用于创建进程时不会出现临界时的情况，所以不影响
				*from_page_table = this_page;                 //180/ 
				this_page -= LOW_MEM;                         //181/ 
				this_page >>= 12;                             //182/ 
				mem_map[this_page]++;                         //183/ 
			}                                                     //184/ 如果该源页表项映射的源页的起始物理地址在1MB以下，则保持源进程对源页的读写属性
		}                                                             //185/ 
	}                                                                     //186/ 
	invalidate();                                                         //187/ 将页目录表的基地址0重新加载进cr3，以刷新页高速缓存TLB
	return 0;                                                             //188/ 
}                                                                             //189/ 
                                                                              //190/ 
/*                                                                            //191/ 
 * This function puts a page in memory at the wanted address.                 //192/ 
 * It returns the physical address of the page gotten, 0 if                   //193/ 
 * out of memory (either when trying to access page-table or                  //194/ 
 * page.)                                                                     //195/ 
 */                                                                           //196/ 
unsigned long put_page(unsigned long page,unsigned long address)              //197/ [b;]
{                                                                             //198/ 
	unsigned long tmp, *page_table;                                       //199/ 
                                                                              //200/ 
/* NOTE !!! This uses the fact that _pg_dir=0 */                              //201/ 
                                                                              //202/ 
	if (page < LOW_MEM || page >= HIGH_MEMORY)                            //203/ 
		printk("Trying to put page %p at %p\n",page,address);         //204/ 
	if (mem_map[(page-LOW_MEM)>>12] != 1)                                 //205/ 
		printk("mem_map disagrees with %p at %p\n",page,address);     //206/ 
	page_table = (unsigned long *) ((address>>20) & 0xffc);               //207/ 
	if ((*page_table)&1)                                                  //208/ 
		page_table = (unsigned long *) (0xfffff000 & *page_table);    //209/ 
	else {                                                                //210/ 
		if (!(tmp=get_free_page()))                                   //211/ 
			return 0;                                             //212/ 
		*page_table = tmp|7;                                          //213/ 
		page_table = (unsigned long *) tmp;                           //214/ 
	}                                                                     //215/ 
	page_table[(address>>12) & 0x3ff] = page | 7;                         //216/ 
/* no need for invalidate */                                                  //217/ 
	return page;                                                          //218/ 
}                                                                             //219/ 
                                                                              //220/ 
void un_wp_page(unsigned long * table_entry)                                  //221/ [b;]判断参数table_entry指向的页表项对应的物理页，若在主内存区且独享(引用计数为1)，则置可读写属性，否则就申请一个新物理页(US=1;RW=1;P=1),并将源页的内容全复制到新页，将参数table_entry指向页表项定位到新的空闲页的地址
{                                                                             //222/ 
	unsigned long old_page,new_page;                                      //223/ 
                                                                              //224/ 
	old_page = 0xfffff000 & *table_entry;                                 //225/ 取得参数指向的页表项中的物理页面的地址
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)]==1) {            //226/ 判断该物理页是否在可映射内存区，并且被引用计数为1(即未被共享)
		*table_entry |= 2;                                            //227/ 更改参数指定的页表项，使该物理页可读写
		invalidate();                                                 //228/ 将页目录表的基地址0重新加载进cr3，以刷新页高速缓存TLB
		return;                                                       //229/ 
	}                                                                     //230/ 
	if (!(new_page=get_free_page()))                                      //231/ 如果不满足上面的条件就在主内存区申请一个新的空闲页
		oom();                                                        //232/ 如果没有申请到，就显示内存已用完的出错信息，并退出当前进程
	if (old_page >= LOW_MEM)                                              //233/ 如果源物理页是在可映射内存区，则将引用计数减一
		mem_map[MAP_NR(old_page)]--;                                  //234/ 
	*table_entry = new_page | 7;                                          //235/ 将参数table_entry指向页表项定位到新的空闲页的地址，并使新页US=1;RW=1;P=1
	invalidate();                                                         //236/ 将页目录表的基地址0重新加载进cr3，以刷新页高速缓存TLB
	copy_page(old_page,new_page);                                         //237/ 将源页的物理内存内容(4KB)复制到新的空闲页中
}	                                                                      //238/ 
                                                                              //239/ 
/*                                                                            //240/ 
 * This routine handles present pages, when users try to write                //241/ 
 * to a shared page. It is done by copying the page to a new address          //242/ 
 * and decrementing the shared-page counter for the old page.                 //243/ 
 *                                                                            //244/ 
 * If it's in code space we exit with a segment error.                        //245/ 
 */                                                                           //246/ 
void do_wp_page(unsigned long error_code,unsigned long address)               //247/ [b;]
{                                                                             //248/ 
#if 0                                                                         //249/ 
/* we cannot do this yet: the estdio library writes to code space */          //250/ 
/* stupid, stupid. I really want the libc.a from GNU */                       //251/ 
	if (CODE_SPACE(address))                                              //252/ 
		do_exit(SIGSEGV);                                             //253/ 
#endif                                                                        //254/ 
	un_wp_page((unsigned long *)                                          //255/ 
		(((address>>10) & 0xffc) + (0xfffff000 &                      //256/ 
		*((unsigned long *) ((address>>20) &0xffc)))));               //257/ 
                                                                              //258/ 
}                                                                             //259/ 
                                                                              //260/ 
void write_verify(unsigned long address)                                      //261/ [b;]检测包含线性地址address的页面是否可写，若不可写就执行共享检验和复制页面操作
{                                                                             //262/ 
	unsigned long page;                                                   //263/ 
                                                                              //264/ 
	if (!( (page = *((unsigned long *) ((address>>20) & 0xffc)) )&1))     //265/ 取得页面对应的页目录项，判断其P位是否为1
		return;                                                       //266/ 
	page &= 0xfffff000;                                                   //267/ 
	page += ((address>>10) & 0xffc);                                      //268/ 取得页面对应的页表项的地址
	if ((3 & *(unsigned long *) page) == 1)  /* non-writeable, present */ //269/ 判断页面对应的页表项是否是P=1，R/W=0(只读)
		un_wp_page((unsigned long *) page);                           //270/ 如果页面存在且只读——则判断page指向的页表项对应的物理页，若在主内存区且独享(引用计数为1)，则置可读写属性，否则就申请一个新物理页(US=1;RW=1;P=1),并将源页的内容全复制到新页，将参数table_entry指向页表项定位到新的空闲页的地址
	return;                                                               //271/ 
}                                                                             //272/ 
                                                                              //273/ 
void get_empty_page(unsigned long address)                                    //274/ [b;]
{                                                                             //275/ 
	unsigned long tmp;                                                    //276/ 
                                                                              //277/ 
	if (!(tmp=get_free_page()) || !put_page(tmp,address)) {               //278/ 
		free_page(tmp);		/* 0 is ok - ignored */               //279/ 
		oom();                                                        //280/ 
	}                                                                     //281/ 
}                                                                             //282/ 
                                                                              //283/ 
/*                                                                            //284/ 
 * try_to_share() checks the page at address "address" in the task "p",       //285/ 
 * to see if it exists, and if it is clean. If so, share it with the current  //286/ 
 * task.                                                                      //287/ 
 *                                                                            //288/ 
 * NOTE! This assumes we have checked that p != current, and that they        //289/ 
 * share the same executable.                                                 //290/ 
 */                                                                           //291/ 
static int try_to_share(unsigned long address, struct task_struct * p)        //292/ [b;]
{                                                                             //293/ 
	unsigned long from;                                                   //294/ 
	unsigned long to;                                                     //295/ 
	unsigned long from_page;                                              //296/ 
	unsigned long to_page;                                                //297/ 
	unsigned long phys_addr;                                              //298/ 
                                                                              //299/ 
	from_page = to_page = ((address>>20) & 0xffc);                        //300/ 
	from_page += ((p->start_code>>20) & 0xffc);                           //301/ 
	to_page += ((current->start_code>>20) & 0xffc);                       //302/ 
/* is there a page-directory at from? */                                      //303/ 
	from = *(unsigned long *) from_page;                                  //304/ 
	if (!(from & 1))                                                      //305/ 
		return 0;                                                     //306/ 
	from &= 0xfffff000;                                                   //307/ 
	from_page = from + ((address>>10) & 0xffc);                           //308/ 
	phys_addr = *(unsigned long *) from_page;                             //309/ 
/* is the page clean and present? */                                          //310/ 
	if ((phys_addr & 0x41) != 0x01)                                       //311/ 
		return 0;                                                     //312/ 
	phys_addr &= 0xfffff000;                                              //313/ 
	if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)                  //314/ 
		return 0;                                                     //315/ 
	to = *(unsigned long *) to_page;                                      //316/ 
	if (!(to & 1))                                                        //317/ 
		if (to = get_free_page())                                     //318/ 
			*(unsigned long *) to_page = to | 7;                  //319/ 
		else                                                          //320/ 
			oom();                                                //321/ 
	to &= 0xfffff000;                                                     //322/ 
	to_page = to + ((address>>10) & 0xffc);                               //323/ 
	if (1 & *(unsigned long *) to_page)                                   //324/ 
		panic("try_to_share: to_page already exists");                //325/ 
/* share them: write-protect */                                               //326/ 
	*(unsigned long *) from_page &= ~2;                                   //327/ 
	*(unsigned long *) to_page = *(unsigned long *) from_page;            //328/ 
	invalidate();                                                         //329/ 
	phys_addr -= LOW_MEM;                                                 //330/ 
	phys_addr >>= 12;                                                     //331/ 
	mem_map[phys_addr]++;                                                 //332/ 
	return 1;                                                             //333/ 
}                                                                             //334/ 
                                                                              //335/ 
/*                                                                            //336/ 
 * share_page() tries to find a process that could share a page with          //337/ 
 * the current one. Address is the address of the wanted page relative        //338/ 
 * to the current data space.                                                 //339/ 
 *                                                                            //340/ 
 * We first check if it is at all feasible by checking executable->i_count.   //341/ 
 * It should be >1 if there are other tasks sharing this inode.               //342/ 
 */                                                                           //343/ 
static int share_page(unsigned long address)                                  //344/ [b;]
{                                                                             //345/ 
	struct task_struct ** p;                                              //346/ 
                                                                              //347/ 
	if (!current->executable)                                             //348/ 
		return 0;                                                     //349/ 
	if (current->executable->i_count < 2)                                 //350/ 
		return 0;                                                     //351/ 
	for (p = &LAST_TASK ; p > &FIRST_TASK ; --p) {                        //352/ 
		if (!*p)                                                      //353/ 
			continue;                                             //354/ 
		if (current == *p)                                            //355/ 
			continue;                                             //356/ 
		if ((*p)->executable != current->executable)                  //357/ 
			continue;                                             //358/ 
		if (try_to_share(address,*p))                                 //359/ 
			return 1;                                             //360/ 
	}                                                                     //361/ 
	return 0;                                                             //362/ 
}                                                                             //363/ 
                                                                              //364/ 
void do_no_page(unsigned long error_code,unsigned long address)               //365/ [b;]
{                                                                             //366/ 
	int nr[4];                                                            //367/ 
	unsigned long tmp;                                                    //368/ 
	unsigned long page;                                                   //369/ 
	int block,i;                                                          //370/ 
                                                                              //371/ 
	address &= 0xfffff000;                                                //372/ 
	tmp = address - current->start_code;                                  //373/ 
	if (!current->executable || tmp >= current->end_data) {               //374/ 
		get_empty_page(address);                                      //375/ 
		return;                                                       //376/ 
	}                                                                     //377/ 
	if (share_page(tmp))                                                  //378/ 
		return;                                                       //379/ 
	if (!(page = get_free_page()))                                        //380/ 
		oom();                                                        //381/ 
/* remember that 1 block is used for header */                                //382/ 
	block = 1 + tmp/BLOCK_SIZE;                                           //383/ 
	for (i=0 ; i<4 ; block++,i++)                                         //384/ 
		nr[i] = bmap(current->executable,block);                      //385/ 
	bread_page(page,current->executable->i_dev,nr);                       //386/ 
	i = tmp + 4096 - current->end_data;                                   //387/ 
	tmp = page + 4096;                                                    //388/ 
	while (i-- > 0) {                                                     //389/ 
		tmp--;                                                        //390/ 
		*(char *)tmp = 0;                                             //391/ 
	}                                                                     //392/ 
	if (put_page(page,address))                                           //393/ 
		return;                                                       //394/ 
	free_page(page);                                                      //395/ 
	oom();                                                                //396/ 
}                                                                             //397/ 
                                                                              //398/ 
void mem_init(long start_mem, long end_mem)                                   //399/ [b;]
{                                                                             //400/ 
	int i;                                                                //401/ 
                                                                              //402/ 
	HIGH_MEMORY = end_mem;                                                //403/ 
	for (i=0 ; i<PAGING_PAGES ; i++)                                      //404/ 
		mem_map[i] = USED;                                            //405/ 
	i = MAP_NR(start_mem);                                                //406/ 
	end_mem -= start_mem;                                                 //407/ 
	end_mem >>= 12;                                                       //408/ 
	while (end_mem-->0)                                                   //409/ 
		mem_map[i++]=0;                                               //410/ 
}                                                                             //411/ 
                                                                              //412/ 
void calc_mem(void)                                                           //413/ [b;]
{                                                                             //414/ 
	int i,j,k,free=0;                                                     //415/ 
	long * pg_tbl;                                                        //416/ 
                                                                              //417/ 
	for(i=0 ; i<PAGING_PAGES ; i++)                                       //418/ 
		if (!mem_map[i]) free++;                                      //419/ 
	printk("%d pages free (of %d)\n\r",free,PAGING_PAGES);                //420/ 
	for(i=2 ; i<1024 ; i++) {                                             //421/ 
		if (1&pg_dir[i]) {                                            //422/ 
			pg_tbl=(long *) (0xfffff000 & pg_dir[i]);             //423/ 
			for(j=k=0 ; j<1024 ; j++)                             //424/ 
				if (pg_tbl[j]&1)                              //425/ 
					k++;                                  //426/ 
			printk("Pg-dir[%d] uses %d pages\n",i,k);             //427/ 
		}                                                             //428/ 
	}                                                                     //429/ 
}                                                                             //430/ 

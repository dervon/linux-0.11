#ifndef _HEAD_H                    // 1/ 
#define _HEAD_H                    // 2/ 
                                   // 3/ 
typedef struct desc_struct {       // 4/ 
	unsigned long a,b;         // 5/ 
} desc_table[256];                 // 6/ 
                                   // 7/ 
extern unsigned long pg_dir[1024]; // 8/ 
extern desc_table idt,gdt;         // 9/ idt,gdt引用的是head.s中第232行上的IDT基地址(_idt)和第234行上的GDT基地址(_gdt)
                                   //10/ 
#define GDT_NUL 0                  //11/ 
#define GDT_CODE 1                 //12/ 
#define GDT_DATA 2                 //13/ 
#define GDT_TMP 3                  //14/ 
                                   //15/ 
#define LDT_NUL 0                  //16/ 
#define LDT_CODE 1                 //17/ 
#define LDT_DATA 2                 //18/ 
                                   //19/ 
#endif                             //20/ 

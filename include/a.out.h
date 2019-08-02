#ifndef _A_OUT_H                                                                           //  1/ 
#define _A_OUT_H                                                                           //  2/ 
                                                                                           //  3/ 
#define __GNU_EXEC_MACROS__                                                                //  4/ 
                                                                                           //  5/ 
struct exec {                                                                              //  6/ 
  unsigned long a_magic;	/* Use macros N_MAGIC, etc for access */                   //  7/ 
  unsigned a_text;		/* length of text, in bytes */                             //  8/ 
  unsigned a_data;		/* length of data, in bytes */                             //  9/ 
  unsigned a_bss;		/* length of uninitialized data area for file, in bytes */ // 10/ 
  unsigned a_syms;		/* length of symbol table data in file, in bytes */        // 11/ 
  unsigned a_entry;		/* start address */                                        // 12/ 
  unsigned a_trsize;		/* length of relocation info for text, in bytes */         // 13/ 
  unsigned a_drsize;		/* length of relocation info for data, in bytes */         // 14/ 
};                                                                                         // 15/ 
                                                                                           // 16/ 
#ifndef N_MAGIC                                                                            // 17/ 
#define N_MAGIC(exec) ((exec).a_magic)                                                     // 18/ 
#endif                                                                                     // 19/ 
                                                                                           // 20/ 
#ifndef OMAGIC                                                                             // 21/ 
/* Code indicating object file or impure executable.  */                                   // 22/ 
#define OMAGIC 0407                                                                        // 23/ 
/* Code indicating pure executable.  */                                                    // 24/ 
#define NMAGIC 0410                                                                        // 25/ 
/* Code indicating demand-paged executable.  */                                            // 26/ 
#define ZMAGIC 0413                                                                        // 27/ 
#endif /* not OMAGIC */                                                                    // 28/ 
                                                                                           // 29/ 
#ifndef N_BADMAG                                                                           // 30/ 
#define N_BADMAG(x)					\                                  // 31/ 
 (N_MAGIC(x) != OMAGIC && N_MAGIC(x) != NMAGIC		\                                  // 32/ 
  && N_MAGIC(x) != ZMAGIC)                                                                 // 33/ 
#endif                                                                                     // 34/ 
                                                                                           // 35/ 
#define _N_BADMAG(x)					\                                  // 36/ 
 (N_MAGIC(x) != OMAGIC && N_MAGIC(x) != NMAGIC		\                                  // 37/ 
  && N_MAGIC(x) != ZMAGIC)                                                                 // 38/ 
                                                                                           // 39/ 
#define _N_HDROFF(x) (SEGMENT_SIZE - sizeof (struct exec))                                 // 40/ 
                                                                                           // 41/ 
#ifndef N_TXTOFF                                                                           // 42/ 
#define N_TXTOFF(x) \                                                                      // 43/ 
 (N_MAGIC(x) == ZMAGIC ? _N_HDROFF((x)) + sizeof (struct exec) : sizeof (struct exec))     // 44/ 
#endif                                                                                     // 45/ 
                                                                                           // 46/ 
#ifndef N_DATOFF                                                                           // 47/ 
#define N_DATOFF(x) (N_TXTOFF(x) + (x).a_text)                                             // 48/ 
#endif                                                                                     // 49/ 
                                                                                           // 50/ 
#ifndef N_TRELOFF                                                                          // 51/ 
#define N_TRELOFF(x) (N_DATOFF(x) + (x).a_data)                                            // 52/ 
#endif                                                                                     // 53/ 
                                                                                           // 54/ 
#ifndef N_DRELOFF                                                                          // 55/ 
#define N_DRELOFF(x) (N_TRELOFF(x) + (x).a_trsize)                                         // 56/ 
#endif                                                                                     // 57/ 
                                                                                           // 58/ 
#ifndef N_SYMOFF                                                                           // 59/ 
#define N_SYMOFF(x) (N_DRELOFF(x) + (x).a_drsize)                                          // 60/ 
#endif                                                                                     // 61/ 
                                                                                           // 62/ 
#ifndef N_STROFF                                                                           // 63/ 
#define N_STROFF(x) (N_SYMOFF(x) + (x).a_syms)                                             // 64/ 
#endif                                                                                     // 65/ 
                                                                                           // 66/ 
/* Address of text segment in memory after it is loaded.  */                               // 67/ 
#ifndef N_TXTADDR                                                                          // 68/ 
#define N_TXTADDR(x) 0                                                                     // 69/ 
#endif                                                                                     // 70/ 
                                                                                           // 71/ 
/* Address of data segment in memory after it is loaded.                                   // 72/ 
   Note that it is up to you to define SEGMENT_SIZE                                        // 73/ 
   on machines not listed here.  */                                                        // 74/ 
#if defined(vax) || defined(hp300) || defined(pyr)                                         // 75/ 
#define SEGMENT_SIZE PAGE_SIZE                                                             // 76/ 
#endif                                                                                     // 77/ 
#ifdef	hp300                                                                              // 78/ 
#define	PAGE_SIZE	4096                                                               // 79/ 
#endif                                                                                     // 80/ 
#ifdef	sony                                                                               // 81/ 
#define	SEGMENT_SIZE	0x2000                                                             // 82/ 
#endif	/* Sony.  */                                                                       // 83/ 
#ifdef is68k                                                                               // 84/ 
#define SEGMENT_SIZE 0x20000                                                               // 85/ 
#endif                                                                                     // 86/ 
#if defined(m68k) && defined(PORTAR)                                                       // 87/ 
#define PAGE_SIZE 0x400                                                                    // 88/ 
#define SEGMENT_SIZE PAGE_SIZE                                                             // 89/ 
#endif                                                                                     // 90/ 
                                                                                           // 91/ 
#define PAGE_SIZE 4096                                                                     // 92/ 80386上此处的PAGE_SIZE有效
#define SEGMENT_SIZE 1024                                                                  // 93/ 
                                                                                           // 94/ 
#define _N_SEGMENT_ROUND(x) (((x) + SEGMENT_SIZE - 1) & ~(SEGMENT_SIZE - 1))               // 95/ 
                                                                                           // 96/ 
#define _N_TXTENDADDR(x) (N_TXTADDR(x)+(x).a_text)                                         // 97/ 
                                                                                           // 98/ 
#ifndef N_DATADDR                                                                          // 99/ 
#define N_DATADDR(x) \                                                                     //100/ 
    (N_MAGIC(x)==OMAGIC? (_N_TXTENDADDR(x)) \                                              //101/ 
     : (_N_SEGMENT_ROUND (_N_TXTENDADDR(x))))                                              //102/ 
#endif                                                                                     //103/ 
                                                                                           //104/ 
/* Address of bss segment in memory after it is loaded.  */                                //105/ 
#ifndef N_BSSADDR                                                                          //106/ 
#define N_BSSADDR(x) (N_DATADDR(x) + (x).a_data)                                           //107/ 
#endif                                                                                     //108/ 
                                                                                           //109/ 
#ifndef N_NLIST_DECLARED                                                                   //110/ 
struct nlist {                                                                             //111/ 
  union {                                                                                  //112/ 
    char *n_name;                                                                          //113/ 
    struct nlist *n_next;                                                                  //114/ 
    long n_strx;                                                                           //115/ 
  } n_un;                                                                                  //116/ 
  unsigned char n_type;                                                                    //117/ 
  char n_other;                                                                            //118/ 
  short n_desc;                                                                            //119/ 
  unsigned long n_value;                                                                   //120/ 
};                                                                                         //121/ 
#endif                                                                                     //122/ 
                                                                                           //123/ 
#ifndef N_UNDF                                                                             //124/ 
#define N_UNDF 0                                                                           //125/ 
#endif                                                                                     //126/ 
#ifndef N_ABS                                                                              //127/ 
#define N_ABS 2                                                                            //128/ 
#endif                                                                                     //129/ 
#ifndef N_TEXT                                                                             //130/ 
#define N_TEXT 4                                                                           //131/ 
#endif                                                                                     //132/ 
#ifndef N_DATA                                                                             //133/ 
#define N_DATA 6                                                                           //134/ 
#endif                                                                                     //135/ 
#ifndef N_BSS                                                                              //136/ 
#define N_BSS 8                                                                            //137/ 
#endif                                                                                     //138/ 
#ifndef N_COMM                                                                             //139/ 
#define N_COMM 18                                                                          //140/ 
#endif                                                                                     //141/ 
#ifndef N_FN                                                                               //142/ 
#define N_FN 15                                                                            //143/ 
#endif                                                                                     //144/ 
                                                                                           //145/ 
#ifndef N_EXT                                                                              //146/ 
#define N_EXT 1                                                                            //147/ 
#endif                                                                                     //148/ 
#ifndef N_TYPE                                                                             //149/ 
#define N_TYPE 036                                                                         //150/ 
#endif                                                                                     //151/ 
#ifndef N_STAB                                                                             //152/ 
#define N_STAB 0340                                                                        //153/ 
#endif                                                                                     //154/ 
                                                                                           //155/ 
/* The following type indicates the definition of a symbol as being                        //156/ 
   an indirect reference to another symbol.  The other symbol                              //157/ 
   appears as an undefined reference, immediately following this symbol.                   //158/ 
                                                                                           //159/ 
   Indirection is asymmetrical.  The other symbol's value will be used                     //160/ 
   to satisfy requests for the indirect symbol, but not vice versa.                        //161/ 
   If the other symbol does not have a definition, libraries will                          //162/ 
   be searched to find a definition.  */                                                   //163/ 
#define N_INDR 0xa                                                                         //164/ 
                                                                                           //165/ 
/* The following symbols refer to set elements.                                            //166/ 
   All the N_SET[ATDB] symbols with the same name form one set.                            //167/ 
   Space is allocated for the set in the text section, and each set                        //168/ 
   element's value is stored into one word of the space.                                   //169/ 
   The first word of the space is the length of the set (number of elements).              //170/ 
                                                                                           //171/ 
   The address of the set is made into an N_SETV symbol                                    //172/ 
   whose name is the same as the name of the set.                                          //173/ 
   This symbol acts like a N_DATA global symbol                                            //174/ 
   in that it can satisfy undefined external references.  */                               //175/ 
                                                                                           //176/ 
/* These appear as input to LD, in a .o file.  */                                          //177/ 
#define	N_SETA	0x14		/* Absolute set element symbol */                          //178/ 
#define	N_SETT	0x16		/* Text set element symbol */                              //179/ 
#define	N_SETD	0x18		/* Data set element symbol */                              //180/ 
#define	N_SETB	0x1A		/* Bss set element symbol */                               //181/ 
                                                                                           //182/ 
/* This is output from LD.  */                                                             //183/ 
#define N_SETV	0x1C		/* Pointer to set vector in data area.  */                 //184/ 
                                                                                           //185/ 
#ifndef N_RELOCATION_INFO_DECLARED                                                         //186/ 
                                                                                           //187/ 
/* This structure describes a single relocation to be performed.                           //188/ 
   The text-relocation section of the file is a vector of these structures,                //189/ 
   all of which apply to the text section.                                                 //190/ 
   Likewise, the data-relocation section applies to the data section.  */                  //191/ 
                                                                                           //192/ 
struct relocation_info                                                                     //193/ 
{                                                                                          //194/ 
  /* Address (within segment) to be relocated.  */                                         //195/ 
  int r_address;                                                                           //196/ 
  /* The meaning of r_symbolnum depends on r_extern.  */                                   //197/ 
  unsigned int r_symbolnum:24;                                                             //198/ 
  /* Nonzero means value is a pc-relative offset                                           //199/ 
     and it should be relocated for changes in its own address                             //200/ 
     as well as for changes in the symbol or section specified.  */                        //201/ 
  unsigned int r_pcrel:1;                                                                  //202/ 
  /* Length (as exponent of 2) of the field to be relocated.                               //203/ 
     Thus, a value of 2 indicates 1<<2 bytes.  */                                          //204/ 
  unsigned int r_length:2;                                                                 //205/ 
  /* 1 => relocate with value of symbol.                                                   //206/ 
          r_symbolnum is the index of the symbol                                           //207/ 
	  in file's the symbol table.                                                      //208/ 
     0 => relocate with the address of a segment.                                          //209/ 
          r_symbolnum is N_TEXT, N_DATA, N_BSS or N_ABS                                    //210/ 
	  (the N_EXT bit may be set also, but signifies nothing).  */                      //211/ 
  unsigned int r_extern:1;                                                                 //212/ 
  /* Four bits that aren't used, but when writing an object file                           //213/ 
     it is desirable to clear them.  */                                                    //214/ 
  unsigned int r_pad:4;                                                                    //215/ 
};                                                                                         //216/ 
#endif /* no N_RELOCATION_INFO_DECLARED.  */                                               //217/ 
                                                                                           //218/ 
                                                                                           //219/ 
#endif /* __A_OUT_GNU_H__ */                                                               //220/ 

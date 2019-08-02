/*                                                                                                 //  1/ 
 *  linux/kernel/console.c                                                                         //  2/ 
 *                                                                                                 //  3/ 
 *  (C) 1991  Linus Torvalds                                                                       //  4/ 
 */                                                                                                //  5/ 
                                                                                                   //  6/ 
/*                                                                                                 //  7/ 
 *	console.c                                                                                  //  8/ 
 *                                                                                                 //  9/ 
 * This module implements the console io functions                                                 // 10/ 
 *	'void con_init(void)'                                                                      // 11/ 
 *	'void con_write(struct tty_queue * queue)'                                                 // 12/ 
 * Hopefully this will be a rather complete VT102 implementation.                                  // 13/ 
 *                                                                                                 // 14/ 
 * Beeping thanks to John T Kohl.                                                                  // 15/ 
 */                                                                                                // 16/ 
                                                                                                   // 17/ 
/*                                                                                                 // 18/ 
 *  NOTE!!! We sometimes disable and enable interrupts for a short while                           // 19/ 
 * (to put a word in video IO), but this will work even for keyboard                               // 20/ 
 * interrupts. We know interrupts aren't enabled when getting a keyboard                           // 21/ 
 * interrupt, as we use trap-gates. Hopefully all is well.                                         // 22/ 
 */                                                                                                // 23/ 
                                                                                                   // 24/ 
/*                                                                                                 // 25/ 
 * Code to check for different video-cards mostly by Galen Hunt,                                   // 26/ 
 * <g-hunt@ee.utah.edu>                                                                            // 27/ 
 */                                                                                                // 28/ 
                                                                                                   // 29/ 
#include <linux/sched.h>                                                                           // 30/ 
#include <linux/tty.h>                                                                             // 31/ 
#include <asm/io.h>                                                                                // 32/ 
#include <asm/system.h>                                                                            // 33/ 
                                                                                                   // 34/ 
/*                                                                                                 // 35/ 
 * These are set up by the setup-routine at boot-time:                                             // 36/ 
 */                                                                                                // 37/ 
                                                                                                   // 38/ 
#define ORIG_X			(*(unsigned char *)0x90000)                                        // 39/ 
#define ORIG_Y			(*(unsigned char *)0x90001)                                        // 40/ 
#define ORIG_VIDEO_PAGE		(*(unsigned short *)0x90004)                                       // 41/ 
#define ORIG_VIDEO_MODE		((*(unsigned short *)0x90006) & 0xff)                              // 42/ 
#define ORIG_VIDEO_COLS 	(((*(unsigned short *)0x90006) & 0xff00) >> 8)                     // 43/ 
#define ORIG_VIDEO_LINES	(25)                                                               // 44/ 
#define ORIG_VIDEO_EGA_AX	(*(unsigned short *)0x90008)                                       // 45/ 
#define ORIG_VIDEO_EGA_BX	(*(unsigned short *)0x9000a)                                       // 46/ 
#define ORIG_VIDEO_EGA_CX	(*(unsigned short *)0x9000c)                                       // 47/ 
                                                                                                   // 48/ 
#define VIDEO_TYPE_MDA		0x10	/* Monochrome Text Display	*/                         // 49/ 
#define VIDEO_TYPE_CGA		0x11	/* CGA Display 			*/                         // 50/ 
#define VIDEO_TYPE_EGAM		0x20	/* EGA/VGA in Monochrome Mode	*/                         // 51/ 
#define VIDEO_TYPE_EGAC		0x21	/* EGA/VGA in Color Mode	*/                         // 52/ 
                                                                                                   // 53/ 
#define NPAR 16                                                                                    // 54/ 
                                                                                                   // 55/ 
extern void keyboard_interrupt(void);                                                              // 56/ 
                                                                                                   // 57/ 
static unsigned char	video_type;		/* Type of display being used	*/                 // 58/ 
static unsigned long	video_num_columns;	/* Number of text columns	*/                 // 59/ 
static unsigned long	video_size_row;		/* Bytes per row		*/                 // 60/ 
static unsigned long	video_num_lines;	/* Number of test lines		*/                 // 61/ 
static unsigned char	video_page;		/* Initial video page		*/                 // 62/ 
static unsigned long	video_mem_start;	/* Start of video RAM		*/                 // 63/ 
static unsigned long	video_mem_end;		/* End of video RAM (sort of)	*/                 // 64/ 
static unsigned short	video_port_reg;		/* Video register select port	*/                 // 65/ 
static unsigned short	video_port_val;		/* Video register value port	*/                 // 66/ 
static unsigned short	video_erase_char;	/* Char+Attrib to erase with	*/                 // 67/ 
                                                                                                   // 68/ 
static unsigned long	origin;		/* Used for EGA/VGA fast scroll	*/                         // 69/ 
static unsigned long	scr_end;	/* Used for EGA/VGA fast scroll	*/                         // 70/ 
static unsigned long	pos;                                                                       // 71/ 
static unsigned long	x,y;                                                                       // 72/ 
static unsigned long	top,bottom;                                                                // 73/ 
static unsigned long	state=0;                                                                   // 74/ 
static unsigned long	npar,par[NPAR];                                                            // 75/ 
static unsigned long	ques=0;                                                                    // 76/ 
static unsigned char	attr=0x07;                                                                 // 77/ 
                                                                                                   // 78/ 
static void sysbeep(void);                                                                         // 79/ 
                                                                                                   // 80/ 
/*                                                                                                 // 81/ 
 * this is what the terminal answers to a ESC-Z or csi0c                                           // 82/ 
 * query (= vt100 response).                                                                       // 83/ 
 */                                                                                                // 84/ 
#define RESPONSE "\033[?1;2c"                                                                      // 85/ 
                                                                                                   // 86/ 
/* NOTE! gotoxy thinks x==video_num_columns is ok */                                               // 87/ 
static inline void gotoxy(unsigned int new_x,unsigned int new_y)                                   // 88/ 
{                                                                                                  // 89/ 
	if (new_x > video_num_columns || new_y >= video_num_lines)                                 // 90/ 
		return;                                                                            // 91/ 
	x=new_x;                                                                                   // 92/ 
	y=new_y;                                                                                   // 93/ 
	pos=origin + y*video_size_row + (x<<1);                                                    // 94/ 
}                                                                                                  // 95/ 
                                                                                                   // 96/ 
static inline void set_origin(void)                                                                // 97/ 
{                                                                                                  // 98/ 
	cli();                                                                                     // 99/ 
	outb_p(12, video_port_reg);                                                                //100/ 
	outb_p(0xff&((origin-video_mem_start)>>9), video_port_val);                                //101/ 
	outb_p(13, video_port_reg);                                                                //102/ 
	outb_p(0xff&((origin-video_mem_start)>>1), video_port_val);                                //103/ 
	sti();                                                                                     //104/ 
}                                                                                                  //105/ 
                                                                                                   //106/ 
static void scrup(void)                                                                            //107/ 
{                                                                                                  //108/ 
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)                        //109/ 
	{                                                                                          //110/ 
		if (!top && bottom == video_num_lines) {                                           //111/ 
			origin += video_size_row;                                                  //112/ 
			pos += video_size_row;                                                     //113/ 
			scr_end += video_size_row;                                                 //114/ 
			if (scr_end > video_mem_end) {                                             //115/ 
				__asm__("cld\n\t"                                                  //116/ 
					"rep\n\t"                                                  //117/ 
					"movsl\n\t"                                                //118/ 
					"movl _video_num_columns,%1\n\t"                           //119/ 
					"rep\n\t"                                                  //120/ 
					"stosw"                                                    //121/ 
					::"a" (video_erase_char),                                  //122/ 
					"c" ((video_num_lines-1)*video_num_columns>>1),            //123/ 
					"D" (video_mem_start),                                     //124/ 
					"S" (origin)                                               //125/ 
					:"cx","di","si");                                          //126/ 
				scr_end -= origin-video_mem_start;                                 //127/ 
				pos -= origin-video_mem_start;                                     //128/ 
				origin = video_mem_start;                                          //129/ 
			} else {                                                                   //130/ 
				__asm__("cld\n\t"                                                  //131/ 
					"rep\n\t"                                                  //132/ 
					"stosw"                                                    //133/ 
					::"a" (video_erase_char),                                  //134/ 
					"c" (video_num_columns),                                   //135/ 
					"D" (scr_end-video_size_row)                               //136/ 
					:"cx","di");                                               //137/ 
			}                                                                          //138/ 
			set_origin();                                                              //139/ 
		} else {                                                                           //140/ 
			__asm__("cld\n\t"                                                          //141/ 
				"rep\n\t"                                                          //142/ 
				"movsl\n\t"                                                        //143/ 
				"movl _video_num_columns,%%ecx\n\t"                                //144/ 
				"rep\n\t"                                                          //145/ 
				"stosw"                                                            //146/ 
				::"a" (video_erase_char),                                          //147/ 
				"c" ((bottom-top-1)*video_num_columns>>1),                         //148/ 
				"D" (origin+video_size_row*top),                                   //149/ 
				"S" (origin+video_size_row*(top+1))                                //150/ 
				:"cx","di","si");                                                  //151/ 
		}                                                                                  //152/ 
	}                                                                                          //153/ 
	else		/* Not EGA/VGA */                                                          //154/ 
	{                                                                                          //155/ 
		__asm__("cld\n\t"                                                                  //156/ 
			"rep\n\t"                                                                  //157/ 
			"movsl\n\t"                                                                //158/ 
			"movl _video_num_columns,%%ecx\n\t"                                        //159/ 
			"rep\n\t"                                                                  //160/ 
			"stosw"                                                                    //161/ 
			::"a" (video_erase_char),                                                  //162/ 
			"c" ((bottom-top-1)*video_num_columns>>1),                                 //163/ 
			"D" (origin+video_size_row*top),                                           //164/ 
			"S" (origin+video_size_row*(top+1))                                        //165/ 
			:"cx","di","si");                                                          //166/ 
	}                                                                                          //167/ 
}                                                                                                  //168/ 
                                                                                                   //169/ 
static void scrdown(void)                                                                          //170/ 
{                                                                                                  //171/ 
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)                        //172/ 
	{                                                                                          //173/ 
		__asm__("std\n\t"                                                                  //174/ 
			"rep\n\t"                                                                  //175/ 
			"movsl\n\t"                                                                //176/ 
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */               //177/ 
			"movl _video_num_columns,%%ecx\n\t"                                        //178/ 
			"rep\n\t"                                                                  //179/ 
			"stosw"                                                                    //180/ 
			::"a" (video_erase_char),                                                  //181/ 
			"c" ((bottom-top-1)*video_num_columns>>1),                                 //182/ 
			"D" (origin+video_size_row*bottom-4),                                      //183/ 
			"S" (origin+video_size_row*(bottom-1)-4)                                   //184/ 
			:"ax","cx","di","si");                                                     //185/ 
	}                                                                                          //186/ 
	else		/* Not EGA/VGA */                                                          //187/ 
	{                                                                                          //188/ 
		__asm__("std\n\t"                                                                  //189/ 
			"rep\n\t"                                                                  //190/ 
			"movsl\n\t"                                                                //191/ 
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */               //192/ 
			"movl _video_num_columns,%%ecx\n\t"                                        //193/ 
			"rep\n\t"                                                                  //194/ 
			"stosw"                                                                    //195/ 
			::"a" (video_erase_char),                                                  //196/ 
			"c" ((bottom-top-1)*video_num_columns>>1),                                 //197/ 
			"D" (origin+video_size_row*bottom-4),                                      //198/ 
			"S" (origin+video_size_row*(bottom-1)-4)                                   //199/ 
			:"ax","cx","di","si");                                                     //200/ 
	}                                                                                          //201/ 
}                                                                                                  //202/ 
                                                                                                   //203/ 
static void lf(void)                                                                               //204/ 
{                                                                                                  //205/ 
	if (y+1<bottom) {                                                                          //206/ 
		y++;                                                                               //207/ 
		pos += video_size_row;                                                             //208/ 
		return;                                                                            //209/ 
	}                                                                                          //210/ 
	scrup();                                                                                   //211/ 
}                                                                                                  //212/ 
                                                                                                   //213/ 
static void ri(void)                                                                               //214/ 
{                                                                                                  //215/ 
	if (y>top) {                                                                               //216/ 
		y--;                                                                               //217/ 
		pos -= video_size_row;                                                             //218/ 
		return;                                                                            //219/ 
	}                                                                                          //220/ 
	scrdown();                                                                                 //221/ 
}                                                                                                  //222/ 
                                                                                                   //223/ 
static void cr(void)                                                                               //224/ 
{                                                                                                  //225/ 
	pos -= x<<1;                                                                               //226/ 
	x=0;                                                                                       //227/ 
}                                                                                                  //228/ 
                                                                                                   //229/ 
static void del(void)                                                                              //230/ 
{                                                                                                  //231/ 
	if (x) {                                                                                   //232/ 
		pos -= 2;                                                                          //233/ 
		x--;                                                                               //234/ 
		*(unsigned short *)pos = video_erase_char;                                         //235/ 
	}                                                                                          //236/ 
}                                                                                                  //237/ 
                                                                                                   //238/ 
static void csi_J(int par)                                                                         //239/ 
{                                                                                                  //240/ 
	long count __asm__("cx");                                                                  //241/ 
	long start __asm__("di");                                                                  //242/ 
                                                                                                   //243/ 
	switch (par) {                                                                             //244/ 
		case 0:	/* erase from cursor to end of display */                                  //245/ 
			count = (scr_end-pos)>>1;                                                  //246/ 
			start = pos;                                                               //247/ 
			break;                                                                     //248/ 
		case 1:	/* erase from start to cursor */                                           //249/ 
			count = (pos-origin)>>1;                                                   //250/ 
			start = origin;                                                            //251/ 
			break;                                                                     //252/ 
		case 2: /* erase whole display */                                                  //253/ 
			count = video_num_columns * video_num_lines;                               //254/ 
			start = origin;                                                            //255/ 
			break;                                                                     //256/ 
		default:                                                                           //257/ 
			return;                                                                    //258/ 
	}                                                                                          //259/ 
	__asm__("cld\n\t"                                                                          //260/ 
		"rep\n\t"                                                                          //261/ 
		"stosw\n\t"                                                                        //262/ 
		::"c" (count),                                                                     //263/ 
		"D" (start),"a" (video_erase_char)                                                 //264/ 
		:"cx","di");                                                                       //265/ 
}                                                                                                  //266/ 
                                                                                                   //267/ 
static void csi_K(int par)                                                                         //268/ 
{                                                                                                  //269/ 
	long count __asm__("cx");                                                                  //270/ 
	long start __asm__("di");                                                                  //271/ 
                                                                                                   //272/ 
	switch (par) {                                                                             //273/ 
		case 0:	/* erase from cursor to end of line */                                     //274/ 
			if (x>=video_num_columns)                                                  //275/ 
				return;                                                            //276/ 
			count = video_num_columns-x;                                               //277/ 
			start = pos;                                                               //278/ 
			break;                                                                     //279/ 
		case 1:	/* erase from start of line to cursor */                                   //280/ 
			start = pos - (x<<1);                                                      //281/ 
			count = (x<video_num_columns)?x:video_num_columns;                         //282/ 
			break;                                                                     //283/ 
		case 2: /* erase whole line */                                                     //284/ 
			start = pos - (x<<1);                                                      //285/ 
			count = video_num_columns;                                                 //286/ 
			break;                                                                     //287/ 
		default:                                                                           //288/ 
			return;                                                                    //289/ 
	}                                                                                          //290/ 
	__asm__("cld\n\t"                                                                          //291/ 
		"rep\n\t"                                                                          //292/ 
		"stosw\n\t"                                                                        //293/ 
		::"c" (count),                                                                     //294/ 
		"D" (start),"a" (video_erase_char)                                                 //295/ 
		:"cx","di");                                                                       //296/ 
}                                                                                                  //297/ 
                                                                                                   //298/ 
void csi_m(void)                                                                                   //299/ 
{                                                                                                  //300/ 
	int i;                                                                                     //301/ 
                                                                                                   //302/ 
	for (i=0;i<=npar;i++)                                                                      //303/ 
		switch (par[i]) {                                                                  //304/ 
			case 0:attr=0x07;break;                                                    //305/ 
			case 1:attr=0x0f;break;                                                    //306/ 
			case 4:attr=0x0f;break;                                                    //307/ 
			case 7:attr=0x70;break;                                                    //308/ 
			case 27:attr=0x07;break;                                                   //309/ 
		}                                                                                  //310/ 
}                                                                                                  //311/ 
                                                                                                   //312/ 
static inline void set_cursor(void)                                                                //313/ 
{                                                                                                  //314/ 
	cli();                                                                                     //315/ 
	outb_p(14, video_port_reg);                                                                //316/ 
	outb_p(0xff&((pos-video_mem_start)>>9), video_port_val);                                   //317/ 
	outb_p(15, video_port_reg);                                                                //318/ 
	outb_p(0xff&((pos-video_mem_start)>>1), video_port_val);                                   //319/ 
	sti();                                                                                     //320/ 
}                                                                                                  //321/ 
                                                                                                   //322/ 
static void respond(struct tty_struct * tty)                                                       //323/ 
{                                                                                                  //324/ 
	char * p = RESPONSE;                                                                       //325/ 
                                                                                                   //326/ 
	cli();                                                                                     //327/ 
	while (*p) {                                                                               //328/ 
		PUTCH(*p,tty->read_q);                                                             //329/ 
		p++;                                                                               //330/ 
	}                                                                                          //331/ 
	sti();                                                                                     //332/ 
	copy_to_cooked(tty);                                                                       //333/ 
}                                                                                                  //334/ 
                                                                                                   //335/ 
static void insert_char(void)                                                                      //336/ 
{                                                                                                  //337/ 
	int i=x;                                                                                   //338/ 
	unsigned short tmp, old = video_erase_char;                                                //339/ 
	unsigned short * p = (unsigned short *) pos;                                               //340/ 
                                                                                                   //341/ 
	while (i++<video_num_columns) {                                                            //342/ 
		tmp=*p;                                                                            //343/ 
		*p=old;                                                                            //344/ 
		old=tmp;                                                                           //345/ 
		p++;                                                                               //346/ 
	}                                                                                          //347/ 
}                                                                                                  //348/ 
                                                                                                   //349/ 
static void insert_line(void)                                                                      //350/ 
{                                                                                                  //351/ 
	int oldtop,oldbottom;                                                                      //352/ 
                                                                                                   //353/ 
	oldtop=top;                                                                                //354/ 
	oldbottom=bottom;                                                                          //355/ 
	top=y;                                                                                     //356/ 
	bottom = video_num_lines;                                                                  //357/ 
	scrdown();                                                                                 //358/ 
	top=oldtop;                                                                                //359/ 
	bottom=oldbottom;                                                                          //360/ 
}                                                                                                  //361/ 
                                                                                                   //362/ 
static void delete_char(void)                                                                      //363/ 
{                                                                                                  //364/ 
	int i;                                                                                     //365/ 
	unsigned short * p = (unsigned short *) pos;                                               //366/ 
                                                                                                   //367/ 
	if (x>=video_num_columns)                                                                  //368/ 
		return;                                                                            //369/ 
	i = x;                                                                                     //370/ 
	while (++i < video_num_columns) {                                                          //371/ 
		*p = *(p+1);                                                                       //372/ 
		p++;                                                                               //373/ 
	}                                                                                          //374/ 
	*p = video_erase_char;                                                                     //375/ 
}                                                                                                  //376/ 
                                                                                                   //377/ 
static void delete_line(void)                                                                      //378/ 
{                                                                                                  //379/ 
	int oldtop,oldbottom;                                                                      //380/ 
                                                                                                   //381/ 
	oldtop=top;                                                                                //382/ 
	oldbottom=bottom;                                                                          //383/ 
	top=y;                                                                                     //384/ 
	bottom = video_num_lines;                                                                  //385/ 
	scrup();                                                                                   //386/ 
	top=oldtop;                                                                                //387/ 
	bottom=oldbottom;                                                                          //388/ 
}                                                                                                  //389/ 
                                                                                                   //390/ 
static void csi_at(unsigned int nr)                                                                //391/ 
{                                                                                                  //392/ 
	if (nr > video_num_columns)                                                                //393/ 
		nr = video_num_columns;                                                            //394/ 
	else if (!nr)                                                                              //395/ 
		nr = 1;                                                                            //396/ 
	while (nr--)                                                                               //397/ 
		insert_char();                                                                     //398/ 
}                                                                                                  //399/ 
                                                                                                   //400/ 
static void csi_L(unsigned int nr)                                                                 //401/ 
{                                                                                                  //402/ 
	if (nr > video_num_lines)                                                                  //403/ 
		nr = video_num_lines;                                                              //404/ 
	else if (!nr)                                                                              //405/ 
		nr = 1;                                                                            //406/ 
	while (nr--)                                                                               //407/ 
		insert_line();                                                                     //408/ 
}                                                                                                  //409/ 
                                                                                                   //410/ 
static void csi_P(unsigned int nr)                                                                 //411/ 
{                                                                                                  //412/ 
	if (nr > video_num_columns)                                                                //413/ 
		nr = video_num_columns;                                                            //414/ 
	else if (!nr)                                                                              //415/ 
		nr = 1;                                                                            //416/ 
	while (nr--)                                                                               //417/ 
		delete_char();                                                                     //418/ 
}                                                                                                  //419/ 
                                                                                                   //420/ 
static void csi_M(unsigned int nr)                                                                 //421/ 
{                                                                                                  //422/ 
	if (nr > video_num_lines)                                                                  //423/ 
		nr = video_num_lines;                                                              //424/ 
	else if (!nr)                                                                              //425/ 
		nr=1;                                                                              //426/ 
	while (nr--)                                                                               //427/ 
		delete_line();                                                                     //428/ 
}                                                                                                  //429/ 
                                                                                                   //430/ 
static int saved_x=0;                                                                              //431/ 
static int saved_y=0;                                                                              //432/ 
                                                                                                   //433/ 
static void save_cur(void)                                                                         //434/ 
{                                                                                                  //435/ 
	saved_x=x;                                                                                 //436/ 
	saved_y=y;                                                                                 //437/ 
}                                                                                                  //438/ 
                                                                                                   //439/ 
static void restore_cur(void)                                                                      //440/ 
{                                                                                                  //441/ 
	gotoxy(saved_x, saved_y);                                                                  //442/ 
}                                                                                                  //443/ 
                                                                                                   //444/ 
void con_write(struct tty_struct * tty)                                                            //445/ 
{                                                                                                  //446/ 
	int nr;                                                                                    //447/ 
	char c;                                                                                    //448/ 
                                                                                                   //449/ 
	nr = CHARS(tty->write_q);                                                                  //450/ 
	while (nr--) {                                                                             //451/ 
		GETCH(tty->write_q,c);                                                             //452/ 
		switch(state) {                                                                    //453/ 
			case 0:                                                                    //454/ 
				if (c>31 && c<127) {                                               //455/ 
					if (x>=video_num_columns) {                                //456/ 
						x -= video_num_columns;                            //457/ 
						pos -= video_size_row;                             //458/ 
						lf();                                              //459/ 
					}                                                          //460/ 
					__asm__("movb _attr,%%ah\n\t"                              //461/ 
						"movw %%ax,%1\n\t"                                 //462/ 
						::"a" (c),"m" (*(short *)pos)                      //463/ 
						:"ax");                                            //464/ 
					pos += 2;                                                  //465/ 
					x++;                                                       //466/ 
				} else if (c==27)                                                  //467/ 
					state=1;                                                   //468/ 
				else if (c==10 || c==11 || c==12)                                  //469/ 
					lf();                                                      //470/ 
				else if (c==13)                                                    //471/ 
					cr();                                                      //472/ 
				else if (c==ERASE_CHAR(tty))                                       //473/ 
					del();                                                     //474/ 
				else if (c==8) {                                                   //475/ 
					if (x) {                                                   //476/ 
						x--;                                               //477/ 
						pos -= 2;                                          //478/ 
					}                                                          //479/ 
				} else if (c==9) {                                                 //480/ 
					c=8-(x&7);                                                 //481/ 
					x += c;                                                    //482/ 
					pos += c<<1;                                               //483/ 
					if (x>video_num_columns) {                                 //484/ 
						x -= video_num_columns;                            //485/ 
						pos -= video_size_row;                             //486/ 
						lf();                                              //487/ 
					}                                                          //488/ 
					c=9;                                                       //489/ 
				} else if (c==7)                                                   //490/ 
					sysbeep();                                                 //491/ 
				break;                                                             //492/ 
			case 1:                                                                    //493/ 
				state=0;                                                           //494/ 
				if (c=='[')                                                        //495/ 
					state=2;                                                   //496/ 
				else if (c=='E')                                                   //497/ 
					gotoxy(0,y+1);                                             //498/ 
				else if (c=='M')                                                   //499/ 
					ri();                                                      //500/ 
				else if (c=='D')                                                   //501/ 
					lf();                                                      //502/ 
				else if (c=='Z')                                                   //503/ 
					respond(tty);                                              //504/ 
				else if (x=='7')                                                   //505/ 
					save_cur();                                                //506/ 
				else if (x=='8')                                                   //507/ 
					restore_cur();                                             //508/ 
				break;                                                             //509/ 
			case 2:                                                                    //510/ 
				for(npar=0;npar<NPAR;npar++)                                       //511/ 
					par[npar]=0;                                               //512/ 
				npar=0;                                                            //513/ 
				state=3;                                                           //514/ 
				if (ques=(c=='?'))                                                 //515/ 
					break;                                                     //516/ 
			case 3:                                                                    //517/ 
				if (c==';' && npar<NPAR-1) {                                       //518/ 
					npar++;                                                    //519/ 
					break;                                                     //520/ 
				} else if (c>='0' && c<='9') {                                     //521/ 
					par[npar]=10*par[npar]+c-'0';                              //522/ 
					break;                                                     //523/ 
				} else state=4;                                                    //524/ 
			case 4:                                                                    //525/ 
				state=0;                                                           //526/ 
				switch(c) {                                                        //527/ 
					case 'G': case '`':                                        //528/ 
						if (par[0]) par[0]--;                              //529/ 
						gotoxy(par[0],y);                                  //530/ 
						break;                                             //531/ 
					case 'A':                                                  //532/ 
						if (!par[0]) par[0]++;                             //533/ 
						gotoxy(x,y-par[0]);                                //534/ 
						break;                                             //535/ 
					case 'B': case 'e':                                        //536/ 
						if (!par[0]) par[0]++;                             //537/ 
						gotoxy(x,y+par[0]);                                //538/ 
						break;                                             //539/ 
					case 'C': case 'a':                                        //540/ 
						if (!par[0]) par[0]++;                             //541/ 
						gotoxy(x+par[0],y);                                //542/ 
						break;                                             //543/ 
					case 'D':                                                  //544/ 
						if (!par[0]) par[0]++;                             //545/ 
						gotoxy(x-par[0],y);                                //546/ 
						break;                                             //547/ 
					case 'E':                                                  //548/ 
						if (!par[0]) par[0]++;                             //549/ 
						gotoxy(0,y+par[0]);                                //550/ 
						break;                                             //551/ 
					case 'F':                                                  //552/ 
						if (!par[0]) par[0]++;                             //553/ 
						gotoxy(0,y-par[0]);                                //554/ 
						break;                                             //555/ 
					case 'd':                                                  //556/ 
						if (par[0]) par[0]--;                              //557/ 
						gotoxy(x,par[0]);                                  //558/ 
						break;                                             //559/ 
					case 'H': case 'f':                                        //560/ 
						if (par[0]) par[0]--;                              //561/ 
						if (par[1]) par[1]--;                              //562/ 
						gotoxy(par[1],par[0]);                             //563/ 
						break;                                             //564/ 
					case 'J':                                                  //565/ 
						csi_J(par[0]);                                     //566/ 
						break;                                             //567/ 
					case 'K':                                                  //568/ 
						csi_K(par[0]);                                     //569/ 
						break;                                             //570/ 
					case 'L':                                                  //571/ 
						csi_L(par[0]);                                     //572/ 
						break;                                             //573/ 
					case 'M':                                                  //574/ 
						csi_M(par[0]);                                     //575/ 
						break;                                             //576/ 
					case 'P':                                                  //577/ 
						csi_P(par[0]);                                     //578/ 
						break;                                             //579/ 
					case '@':                                                  //580/ 
						csi_at(par[0]);                                    //581/ 
						break;                                             //582/ 
					case 'm':                                                  //583/ 
						csi_m();                                           //584/ 
						break;                                             //585/ 
					case 'r':                                                  //586/ 
						if (par[0]) par[0]--;                              //587/ 
						if (!par[1]) par[1] = video_num_lines;             //588/ 
						if (par[0] < par[1] &&                             //589/ 
						    par[1] <= video_num_lines) {                   //590/ 
							top=par[0];                                //591/ 
							bottom=par[1];                             //592/ 
						}                                                  //593/ 
						break;                                             //594/ 
					case 's':                                                  //595/ 
						save_cur();                                        //596/ 
						break;                                             //597/ 
					case 'u':                                                  //598/ 
						restore_cur();                                     //599/ 
						break;                                             //600/ 
				}                                                                  //601/ 
		}                                                                                  //602/ 
	}                                                                                          //603/ 
	set_cursor();                                                                              //604/ 
}                                                                                                  //605/ 
                                                                                                   //606/ 
/*                                                                                                 //607/ 
 *  void con_init(void);                                                                           //608/ 
 *                                                                                                 //609/ 
 * This routine initalizes console interrupts, and does nothing                                    //610/ 
 * else. If you want the screen to clear, call tty_write with                                      //611/ 
 * the appropriate escape-sequece.                                                                 //612/ 
 *                                                                                                 //613/ 
 * Reads the information preserved by setup.s to determine the current display                     //614/ 
 * type and sets everything accordingly.                                                           //615/ 
 */                                                                                                //616/ 
void con_init(void)                                                                                //617/ 
{                                                                                                  //618/ 
	register unsigned char a;                                                                  //619/ 
	char *display_desc = "????";                                                               //620/ 
	char *display_ptr;                                                                         //621/ 
                                                                                                   //622/ 
	video_num_columns = ORIG_VIDEO_COLS;                                                       //623/ 
	video_size_row = video_num_columns * 2;                                                    //624/ 
	video_num_lines = ORIG_VIDEO_LINES;                                                        //625/ 
	video_page = ORIG_VIDEO_PAGE;                                                              //626/ 
	video_erase_char = 0x0720;                                                                 //627/ 
	                                                                                           //628/ 
	if (ORIG_VIDEO_MODE == 7)			/* Is this a monochrome display? */        //629/ 
	{                                                                                          //630/ 
		video_mem_start = 0xb0000;                                                         //631/ 
		video_port_reg = 0x3b4;                                                            //632/ 
		video_port_val = 0x3b5;                                                            //633/ 
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)                                            //634/ 
		{                                                                                  //635/ 
			video_type = VIDEO_TYPE_EGAM;                                              //636/ 
			video_mem_end = 0xb8000;                                                   //637/ 
			display_desc = "EGAm";                                                     //638/ 
		}                                                                                  //639/ 
		else                                                                               //640/ 
		{                                                                                  //641/ 
			video_type = VIDEO_TYPE_MDA;                                               //642/ 
			video_mem_end	= 0xb2000;                                                 //643/ 
			display_desc = "*MDA";                                                     //644/ 
		}                                                                                  //645/ 
	}                                                                                          //646/ 
	else								/* If not, it is color. */ //647/ 
	{                                                                                          //648/ 
		video_mem_start = 0xb8000;                                                         //649/ 
		video_port_reg	= 0x3d4;                                                           //650/ 
		video_port_val	= 0x3d5;                                                           //651/ 
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)                                            //652/ 
		{                                                                                  //653/ 
			video_type = VIDEO_TYPE_EGAC;                                              //654/ 
			video_mem_end = 0xbc000;                                                   //655/ 
			display_desc = "EGAc";                                                     //656/ 
		}                                                                                  //657/ 
		else                                                                               //658/ 
		{                                                                                  //659/ 
			video_type = VIDEO_TYPE_CGA;                                               //660/ 
			video_mem_end = 0xba000;                                                   //661/ 
			display_desc = "*CGA";                                                     //662/ 
		}                                                                                  //663/ 
	}                                                                                          //664/ 
                                                                                                   //665/ 
	/* Let the user known what kind of display driver we are using */                          //666/ 
	                                                                                           //667/ 
	display_ptr = ((char *)video_mem_start) + video_size_row - 8;                              //668/ 
	while (*display_desc)                                                                      //669/ 
	{                                                                                          //670/ 
		*display_ptr++ = *display_desc++;                                                  //671/ 
		display_ptr++;                                                                     //672/ 
	}                                                                                          //673/ 
	                                                                                           //674/ 
	/* Initialize the variables used for scrolling (mostly EGA/VGA)	*/                         //675/ 
	                                                                                           //676/ 
	origin	= video_mem_start;                                                                 //677/ 
	scr_end	= video_mem_start + video_num_lines * video_size_row;                              //678/ 
	top	= 0;                                                                               //679/ 
	bottom	= video_num_lines;                                                                 //680/ 
                                                                                                   //681/ 
	gotoxy(ORIG_X,ORIG_Y);                                                                     //682/ 
	set_trap_gate(0x21,&keyboard_interrupt);                                                   //683/ 
	outb_p(inb_p(0x21)&0xfd,0x21);                                                             //684/ 
	a=inb_p(0x61);                                                                             //685/ 
	outb_p(a|0x80,0x61);                                                                       //686/ 
	outb(a,0x61);                                                                              //687/ 
}                                                                                                  //688/ 
/* from bsd-net-2: */                                                                              //689/ 
                                                                                                   //690/ 
void sysbeepstop(void)                                                                             //691/ 关闭扬声器
{                                                                                                  //692/ 
	/* disable counter 2 */                                                                    //693/ 
	outb(inb_p(0x61)&0xFC, 0x61);                                                              //694/ 
}                                                                                                  //695/ 
                                                                                                   //696/ 
int beepcount = 0;                                                                                 //697/ 扬声器发声时间(滴答数)
                                                                                                   //698/ 
static void sysbeep(void)                                                                          //699/ 
{                                                                                                  //700/ 
	/* enable counter 2 */                                                                     //701/ 
	outb_p(inb_p(0x61)|3, 0x61);                                                               //702/ 
	/* set command for counter 2, 2 byte write */                                              //703/ 
	outb_p(0xB6, 0x43);                                                                        //704/ 
	/* send 0x637 for 750 HZ */                                                                //705/ 
	outb_p(0x37, 0x42);                                                                        //706/ 
	outb(0x06, 0x42);                                                                          //707/ 
	/* 1/8 second */                                                                           //708/ 
	beepcount = HZ/8;	                                                                   //709/ 
}                                                                                                  //710/ 

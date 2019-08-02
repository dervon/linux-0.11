/*                                                                            //  1/ 
 *  linux/tools/build.c                                                       //  2/ 
 *                                                                            //  3/ 
 *  (C) 1991  Linus Torvalds                                                  //  4/ 
 */                                                                           //  5/ 
                                                                              //  6/ 
/*                                                                            //  7/ 
 * This file builds a disk-image from three different files:                  //  8/ 
 *                                                                            //  9/ 
 * - bootsect: max 510 bytes of 8086 machine code, loads the rest             // 10/ 
 * - setup: max 4 sectors of 8086 machine code, sets up system parm           // 11/ 
 * - system: 80386 code for actual system                                     // 12/ 
 *                                                                            // 13/ 
 * It does some checking that all files are of the correct type, and          // 14/ 
 * just writes the result to stdout, removing headers and padding to          // 15/ 
 * the right amount. It also writes some system data to stderr.               // 16/ 
 */                                                                           // 17/ 
                                                                              // 18/ 
/*                                                                            // 19/ 
 * Changes by tytso to allow root device specification                        // 20/ 
 */                                                                           // 21/ 
                                                                              // 22/ 
#include <stdio.h>	/* fprintf */                                         // 23/ 标准C库头文件
#include <string.h>                                                           // 24/ 标准C库头文件
#include <stdlib.h>	/* contains exit */                                   // 25/ 标准C库头文件
#include <sys/types.h>	/* unistd.h needs this */                             // 26/ 标准C库头文件
#include <sys/stat.h>                                                         // 27/ 标准C库头文件
#include <linux/fs.h>                                                         // 28/ 标准C库头文件
#include <unistd.h>	/* contains read/write */                             // 29/ 标准C库头文件
#include <fcntl.h>                                                            // 30/ 标准C库头文件
                                                                              // 31/ 
#define MINIX_HEADER 32                                                       // 32/ 
#define GCC_HEADER 1024                                                       // 33/ 
                                                                              // 34/ 
#define SYS_SIZE 0x2000                                                       // 35/ 
                                                                              // 36/ 
#define DEFAULT_MAJOR_ROOT 3                                                  // 37/ 
#define DEFAULT_MINOR_ROOT 6                                                  // 38/ 
                                                                              // 39/ 
/* max nr of sectors of setup: don't change unless you also change            // 40/ 
 * bootsect etc */                                                            // 41/ 
#define SETUP_SECTS 4                                                         // 42/ 
                                                                              // 43/ 
#define STRINGIFY(x) #x                                                       // 44/ 
                                                                              // 45/ 
void die(char * str)                                                          // 46/ [b;]显示出错信息，并终止程序
{                                                                             // 47/ 由标准C库stdio.h提供fprintf和stderr;
	fprintf(stderr,"%s\n",str);                                           // 48/ int fprintf( FILE *stream, const char *format, ... );
	exit(1);                                                              // 49/ 根据指定的format(格式)发送信息(参数)到由stream(流)指定的文件
}                                                                             // 50/ 
                                                                              // 51/ 
void usage(void)                                                              // 52/ [b;]显示程序使用方法，并退出程序
{                                                                             // 53/ 
	die("Usage: build bootsect setup system [rootdev] [> image]");        // 54/ 
}                                                                             // 55/ 
                                                                              // 56/ 
int main(int argc, char ** argv)                                              // 57/ [b;]将bootsect模块、setup模块的MINIX执行文件头结构和system模块的a.out头结构都去掉，将它们都代码和数据部分组合到一起，写到指定的设备(从参数指定的设备取得其设备号，若参数未指定则使用默认根设备的设备号)
{                                                                             // 58/ 
	int i,c,id;                                                           // 59/ 
	char buf[1024];                                                       // 60/ 
	char major_root, minor_root;                                          // 61/ 
	struct stat sb;                                                       // 62/ 
                                                                              // 63/ 
	if ((argc != 4) && (argc != 5))                                       // 64/ 
		usage();                                                      // 65/ 
	if (argc == 5) {                                                      // 66/ 
		if (strcmp(argv[4], "FLOPPY")) {                              // 67/ 
			if (stat(argv[4], &sb)) {                             // 68/ 
				perror(argv[4]);                              // 69/ 
				die("Couldn't stat root device.");            // 70/ 
			}                                                     // 71/ 
			major_root = MAJOR(sb.st_rdev);                       // 72/ 
			minor_root = MINOR(sb.st_rdev);                       // 73/ 
		} else {                                                      // 74/ 
			major_root = 0;                                       // 75/ 
			minor_root = 0;                                       // 76/ 
		}                                                             // 77/ 
	} else {                                                              // 78/ 
		major_root = DEFAULT_MAJOR_ROOT;                              // 79/ 
		minor_root = DEFAULT_MINOR_ROOT;                              // 80/ 
	}                                                                     // 81/ 
	fprintf(stderr, "Root device is (%d, %d)\n", major_root, minor_root); // 82/ 
	if ((major_root != 2) && (major_root != 3) &&                         // 83/ 
	    (major_root != 0)) {                                              // 84/ 
		fprintf(stderr, "Illegal root device (major = %d)\n",         // 85/ 
			major_root);                                          // 86/ 
		die("Bad root device --- major #");                           // 87/ 
	}                                                                     // 88/ 
	for (i=0;i<sizeof buf; i++) buf[i]=0;                                 // 89/ 
	if ((id=open(argv[1],O_RDONLY,0))<0)                                  // 90/ 
		die("Unable to open 'boot'");                                 // 91/ 
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)                        // 92/ 
		die("Unable to read header of 'boot'");                       // 93/ 
	if (((long *) buf)[0]!=0x04100301)                                    // 94/ 
		die("Non-Minix header of 'boot'");                            // 95/ 
	if (((long *) buf)[1]!=MINIX_HEADER)                                  // 96/ 
		die("Non-Minix header of 'boot'");                            // 97/ 
	if (((long *) buf)[3]!=0)                                             // 98/ 
		die("Illegal data segment in 'boot'");                        // 99/ 
	if (((long *) buf)[4]!=0)                                             //100/ 
		die("Illegal bss in 'boot'");                                 //101/ 
	if (((long *) buf)[5] != 0)                                           //102/ 
		die("Non-Minix header of 'boot'");                            //103/ 
	if (((long *) buf)[7] != 0)                                           //104/ 
		die("Illegal symbol table in 'boot'");                        //105/ 
	i=read(id,buf,sizeof buf);                                            //106/ 
	fprintf(stderr,"Boot sector %d bytes.\n",i);                          //107/ 
	if (i != 512)                                                         //108/ 
		die("Boot block must be exactly 512 bytes");                  //109/ 
	if ((*(unsigned short *)(buf+510)) != 0xAA55)                         //110/ 
		die("Boot block hasn't got boot flag (0xAA55)");              //111/ 
	buf[508] = (char) minor_root;                                         //112/ 
	buf[509] = (char) major_root;	                                      //113/ 
	i=write(1,buf,512);                                                   //114/ 
	if (i!=512)                                                           //115/ 
		die("Write call failed");                                     //116/ 
	close (id);                                                           //117/ 
	                                                                      //118/ 
	if ((id=open(argv[2],O_RDONLY,0))<0)                                  //119/ 
		die("Unable to open 'setup'");                                //120/ 
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)                        //121/ 
		die("Unable to read header of 'setup'");                      //122/ 
	if (((long *) buf)[0]!=0x04100301)                                    //123/ 
		die("Non-Minix header of 'setup'");                           //124/ 
	if (((long *) buf)[1]!=MINIX_HEADER)                                  //125/ 
		die("Non-Minix header of 'setup'");                           //126/ 
	if (((long *) buf)[3]!=0)                                             //127/ 
		die("Illegal data segment in 'setup'");                       //128/ 
	if (((long *) buf)[4]!=0)                                             //129/ 
		die("Illegal bss in 'setup'");                                //130/ 
	if (((long *) buf)[5] != 0)                                           //131/ 
		die("Non-Minix header of 'setup'");                           //132/ 
	if (((long *) buf)[7] != 0)                                           //133/ 
		die("Illegal symbol table in 'setup'");                       //134/ 
	for (i=0 ; (c=read(id,buf,sizeof buf))>0 ; i+=c )                     //135/ 
		if (write(1,buf,c)!=c)                                        //136/ 
			die("Write call failed");                             //137/ 
	close (id);                                                           //138/ 
	if (i > SETUP_SECTS*512)                                              //139/ 
		die("Setup exceeds " STRINGIFY(SETUP_SECTS)                   //140/ 
			" sectors - rewrite build/boot/setup");               //141/ 
	fprintf(stderr,"Setup is %d bytes.\n",i);                             //142/ 
	for (c=0 ; c<sizeof(buf) ; c++)                                       //143/ 
		buf[c] = '\0';                                                //144/ 
	while (i<SETUP_SECTS*512) {                                           //145/ 
		c = SETUP_SECTS*512-i;                                        //146/ 
		if (c > sizeof(buf))                                          //147/ 
			c = sizeof(buf);                                      //148/ 
		if (write(1,buf,c) != c)                                      //149/ 
			die("Write call failed");                             //150/ 
		i += c;                                                       //151/ 
	}                                                                     //152/ 
	                                                                      //153/ 
	if ((id=open(argv[3],O_RDONLY,0))<0)                                  //154/ 
		die("Unable to open 'system'");                               //155/ 
	if (read(id,buf,GCC_HEADER) != GCC_HEADER)                            //156/ 
		die("Unable to read header of 'system'");                     //157/ 
	if (((long *) buf)[5] != 0)                                           //158/ 
		die("Non-GCC header of 'system'");                            //159/ 
	for (i=0 ; (c=read(id,buf,sizeof buf))>0 ; i+=c )                     //160/ 
		if (write(1,buf,c)!=c)                                        //161/ 
			die("Write call failed");                             //162/ 
	close(id);                                                            //163/ 
	fprintf(stderr,"System is %d bytes.\n",i);                            //164/ 
	if (i > SYS_SIZE*16)                                                  //165/ 
		die("System is too big");                                     //166/ 
	return(0);                                                            //167/ 
}                                                                             //168/ 

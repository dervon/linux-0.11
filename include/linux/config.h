#ifndef _CONFIG_H                                                      // 1/ 
#define _CONFIG_H                                                      // 2/ 
                                                                       // 3/ 
/*                                                                     // 4/ 
 * The root-device is no longer hard-coded. You can change the default // 5/ 
 * root-device by changing the line ROOT_DEV = XXX in boot/bootsect.s  // 6/ 
 */                                                                    // 7/ 
                                                                       // 8/ 
/*                                                                     // 9/ 
 * define your keyboard here -                                         //10/ 
 * KBD_FINNISH for Finnish keyboards                                   //11/ 
 * KBD_US for US-type                                                  //12/ 
 * KBD_GR for German keyboards                                         //13/ 
 * KBD_FR for Frech keyboard                                           //14/ 
 */                                                                    //15/ 
/*#define KBD_US */                                                    //16/ 美式键盘
/*#define KBD_GR */                                                    //17/ 德式键盘
/*#define KBD_FR */                                                    //18/ 法式键盘
#define KBD_FINNISH                                                    //19/ 芬兰式键盘--由于作者的键盘类型是芬兰式
                                                                       //20/ 
/*                                                                     //21/ 
 * Normally, Linux can get the drive parameters from the BIOS at       //22/ 
 * startup, but if this for some unfathomable reason fails, you'd      //23/ 
 * be left stranded. For this case, you can define HD_TYPE, which      //24/ 
 * contains all necessary info on your harddisk.                       //25/ 
 *                                                                     //26/ 
 * The HD_TYPE macro should look like this:                            //27/ 
 *                                                                     //28/ 
 * #define HD_TYPE { head, sect, cyl, wpcom, lzone, ctl}               //29/ 
 *                                                                     //30/ 
 * In case of two harddisks, the info should be sepatated by           //31/ 
 * commas:                                                             //32/ 
 *                                                                     //33/ 
 * #define HD_TYPE { h,s,c,wpcom,lz,ctl },{ h,s,c,wpcom,lz,ctl }       //34/ 
 */                                                                    //35/ 
/*                                                                     //36/ 
 This is an example, two drives, first is type 2, second is type 3:    //37/ 
                                                                       //38/ 
#define HD_TYPE { 4,17,615,300,615,8 }, { 6,17,615,300,615,0 }         //39/ 
                                                                       //40/ 
 NOTE: ctl is 0 for all drives with heads<=8, and ctl=8 for drives     //41/ 
 with more than 8 heads.                                               //42/ 
                                                                       //43/ 
 If you want the BIOS to tell what kind of drive you have, just        //44/ 
 leave HD_TYPE undefined. This is the normal thing to do.              //45/ 
*/                                                                     //46/ 
                                                                       //47/ 
#endif                                                                 //48/ 

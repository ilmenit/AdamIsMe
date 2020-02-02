#ifndef MY_TYPES_H
#define MY_TYPES_H
 
/*****************************************************************************/
/*                               Types                                       */
/*****************************************************************************/

typedef unsigned char byte;
typedef unsigned int uint; // we don't use 'word' as type, because the game has a lot of 'words'
#ifndef bool
	#ifndef __cplusplus
	typedef unsigned char bool;
	#endif
#endif
typedef unsigned char direction; // 0,1,2,3

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#endif // !MY_TYPES_H

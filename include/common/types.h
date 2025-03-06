#ifndef __MYOS__COMMON__TYPES_H
#define __MYOS__COMMON__TYPES_H
#define NULL 0

/* Bits in the third argument to `waitpid'.  */
#define	WNOHANG		1	/* Don't block waiting.  */
#define	WUNTRACED	2	/* Report status of stopped children.  */

namespace myos
{
    namespace common
    {
        
        typedef char                     int8_t;
        typedef unsigned char           uint8_t;
        typedef short                   int16_t;
        typedef unsigned short         uint16_t;
        typedef int                     int32_t;
        typedef unsigned int           uint32_t;
        typedef long long int           int64_t;
        typedef unsigned long long int uint64_t;
    
        typedef const char*              string;
        typedef uint32_t                 size_t;
        typedef uint32_t                 pid_t;
    }
}
    
#endif
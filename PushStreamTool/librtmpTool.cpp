#include "librtmpTool.h"
#include <WinSock2.h>

void CleanupSockets()
{
#ifdef WIN32
    WSACleanup();
#endif
}

int InitSockets()
{
#ifdef WIN32
    WORD version;
    WSADATA wsaData;
    version=MAKEWORD(2,2);
    return (WSAStartup(version, &wsaData) == 0);
#endif
}

/*read 4 byte and convert to time format*/
int ReadTime(uint32_t *utime,FILE*fp){
    if(fread(utime,4,1,fp)!=1)
        return 0;
    *utime=HTONTIME(*utime);
    return 1;
}

/*read 3 byte*/
int ReadU24(uint32_t *u24,FILE*fp){
    if(fread(u24,3,1,fp)!=1)
        return 0;
    *u24=HTON24(*u24);
    return 1;
}

/*read 1 byte*/
int ReadU8(uint32_t *u8,FILE*fp){
    if(fread(u8,1,1,fp)!=1)
        return 0;
    return 1;
}

/*read 4 byte*/
int ReadU32(uint32_t *u32,FILE*fp){
    if(fread(u32,4,1,fp)!=1)
        return 0;
    *u32=HTON32(*u32);
    return 1;
}

/*read 1 byte,and loopback 1 byte at once*/
int PeekU8(uint32_t *u8,FILE*fp){
    if(fread(u8,1,1,fp)!=1)
        return 0;
    fseek(fp,-1,SEEK_CUR);
    return 1;
}
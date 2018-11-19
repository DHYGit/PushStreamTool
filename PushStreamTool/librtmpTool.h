#pragma once
#include <stdint.h>
#include <stdio.h>

#define HTONTIME(x) ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00)|(x&0xff000000))
#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00))
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|(x<<8&0xff0000)|(x<<24&0xff000000))

void CleanupSockets();
int InitSockets();
int ReadTime(uint32_t *utime,FILE*fp);
int ReadU24(uint32_t *u24,FILE*fp);
int ReadU8(uint32_t *u8,FILE*fp);
int ReadU32(uint32_t *u32,FILE*fp);
int PeekU8(uint32_t *u8,FILE*fp);
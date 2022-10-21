#pragma once
#include <cstdint>
#include <fstream>
using namespace std;

#define DISKSIZE (100 * 1 << 20) //100兆的文件系统存储空间
#define BLOCKSIZE (1 << 10)  // 1kB数据块
#define BLOCKCOUNT (DISKSIZE/BLOCKSIZE) //系统盘块数目

bool initDisk(string diskPath);
void openDisk(string diskPath);
void closeDisk();
bool readDisk(uint8_t* buff, uint64_t pos, uint32_t len);
bool writeDisk(uint8_t* buff, uint64_t pos, uint32_t len);
void formatDisk(uint32_t blocksize = 1 << 10, uint32_t FCBBlockNum = 0);
void printDiskInfo();
uint32_t getDiskSize();
uint16_t getBlockSize();
uint8_t getBlockCount();

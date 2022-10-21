#define _CRT_SECURE_NO_WARNINGS
#include<bits/stdc++.h>
#include "DriverOperate.h"
#include "FileSystem.h"
#include "User.h"

fstream Disk; //文件流磁盘
extern SuperBlock Super;
extern BitMap* DataBitMap;
extern BitMap* FCBBitMap;

//初始化磁盘
bool initDisk(string diskPath) {
	openDisk(diskPath);
	if (!Disk.is_open()) {
		Disk.open(diskPath, ios::out | ios::binary);
		char block[1 << 10];  // 每块1024
		memset(block, 0, (1 << 10)); // 初始化块全部为0
		for (int i = 0; i < BLOCKCOUNT; i++) {
			Disk.write(block, (1 << 10));
		}
		closeDisk();
		openDisk(diskPath);
	}
	return true;
}

//根据路径打开磁盘
void openDisk(string diskPath) {
	Disk.open(diskPath, ios::in | ios::out | ios::binary);
}

//关闭磁盘，将缓冲区内容写入磁盘
void closeDisk() {
	Disk.close();
}

//从磁盘读到内存
bool readDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekp(pos);
	if (Disk.read((char*)buff, len)) {// ostream 从磁盘读出
		return true;
	}
	return false;
}

//将内存信息写入磁盘
bool writeDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekg(pos);
	if (Disk.write((char*)buff, len)) { //istream 写入磁盘
		return true;
	}
	return false;
}

//格式化磁盘
void formatDisk(uint32_t blocksize, uint32_t FCBBlockNum) {
	//构建Super块
	Super.DiskSizeOfByte = getDiskSize();
	Super.BlockSizeOfByte = blocksize;
	Super.UserSizeOfByte = USER_LEN;
	Super.BlockNum = Super.DiskSizeOfByte / Super.BlockSizeOfByte;
	strcpy(Super.Version, VERSION_STRING);
	if (FCBBlockNum == 0) {
		FCBBlockNum = (Super.BlockNum / 4) / (Super.BlockSizeOfByte / FILE_CONTROL_BLOCK_SIZE);
	}
	Super.FCBNum = FCBBlockNum * blocksize / FILE_CONTROL_BLOCK_SIZE;

	uint32_t FCBBitmapBlock = (uint32_t)ceil(Super.FCBNum / (8.0 * blocksize));                   // FCB位示图所占Blcok数
	uint32_t RemainBlock = Super.BlockNum - 1 - FCBBitmapBlock - FCBBlockNum;                     //剩余的block数 = 总block数 - super - FCB位示图 - [Data位示图] - FCBBlockNum - [Data]
	uint32_t DataBitmapBlock = (uint32_t)ceil(RemainBlock * 1.0 / ((uint64_t)1 + 8 * blocksize)); //位示图占x个block，data区有`blocksize`*8*x个DataBlock。所以x=Remian/(1+8*blocksize)
	Super.DataBlockNum = RemainBlock - DataBitmapBlock;
	Super.FCBBitMapStart = 1;
	Super.DataBitMapStart = 1 + FCBBitmapBlock;
	Super.FCBStart = Super.DataBitMapStart + DataBitmapBlock;
	Super.DataStart = Super.FCBStart + FCBBlockNum;
	uint8_t* blockBuff = (uint8_t*)calloc(Super.BlockSizeOfByte, sizeof(uint8_t));
	memcpy(blockBuff, &Super, sizeof(Super));
	writeDisk(blockBuff, 0, Super.BlockSizeOfByte);
	free(blockBuff);

	//初始化超级用户
	Super.UserNum = 0;
	addUser("root", "root", 0xFF);

	//初始化bitMap
	FCBBitMap = new BitMap(Super.FCBNum);        //初始化FCB的bitmap
	DataBitMap = new BitMap(Super.DataBlockNum); //初始化Data的bitmap
	
	//空目录不需要数据块,构建根目录FCB
	FileControlBlock DirRoot(FileType::Directory, "Root", 0xFF, 0);
	time(&DirRoot.CreateTime);

	DirRoot.Size = 0;
	setBlockInUse((FCBBitMap), 0); //设置DataBlock的bitmap
	writeFCB(0, &DirRoot); // Root的FCB.id = 0
	writeDisk((FCBBitMap)->MapData, Super.BlockSizeOfByte * Super.FCBBitMapStart, (FCBBitMap)->MapSizeOfByte);    //写入FCB的bitmap
	writeDisk((DataBitMap)->MapData, Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //写入DataBlock的bitmap

}


//打印磁盘信息
void printDiskInfo() {
	printf("磁盘:\n");
	printf("    磁盘大小     = %8.2lf MByte\n", Super.DiskSizeOfByte * 1.0 / ((uint64_t)1 << 20));
	printf("    块大小       = %8d Bytes\n", Super.BlockSizeOfByte);
	printf("    最大文件     = %8.2lf KByte\n", ((((uint64_t)10 + (Super.BlockSizeOfByte - sizeof(Block)) / sizeof(BlockIndex)) * Super.BlockSizeOfByte)) * 1.0 / 1024);
	printf("分块:\n");
	printf("    超级块        = %8d Blocks\n", 1);
	printf("    FCB块位示图   = %8d Blocks\n", Super.DataBitMapStart - Super.FCBBitMapStart);
	printf("    数据块位示图  = %8d Blocks\n", Super.FCBStart - Super.DataBitMapStart);
	printf("    FCB           = %8d Blocks\n", Super.DataStart - Super.FCBStart);
	printf("    数据          = %8d Blocks\n", Super.DataBlockNum);
	printf("    合计          = %8d Blocks\n", Super.BlockNum);
	printf("使用情况:\n");
	uint32_t freeFCB = 0, freeBlock = 0, usedFCB = 0, usedBlock = 0;
	for (size_t i = 0; i < (FCBBitMap)->MapSizeOfBit; i++) {
		if (!isBlockInUse((FCBBitMap), i)) freeFCB++;
		else usedFCB++;
	}
	for (size_t i = 0; i < (DataBitMap)->MapSizeOfBit; i++) {
		if (!isBlockInUse((DataBitMap), i)) freeBlock++;
		else usedBlock++;
	}
	printf("    FCB总数       = %8d pics\n", freeFCB + usedFCB);
	printf("    FCB可用       = %8d pics\n", freeFCB);
	printf("    FCB已用       = %8d pics\n", usedFCB);
	printf("    总块数        = %8d Blocks\n", freeBlock + usedBlock);
	printf("    可使用块数    = %8d Blocks\n", freeBlock);
	printf("    已使用块数    = %8d Blocks\n", usedBlock);
	printf("用户情况:\n");
	printf("	用户总量	  = %8d \n", 30);
	printf("	已有用户	  = %8d \n", Super.UserNum);
	printf("	剩余用户	  = %8d \n", 30 - Super.UserNum);

}

//工具函数
uint32_t getDiskSize() {
	return DISKSIZE;
}

uint16_t getBlockSize() {
	return BLOCKSIZE;
}

uint8_t getBlockCount() {
	return BLOCKCOUNT;
}
#define _CRT_SECURE_NO_WARNINGS
#include<bits/stdc++.h>
#include "DriverOperate.h"
#include "FileSystem.h"
#include "User.h"

fstream Disk; //�ļ�������
extern SuperBlock Super;
extern BitMap* DataBitMap;
extern BitMap* FCBBitMap;

//��ʼ������
bool initDisk(string diskPath) {
	openDisk(diskPath);
	if (!Disk.is_open()) {
		Disk.open(diskPath, ios::out | ios::binary);
		char block[1 << 10];  // ÿ��1024
		memset(block, 0, (1 << 10)); // ��ʼ����ȫ��Ϊ0
		for (int i = 0; i < BLOCKCOUNT; i++) {
			Disk.write(block, (1 << 10));
		}
		closeDisk();
		openDisk(diskPath);
	}
	return true;
}

//����·���򿪴���
void openDisk(string diskPath) {
	Disk.open(diskPath, ios::in | ios::out | ios::binary);
}

//�رմ��̣�������������д�����
void closeDisk() {
	Disk.close();
}

//�Ӵ��̶����ڴ�
bool readDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekp(pos);
	if (Disk.read((char*)buff, len)) {// ostream �Ӵ��̶���
		return true;
	}
	return false;
}

//���ڴ���Ϣд�����
bool writeDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekg(pos);
	if (Disk.write((char*)buff, len)) { //istream д�����
		return true;
	}
	return false;
}

//��ʽ������
void formatDisk(uint32_t blocksize, uint32_t FCBBlockNum) {
	//����Super��
	Super.DiskSizeOfByte = getDiskSize();
	Super.BlockSizeOfByte = blocksize;
	Super.UserSizeOfByte = USER_LEN;
	Super.BlockNum = Super.DiskSizeOfByte / Super.BlockSizeOfByte;
	strcpy(Super.Version, VERSION_STRING);
	if (FCBBlockNum == 0) {
		FCBBlockNum = (Super.BlockNum / 4) / (Super.BlockSizeOfByte / FILE_CONTROL_BLOCK_SIZE);
	}
	Super.FCBNum = FCBBlockNum * blocksize / FILE_CONTROL_BLOCK_SIZE;

	uint32_t FCBBitmapBlock = (uint32_t)ceil(Super.FCBNum / (8.0 * blocksize));                   // FCBλʾͼ��ռBlcok��
	uint32_t RemainBlock = Super.BlockNum - 1 - FCBBitmapBlock - FCBBlockNum;                     //ʣ���block�� = ��block�� - super - FCBλʾͼ - [Dataλʾͼ] - FCBBlockNum - [Data]
	uint32_t DataBitmapBlock = (uint32_t)ceil(RemainBlock * 1.0 / ((uint64_t)1 + 8 * blocksize)); //λʾͼռx��block��data����`blocksize`*8*x��DataBlock������x=Remian/(1+8*blocksize)
	Super.DataBlockNum = RemainBlock - DataBitmapBlock;
	Super.FCBBitMapStart = 1;
	Super.DataBitMapStart = 1 + FCBBitmapBlock;
	Super.FCBStart = Super.DataBitMapStart + DataBitmapBlock;
	Super.DataStart = Super.FCBStart + FCBBlockNum;
	uint8_t* blockBuff = (uint8_t*)calloc(Super.BlockSizeOfByte, sizeof(uint8_t));
	memcpy(blockBuff, &Super, sizeof(Super));
	writeDisk(blockBuff, 0, Super.BlockSizeOfByte);
	free(blockBuff);

	//��ʼ�������û�
	Super.UserNum = 0;
	addUser("root", "root", 0xFF);

	//��ʼ��bitMap
	FCBBitMap = new BitMap(Super.FCBNum);        //��ʼ��FCB��bitmap
	DataBitMap = new BitMap(Super.DataBlockNum); //��ʼ��Data��bitmap
	
	//��Ŀ¼����Ҫ���ݿ�,������Ŀ¼FCB
	FileControlBlock DirRoot(FileType::Directory, "Root", 0xFF, 0);
	time(&DirRoot.CreateTime);

	DirRoot.Size = 0;
	setBlockInUse((FCBBitMap), 0); //����DataBlock��bitmap
	writeFCB(0, &DirRoot); // Root��FCB.id = 0
	writeDisk((FCBBitMap)->MapData, Super.BlockSizeOfByte * Super.FCBBitMapStart, (FCBBitMap)->MapSizeOfByte);    //д��FCB��bitmap
	writeDisk((DataBitMap)->MapData, Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //д��DataBlock��bitmap

}


//��ӡ������Ϣ
void printDiskInfo() {
	printf("����:\n");
	printf("    ���̴�С     = %8.2lf MByte\n", Super.DiskSizeOfByte * 1.0 / ((uint64_t)1 << 20));
	printf("    ���С       = %8d Bytes\n", Super.BlockSizeOfByte);
	printf("    ����ļ�     = %8.2lf KByte\n", ((((uint64_t)10 + (Super.BlockSizeOfByte - sizeof(Block)) / sizeof(BlockIndex)) * Super.BlockSizeOfByte)) * 1.0 / 1024);
	printf("�ֿ�:\n");
	printf("    ������        = %8d Blocks\n", 1);
	printf("    FCB��λʾͼ   = %8d Blocks\n", Super.DataBitMapStart - Super.FCBBitMapStart);
	printf("    ���ݿ�λʾͼ  = %8d Blocks\n", Super.FCBStart - Super.DataBitMapStart);
	printf("    FCB           = %8d Blocks\n", Super.DataStart - Super.FCBStart);
	printf("    ����          = %8d Blocks\n", Super.DataBlockNum);
	printf("    �ϼ�          = %8d Blocks\n", Super.BlockNum);
	printf("ʹ�����:\n");
	uint32_t freeFCB = 0, freeBlock = 0, usedFCB = 0, usedBlock = 0;
	for (size_t i = 0; i < (FCBBitMap)->MapSizeOfBit; i++) {
		if (!isBlockInUse((FCBBitMap), i)) freeFCB++;
		else usedFCB++;
	}
	for (size_t i = 0; i < (DataBitMap)->MapSizeOfBit; i++) {
		if (!isBlockInUse((DataBitMap), i)) freeBlock++;
		else usedBlock++;
	}
	printf("    FCB����       = %8d pics\n", freeFCB + usedFCB);
	printf("    FCB����       = %8d pics\n", freeFCB);
	printf("    FCB����       = %8d pics\n", usedFCB);
	printf("    �ܿ���        = %8d Blocks\n", freeBlock + usedBlock);
	printf("    ��ʹ�ÿ���    = %8d Blocks\n", freeBlock);
	printf("    ��ʹ�ÿ���    = %8d Blocks\n", usedBlock);
	printf("�û����:\n");
	printf("	�û�����	  = %8d \n", 30);
	printf("	�����û�	  = %8d \n", Super.UserNum);
	printf("	ʣ���û�	  = %8d \n", 30 - Super.UserNum);

}

//���ߺ���
uint32_t getDiskSize() {
	return DISKSIZE;
}

uint16_t getBlockSize() {
	return BLOCKSIZE;
}

uint8_t getBlockCount() {
	return BLOCKCOUNT;
}
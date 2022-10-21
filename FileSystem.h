#pragma once
#include <bits/stdc++.h>
#include "DriverOperate.h"
#define _CRT_SECURE_NO_WARNINGS
#define FILE_CONTROL_BLOCK_SIZE 128u
#define MAP_ROW_LEN 8
#define MAX_POINTER ((Super.BlockSizeOfByte - sizeof(Block)) / sizeof(BlockIndex) + 10)
#define MAX_BLOCK_SPACE (Super.BlockSizeOfByte - sizeof(Block))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define SUPER_SIZE sizeof(SuperBlock)
#define VERSION_STRING "SimDisk v-1.0"
#define INODESIZE 128 

using namespace std;

typedef int32_t BlockIndex;
typedef int32_t FCBIndex;

//�ļ�����
enum class FileType : uint8_t { File, Directory };

class Access {
public:
	static constexpr uint8_t None = 0;
	static constexpr uint8_t Read = 1 << 1;
	static constexpr uint8_t Write = 1 << 2;
	static constexpr uint8_t Delete = 1 << 3;
	static constexpr uint8_t All = 0xF;
};


/****������****/
typedef struct SuperBlock {
	uint32_t DiskSizeOfByte;             //���̴�С
	uint32_t BlockSizeOfByte;            //���С
	uint32_t BlockNum;             //������(=DISK_SIZE/���С)
	BlockIndex FCBBitMapStart;    // FCB��λʾͼ�б�ʼ���
	BlockIndex DataBitMapStart;   // Data��λʾͼ���ݿ鿪ʼ���
	BlockIndex FCBStart;          // FCB�б�ʼ���
	BlockIndex DataStart;         // Data���ݿ鿪ʼ���
	uint32_t FCBNum;		//FCB����
	uint32_t DataBlockNum; // FDataBlock����
	uint8_t UserNum;			   //�û�����
	uint8_t UserSizeOfByte;		//�û���С
	char Version[16];         //�ļ�ϵͳ�汾��
	uint8_t padding[9];	//��Ե���
}SuperBlock;


/****λʾͼ****/
typedef struct BitMap {
	uint32_t MapSizeOfBit; //��bitΪ��λ��λʾͼ����
	uint32_t MapSizeOfByte; //��ByteΪ��λ��λʾͼ����
	uint8_t* MapData = nullptr; //λʾͼָ�룬ÿ��Ԫ�ش�8λ
	/* row			mapdata
		0		0 0 0 0 0 0 0 0
		1		0 0 0 0 0 0 0 0
		2		0 0 0 0 0 0 0 0
		......
	*/
	//��ʼ��λʾͼ
	BitMap() {};
	BitMap(uint32_t size) {
		this->MapSizeOfBit = size; //��¼�ж��ٿ�
		this->MapSizeOfByte = (uint16_t)ceil(size / MAP_ROW_LEN); //����ȡ���õ��ֽ���
		this->MapData = (uint8_t*)calloc(this->MapSizeOfByte, sizeof(uint8_t)); //����λʾͼ
	};
}BitMap;
//λʾͼ������غ�������
bool isBlockInUse(BitMap* map, uint32_t index); 
void setBlockInUse(BitMap* map, uint32_t index);
void setBlockFree(BitMap* map, uint32_t index);
bool freeBitMap(BitMap* map);


/****�����������ݿ�****/
typedef struct Block {
	uint32_t Size = 0; //������Ч���ݵĳ���
}Block;
typedef Block DataBlock;
//���ݿ���غ�������
uint32_t& blockSize(uint8_t* blockBuff);
BlockIndex getEmptyDataBlock();
void loadDataBlock(BlockIndex index, uint8_t* buff);
void writeDataBlock(BlockIndex index, uint8_t* buff);
void makeDirBlock(FCBIndex firstIndex, uint8_t* buff);


/****�ļ����ƿ�****/
typedef struct FileControlBlock {
	char FileName[32]; //�ļ���
	enum FileType Type; //�ļ�����{�ļ����ļ���}
	uint32_t Size; //�ļ���С
	time_t CreateTime = 0, ModifyTime = 0, ReadTime = 0;//��д������Ϣ
	uint8_t AccessMode; //���ʿ���
	FCBIndex Parent; //��FCB�������
	BlockIndex DirectBlock[10] = { //ֱ���ļ���
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 
	}; 
	BlockIndex Pointer = -1; //һ���ļ�ָ������
	uint8_t padding[6];//��Ե���
	//��ʼ��
	FileControlBlock() {};
	FileControlBlock(enum FileType t, const char* fileName, uint8_t accessMode, FCBIndex parent) {
		strcpy_s(FileName, fileName);
		this->Type = t;
		time(&this->CreateTime);
		this->Size = 0;
		this->Parent = parent;
		this->AccessMode = accessMode;
	};
}FileControlBlock;
//�ļ����ƿ���غ�������
FCBIndex getEmptyFCB();
void loadFCB(FCBIndex index, FileControlBlock* buff);
void writeFCB(FCBIndex index, FileControlBlock* buff);


/****�ļ�ָ���ȡ****/
typedef struct FilePointerReader {
	FileControlBlock FCB; //�ļ����ƿ�
	uint8_t* Pointer = (uint8_t*)malloc(getBlockSize()); //����ָ��
	FCBIndex Self_Index = -1;     //FCB������
	vector<BlockIndex> BlockList; //����
}FPR;
//�ļ���ȡ��غ�������
BlockIndex getBlockIndex(FPR* fpr, BlockIndex index);
BlockIndex addNewBlock(FPR* fpr, uint8_t* blockBuff);
BlockIndex readPage(FPR* fpr, uint32_t index, uint8_t* blockBuff);
bool freeBPR(FPR* fpr);



FCBIndex createDirectory(const string& name, FCBIndex parent, uint8_t access);
FCBIndex createFile(const string& name, FCBIndex dir, uint8_t access);

void printDir(FCBIndex dir);
void printInfo(FCBIndex file);

FCBIndex create(const string& name, FCBIndex dir, enum FileType t, uint8_t access);
int64_t readFile(FCBIndex file, int64_t pos, int64_t len, uint8_t* buff);
int64_t writeFile(FCBIndex file, int64_t pos, int64_t len, const uint8_t* buff);
bool deleteFile(FCBIndex file);
bool changeAccessMode(FCBIndex file, uint8_t newMode);
bool checkAccess(FCBIndex file,FileControlBlock fileFCB, uint8_t operate, uint8_t access);
FCBIndex find(FCBIndex dir, const string& filename);
void fileInfo(FCBIndex file, FileControlBlock* fcb);
vector<FCBIndex> getChildren(FCBIndex dir);

//ϵͳ��ʼ��������
bool FSInit();
bool FSDestruction();
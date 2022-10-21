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

//文件类型
enum class FileType : uint8_t { File, Directory };

class Access {
public:
	static constexpr uint8_t None = 0;
	static constexpr uint8_t Read = 1 << 1;
	static constexpr uint8_t Write = 1 << 2;
	static constexpr uint8_t Delete = 1 << 3;
	static constexpr uint8_t All = 0xF;
};


/****超级块****/
typedef struct SuperBlock {
	uint32_t DiskSizeOfByte;             //磁盘大小
	uint32_t BlockSizeOfByte;            //块大小
	uint32_t BlockNum;             //块数量(=DISK_SIZE/块大小)
	BlockIndex FCBBitMapStart;    // FCB的位示图列表开始块号
	BlockIndex DataBitMapStart;   // Data的位示图数据块开始块号
	BlockIndex FCBStart;          // FCB列表开始块号
	BlockIndex DataStart;         // Data数据块开始块号
	uint32_t FCBNum;		//FCB数量
	uint32_t DataBlockNum; // FDataBlock数量
	uint8_t UserNum;			   //用户数量
	uint8_t UserSizeOfByte;		//用户大小
	char Version[16];         //文件系统版本号
	uint8_t padding[9];	//边缘填充
}SuperBlock;


/****位示图****/
typedef struct BitMap {
	uint32_t MapSizeOfBit; //以bit为单位的位示图长度
	uint32_t MapSizeOfByte; //以Byte为单位的位示图长度
	uint8_t* MapData = nullptr; //位示图指针，每个元素存8位
	/* row			mapdata
		0		0 0 0 0 0 0 0 0
		1		0 0 0 0 0 0 0 0
		2		0 0 0 0 0 0 0 0
		......
	*/
	//初始化位示图
	BitMap() {};
	BitMap(uint32_t size) {
		this->MapSizeOfBit = size; //记录有多少块
		this->MapSizeOfByte = (uint16_t)ceil(size / MAP_ROW_LEN); //向上取整得到字节数
		this->MapData = (uint8_t*)calloc(this->MapSizeOfByte, sizeof(uint8_t)); //分配位示图
	};
}BitMap;
//位示图操作相关函数声明
bool isBlockInUse(BitMap* map, uint32_t index); 
void setBlockInUse(BitMap* map, uint32_t index);
void setBlockFree(BitMap* map, uint32_t index);
bool freeBitMap(BitMap* map);


/****基本块与数据块****/
typedef struct Block {
	uint32_t Size = 0; //块内有效数据的长度
}Block;
typedef Block DataBlock;
//数据块相关函数声明
uint32_t& blockSize(uint8_t* blockBuff);
BlockIndex getEmptyDataBlock();
void loadDataBlock(BlockIndex index, uint8_t* buff);
void writeDataBlock(BlockIndex index, uint8_t* buff);
void makeDirBlock(FCBIndex firstIndex, uint8_t* buff);


/****文件控制块****/
typedef struct FileControlBlock {
	char FileName[32]; //文件名
	enum FileType Type; //文件类型{文件或文件夹}
	uint32_t Size; //文件大小
	time_t CreateTime = 0, ModifyTime = 0, ReadTime = 0;//读写控制信息
	uint8_t AccessMode; //访问控制
	FCBIndex Parent; //父FCB结点索引
	BlockIndex DirectBlock[10] = { //直接文件块
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 
	}; 
	BlockIndex Pointer = -1; //一级文件指针索引
	uint8_t padding[6];//边缘填充
	//初始化
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
//文件控制块相关函数声明
FCBIndex getEmptyFCB();
void loadFCB(FCBIndex index, FileControlBlock* buff);
void writeFCB(FCBIndex index, FileControlBlock* buff);


/****文件指针读取****/
typedef struct FilePointerReader {
	FileControlBlock FCB; //文件控制块
	uint8_t* Pointer = (uint8_t*)malloc(getBlockSize()); //链接指针
	FCBIndex Self_Index = -1;     //FCB的索引
	vector<BlockIndex> BlockList; //块链
}FPR;
//文件读取相关函数声明
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

//系统初始化与销毁
bool FSInit();
bool FSDestruction();
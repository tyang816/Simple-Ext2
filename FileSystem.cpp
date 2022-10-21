#define _CRT_SECURE_NO_WARNINGS

#include "FileSystem.h"
#include "DriverOperate.h"
#include "User.h"

//引用外部变量和全局变量声明
extern FCBIndex curFCBIndex;
extern User userOfBuff;
typedef FileControlBlock Inode; // fcb别名Inode
SuperBlock Super;
BitMap* FCBBitMap;
BitMap* DataBitMap;

/****位示图操作****/
//根据位示图判断块是否被使用
bool isBlockInUse(BitMap* map, uint32_t index) {
	uint16_t rowIndex = index / MAP_ROW_LEN; //位示图行数
	uint8_t colIndex = index % MAP_ROW_LEN; //
	uint8_t position = (1 << colIndex);
	return bool(map->MapData[rowIndex] & position);
}

//设置该位示图的某块已分配
void setBlockInUse(BitMap* map, uint32_t index) {
	uint16_t rowIndex = index / MAP_ROW_LEN;
	uint8_t colIndex = index % MAP_ROW_LEN;
	uint8_t position = 1 << colIndex;
	map->MapData[rowIndex] = map->MapData[rowIndex] | position;
}

//释放该位示图某块
void setBlockFree(BitMap* map, uint32_t index) {
	uint16_t rowIndex = index / MAP_ROW_LEN;
	uint8_t colIndex = index % MAP_ROW_LEN;
	uint8_t position = 1 << colIndex;
	map->MapData[rowIndex] = map->MapData[rowIndex] & ~position;
}

//释放位示图占用内存
bool freeBitMap(BitMap* map) {
	free(map);
	return true;
}

/****基本与数据块相关操作****/
//获取基本块大小
uint32_t& blockSize(uint8_t* blockBuff) {
	return ((Block*)blockBuff)->Size;
}

//获取空的数据块
BlockIndex getEmptyDataBlock() {
	for (uint16_t row = 0; row < (DataBitMap)->MapSizeOfByte; ++row) {
		if ((DataBitMap)->MapData[row] != 0xFF) {
			for (uint8_t col = 0; col < MAP_ROW_LEN; ++col) {
				uint32_t index = row * MAP_ROW_LEN + col;
				if (!isBlockInUse((DataBitMap), index) && index < (DataBitMap)->MapSizeOfBit) {
					return index;
				}
			}
		}
	}
	printf("数据已满，请删除一些文件。\n");
	return -1;
}

//将数据块加载入内存
void loadDataBlock(BlockIndex index, uint8_t* buff) {
	readDisk(buff, ((uint64_t)index + Super.DataStart) * Super.BlockSizeOfByte, Super.BlockSizeOfByte);
}

//将dataBlock写入磁盘
void writeDataBlock(BlockIndex index, uint8_t* buff) {
	writeDisk(buff, ((uint64_t)index + Super.DataStart) * Super.BlockSizeOfByte, Super.BlockSizeOfByte);
}

//创建文件块
void makeDirBlock(FCBIndex firstIndex, uint8_t* buff) {
	((Block*)buff)->Size = sizeof(FCBIndex);
	*(FCBIndex*)(buff + sizeof(Block)) = firstIndex;
	for (size_t i = sizeof(Block) + sizeof(FCBIndex); i < Super.BlockSizeOfByte; i += sizeof(FCBIndex)) {
		*(FCBIndex*)(buff + i) = -1;
	}
}

/****FCB块相关操作****/
//获取空的FCB块
FCBIndex getEmptyFCB() {
	for (uint16_t row = 0; row < (FCBBitMap)->MapSizeOfByte; ++row) {
		if ((FCBBitMap)->MapData[row] != 0xFF) {
			for (uint8_t col = 0; col < MAP_ROW_LEN; ++col) {
				uint32_t index = row * MAP_ROW_LEN + col;
				if (!isBlockInUse((FCBBitMap), index) && index < (FCBBitMap)->MapSizeOfBit) {
					return index;
				}
			}
		}
	}
	printf("FCB块已满，请删除一些文件。\n");
	return -1;
}

//将FCB块加载入内存
void loadFCB(FCBIndex index, FileControlBlock* buff) {
	if (index == -1) {
		printf("加载FCB块出错，请检查！\n");
		assert(index);
	}
	readDisk((uint8_t*)buff, (uint64_t)Super.FCBStart * Super.BlockSizeOfByte + (uint64_t)index * FILE_CONTROL_BLOCK_SIZE, FILE_CONTROL_BLOCK_SIZE);

	buff->ReadTime = time(0);
}

//将FCB块写入磁盘
void writeFCB(FCBIndex index, FileControlBlock* buff) {
	if (index == -1) {
		printf("存储FCB块出错，请检查！\n");
		assert(index);
	}
	writeDisk((uint8_t*)buff, (uint64_t)Super.FCBStart * Super.BlockSizeOfByte + (uint64_t)index * FILE_CONTROL_BLOCK_SIZE, FILE_CONTROL_BLOCK_SIZE);
}


/****文件指针读取相关操作****/
//加载BPR
void loadFPR(FPR* fpr, FCBIndex index) {
	fpr->Self_Index = index;
	loadFCB(index, &(fpr->FCB));
	for (int i = 0; i < 10; i++) fpr->BlockList.push_back(fpr->FCB.DirectBlock[i]);
	if (fpr->FCB.Pointer != -1) {
		loadDataBlock(fpr->FCB.Pointer, fpr->Pointer);
		for (auto iter = (BlockIndex*)(fpr->Pointer + sizeof(Block)); iter < (BlockIndex*)(fpr->Pointer + Super.BlockSizeOfByte); iter++) {
			fpr->BlockList.push_back(*iter);
		}
	}
}

//获取文件块的index
BlockIndex getBlockIndex(FPR* fpr, BlockIndex index) {
	if (index < 10) {
		return fpr->FCB.DirectBlock[index];
	}
	else if (fpr->FCB.Pointer == -1) {
		return -1;
	}
	else {
		return *((BlockIndex*)(fpr->FCB.Pointer + sizeof(Block)) + (index - 10));
	}
}

//增加新的块
BlockIndex addNewBlock(FPR* fpr, uint8_t* blockBuff) {
	BlockIndex dataBlockIndex = -1;
	for (int i = 0; i < 10; i++) {
		if (fpr->FCB.DirectBlock[i] == -1) {
			dataBlockIndex = getEmptyDataBlock();
			setBlockInUse((DataBitMap), dataBlockIndex);
			writeDataBlock(dataBlockIndex, blockBuff); //写入数据块
			fpr->FCB.DirectBlock[i] = dataBlockIndex;   //更新FCB
			writeDisk((DataBitMap)->MapData, Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //写入DataBlock的bitmap
			writeFCB(fpr->Self_Index, &(fpr->FCB));
			return dataBlockIndex;
		}
	}
	//前10个块没存下
	if (fpr->FCB.Pointer == -1) {
		fpr->Pointer = (uint8_t*)malloc(Super.BlockSizeOfByte);
		//制作空的Pointer
		for (BlockIndex* iter = (BlockIndex*)(fpr->Pointer + sizeof(Block)); iter < (BlockIndex*)(fpr->Pointer + Super.BlockSizeOfByte); ++iter) {
			*iter = -1;
		}
		//写入Pointer块
		dataBlockIndex = getEmptyDataBlock();
		setBlockInUse((DataBitMap), dataBlockIndex);
		writeDataBlock(dataBlockIndex, fpr->Pointer);
		//更新 FCB
		fpr->FCB.Pointer = dataBlockIndex;
	}
	else {
		loadDataBlock(fpr->FCB.Pointer, fpr->Pointer);
	}
	for (BlockIndex* iter = (BlockIndex*)(fpr->Pointer + sizeof(Block)); iter < (BlockIndex*)(fpr->Pointer + Super.BlockSizeOfByte); ++iter) {
		if (*iter == -1) {
			dataBlockIndex = getEmptyDataBlock();
			setBlockInUse((DataBitMap), dataBlockIndex);
			writeDataBlock(dataBlockIndex, blockBuff); //写入数据块
			*iter = dataBlockIndex;
			writeDataBlock(fpr->FCB.Pointer, fpr->Pointer); //更新pointer块
			// goto AppendNewBlock_end;
			writeDisk((DataBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //写入DataBlock的bitmap
			writeFCB(fpr->Self_Index, &(fpr->FCB));
			return dataBlockIndex;
		}
	}
	printf("错误：添加块失败，指针不可用！\n");
	return -1;
}

//将第index页读入内存
BlockIndex readPage(FPR* fpr, uint32_t index, uint8_t* blockBuff) {
	if (index < 10) {
		if (fpr->FCB.DirectBlock[index] != -1) {
			loadDataBlock(fpr->FCB.DirectBlock[index], blockBuff);
			return fpr->FCB.DirectBlock[index];
		}
		else {
			return -1;
		}
	}
	else {
		auto blockId = *((BlockIndex*)(fpr->Pointer + sizeof(Block)) + (index - 10));
		if (blockId != -1) {
			loadDataBlock(blockId, blockBuff);
			return blockId;
		}
		else {
			return -1;
		}
	}
}

//撤销FPR
bool freeBPR(FPR* fpr) {
	free(fpr);
	return true;
}



void printDir(FCBIndex dir) {
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);
	BlockIndex block = 0;
	FileControlBlock DirFCB, childs;
	loadFCB(dir, &DirFCB);
	for (size_t i = 0; i < 10 && DirFCB.DirectBlock[i] != -1; i++) {
		loadDataBlock(DirFCB.DirectBlock[i], blockBuff);
		for (FCBIndex* iter = (FCBIndex*)(blockBuff + sizeof(Block)); iter < (FCBIndex*)(blockBuff + Super.BlockSizeOfByte); iter++) {
			if (*iter == -1) {
				continue;
			}
			loadFCB(*iter, &childs);
			if (childs.Type == FileType::Directory) {
				printf("[%6d] %10s  %12s    @ ", *iter, childs.FileName, "<文件夹>");
			}
			else {
				printf("[%6d] %10s  %6d Bytes    @ ", *iter, childs.FileName, childs.Size);
			}
			for (int i = 0; i < 10 && childs.DirectBlock[i] != -1; i++) {
				printf("0x%X ", childs.DirectBlock[i]);
			}
			if (childs.Pointer != -1) {
				printf("...");
			}
			printf("\n");
		}
	}
	free(blockBuff);
}

//返回用户文件夹级目录名
string userDir(FCBIndex file) {
	string curPath;
	if (file == 0) curPath = "";
	else {
		FCBIndex tempFCBIndex = file;
		FileControlBlock fcb;
		while (tempFCBIndex != 0) {
			fileInfo(tempFCBIndex, &fcb);//首次依据当前FCB号，查找当前文件夹信息
			tempFCBIndex = fcb.Parent;
		}
		curPath = fcb.FileName;
	}
	return curPath;
}

void printInfo(FCBIndex file) {
	FileControlBlock fcb;
	loadFCB(file, &fcb);
	printf("文件名    : %s\n", fcb.FileName);
	printf("访问权限  : ");
	if (fcb.AccessMode & Access::Read) {
		printf("r");
	}
	if (fcb.AccessMode & Access::Write) {
		printf("w");
	}
	if (fcb.AccessMode & Access::Delete) {
		printf("d");
	}
	printf("\n");
	printf("组号      : %d\n", (fcb.AccessMode>>4));
	if (fcb.Type == FileType::Directory) {
		printf("类型      : 文件夹\n");
	}
	else {
		printf("类型      : 文件\n");
		printf("大小      : %d Bytes\n", fcb.Size);
	}
	printf("创建时间  : %s", ctime(&fcb.CreateTime));
	printf("读取时间  : %s", ctime(&fcb.ReadTime));
	printf("父节点    : %d\n", fcb.Parent);
	printf("物理地址  : ");
	FilePointerReader* fpr = new FilePointerReader();
	loadFPR(fpr, file);
	for (uint32_t i = 0; i < MAX_POINTER; ++i) {
		if (getBlockIndex(fpr,i) != -1) {
			printf("0x%X ", getBlockIndex(fpr, i));
		}
		else {
			break;
		}
	}
	printf("\n\n");
}

FCBIndex createDirectory(const string& name, FCBIndex dir, uint8_t access) {
	return create(name, dir, FileType::Directory, access<<4);
}

FCBIndex createFile(const string& name, FCBIndex dir, uint8_t access) {
	return create(name, dir, FileType::File, access<<4); 
}

FCBIndex find(FCBIndex dir, const string& filename) {
	FileControlBlock fcb, result;
	loadFCB(dir, &fcb);
	if (fcb.Type != FileType::Directory) {
		return -1;
	}
	else if (filename == "..") {
		return fcb.Parent;
	}
	else if (filename == ".") {
		return dir;
	}
	else {
		uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);
		for (size_t i = 0; i < 10; i++) {
			if (fcb.DirectBlock[i] != -1) {
				loadDataBlock(fcb.DirectBlock[i], blockBuff);
				for (size_t pos = sizeof(Block); pos < Super.BlockSizeOfByte; pos += sizeof(FCBIndex)) {
					FCBIndex fcbIndex = *(FCBIndex*)(blockBuff + pos);
					if (fcbIndex != -1) {
						loadFCB(fcbIndex, &result);
						if (strcmp(result.FileName, filename.c_str()) == 0) {
							free(blockBuff);
							return fcbIndex;
						}
					}
				}
			}
		}
		free(blockBuff);
		return -1;
	}
}

vector<FCBIndex> getChildren(FCBIndex dir) {
	vector<FCBIndex> ret;
	FileControlBlock DirFCB, childs;
	loadFCB(dir, &DirFCB);
	if (DirFCB.Type == FileType::File) {
		return ret;
	}
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);
	BlockIndex block = 0;

	for (size_t i = 0; i < 10 && DirFCB.DirectBlock[i] != -1; i++) {
		loadDataBlock(DirFCB.DirectBlock[i], blockBuff);
		for (FCBIndex* iter = (FCBIndex*)(blockBuff + sizeof(Block)); iter < (FCBIndex*)(blockBuff + Super.BlockSizeOfByte); iter++) {
			if (*iter == -1) {
				continue;
			}
			ret.push_back(*iter);
		}
	}
	free(blockBuff);
	return ret;
}
bool checkAccess(FCBIndex file,FileControlBlock fileFCB, uint8_t operate, uint8_t access){
	if ((userOfBuff.UserGroup != 0xff) && (strcmp(userOfBuff.UserName, userDir(file).c_str()) != 0))
		if (((access >> 4) != (fileFCB.AccessMode >> 4)) || ((fileFCB.AccessMode & operate) == false) || ((userOfBuff.UserGroup & operate) == false))
			return true;
	return false;
}

FCBIndex create(const string& name, FCBIndex dir, enum FileType t,uint8_t access) {
	if (name.length() > 30) {
		printf("文件名过长！\n");
		return -1;
	}
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);

	//创建新的FCB
	FileControlBlock dirFCB;
	loadFCB(dir, &dirFCB);
	if (checkAccess(dir,dirFCB,Access::Write,access)) {
		printf("错误：拒绝访问，该目录不可写入。\n");
		return -1;
	}

	FileControlBlock newFileFCB(t, name.c_str(),access+(Access::Read | Access::Write | Access::Delete), dir);
	newFileFCB.CreateTime = time(NULL);
	FCBIndex fcbIndex = getEmptyFCB();

	//检查重名并写入父目录(将fcbIndex写入父目录)

	FileControlBlock NameCheck;
	for (size_t page = 0; page < 10; page++) {

		if (dirFCB.DirectBlock[page] == -1) { //存入新的目录页
			//真正写入FCB
			setBlockInUse((FCBBitMap), fcbIndex);
			writeFCB(fcbIndex, &newFileFCB);

			makeDirBlock(fcbIndex, BlockBuff);
			BlockIndex newDirPage = getEmptyDataBlock();
			setBlockInUse((DataBitMap), newDirPage);
			dirFCB.DirectBlock[page] = newDirPage;
			writeFCB(dir, &dirFCB);
			writeDataBlock(newDirPage, BlockBuff);
			goto CreateFile_end;
		}
		else { //尝试加入现有目录页，顺便检查重名
			loadDataBlock(dirFCB.DirectBlock[page], BlockBuff);
			for (FCBIndex* iter = (FCBIndex*)(BlockBuff + sizeof(Block)); iter != (FCBIndex*)(BlockBuff + Super.BlockSizeOfByte); iter++) {
				if (*iter == -1) {
					//真正写入FCB
					setBlockInUse((FCBBitMap), fcbIndex);
					writeFCB(fcbIndex, &newFileFCB);
					*iter = fcbIndex;
					//BlockSize(BlockBuff) += sizeof(FCBIndex);
					writeDataBlock(dirFCB.DirectBlock[page], BlockBuff);
					goto CreateFile_end;
				}
				else {
					loadFCB(*iter, &NameCheck);
					if (strcmp(NameCheck.FileName, name.c_str()) == 0) {
						printf("错误：文件名重复，[%s]已经存在\n", NameCheck.FileName);
						free(BlockBuff);
						return -1;
					}
				}
			}

		}
	}

CreateFile_end:
	dirFCB.ModifyTime = time(NULL);
	dirFCB.ReadTime = time(NULL);

	writeDisk((FCBBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.FCBBitMapStart, (FCBBitMap)->MapSizeOfByte);    //写入FCB的bitmap
	writeDisk((DataBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //写入DataBlock的bitmap
	free(BlockBuff);
	return fcbIndex;
}

void fileInfo(FCBIndex file, FileControlBlock* fcb) { 
	loadFCB(file, fcb); 
}

int64_t readFile(FCBIndex file, int64_t pos, int64_t len, uint8_t* buff) {
	FileControlBlock ThisFileFCB;
	loadFCB(file, &ThisFileFCB);
	if (pos + len > ThisFileFCB.Size) {
		printf("错误：起始位置+长度为 %lld ，大于文件大小 %lu。\n", pos + len, ThisFileFCB.Size);
		return -1;
	}
	else if (pos < 0) {
		printf("错误：起始位置为 %lld ，应大于等于0。\n", pos);
		return -1;
	}
	else if (len < 0) {
		printf("错误：长度为 %lld ，应大于等于0。\n", len);
		return -1;
	}
	else if (checkAccess(file,ThisFileFCB, Access::Read, userOfBuff.UserGroup)) {
		printf("错误：拒绝访问，文件不可读。\n");
		return -1;
	}
	else if (ThisFileFCB.Type == FileType::Directory) {
		printf("错误：%s 是目录。\n", ThisFileFCB.FileName);
		return -1;
	}
	uint64_t inBlockPos = pos;
	uint64_t BytesToBeRead = len;
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);

	FilePointerReader fpr;
	loadFPR(&fpr, file);
	size_t PageNum;
	for (PageNum = 0; PageNum < MAX_POINTER; PageNum++) {
		BlockIndex block = readPage(&fpr, PageNum, blockBuff);

		if (inBlockPos < MAX_BLOCK_SPACE) { //从此块开始读
			goto ReadFile_StartToRead;
		}
		else { //读取起点还在后面
			inBlockPos -= blockSize(blockBuff);
		}
	}

ReadFile_StartToRead:
	//从文件的第PageNum块inBlockOffset字节开始，读取BytesToBeRead字节的数据
	for (; BytesToBeRead > 0; ++PageNum) {
		BlockIndex block = readPage(&fpr, PageNum, blockBuff);
		if (BytesToBeRead >= blockSize(blockBuff)) { //读完整块
			memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), blockSize(blockBuff));
			BytesToBeRead -= blockSize(blockBuff);
		}
		else { //读取一部分
			memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), BytesToBeRead);
			BytesToBeRead -= BytesToBeRead;
		}
	}

ReadFile_end:
	free(blockBuff);
	fpr.FCB.ReadTime = time(NULL);
	writeFCB(file, &fpr.FCB);
	return pos + len;
}

int64_t writeFile(FCBIndex file, int64_t pos, int64_t len, const uint8_t* buff) {
	FileControlBlock ThisFileFCB;
	loadFCB(file, &ThisFileFCB);
	if (pos > ThisFileFCB.Size) {
		printf("错误：写入位置 = %lld 大于文件大小 %lu.\n", pos, ThisFileFCB.Size);
		return -1;
	}
	else if (pos < 0) {
		printf("错误：写入位置 = %lld 应大于等于0.\n", pos);
		return -1;
	}
	else if (len < 0) {
		printf("错误：写入长度 = %lld 应大于等于0.\n", len);
		return -1;
	}
	else if (checkAccess(file,ThisFileFCB, Access::Write, userOfBuff.UserGroup)){
		printf("错误：拒绝访问，文件不可写.\n");
		return -1;
	}
	else if (ThisFileFCB.Type == FileType::Directory) {
		printf("错误：%s 是一个文件夹.\n", ThisFileFCB.FileName);
		return -1;
	}
	uint64_t inBlockPos = pos;
	uint64_t StartPos = 0;
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);

	FilePointerReader fpr;
	loadFPR(&fpr, file);

	(&fpr)->FCB.Size = MAX((&fpr)->FCB.Size, pos + len);

	for (size_t PageNum = 0; PageNum < MAX_POINTER; PageNum++) {
		BlockIndex block = readPage((&fpr), PageNum, blockBuff);
		if (block == -1) { //写入起点是新的块
			goto WriteFile_CreateNew;
		}
		else if (inBlockPos < MAX_BLOCK_SPACE) {     //写入起点是当前块
			if (inBlockPos + len <= MAX_BLOCK_SPACE) { //在空隙中存下
				memcpy(blockBuff + sizeof(Block) + inBlockPos, buff, len);
				blockSize(blockBuff) = MAX(blockSize(blockBuff), inBlockPos + len);
				writeDataBlock(block, blockBuff);
				goto WriteFile_end;
			}
			else { //存不下->填满空隙
				memcpy(blockBuff + sizeof(Block) + inBlockPos, buff, MAX_BLOCK_SPACE - inBlockPos);
				blockSize(blockBuff) = MAX_BLOCK_SPACE;
				writeDataBlock(block, blockBuff);
				StartPos += MAX_BLOCK_SPACE - inBlockPos; //写了MAX_BLOCK_SPACE - inBlockPos个字节
				goto WriteFile_CreateNew;
			}

		}
		else { //写入起点还在后面
			inBlockPos -= blockSize(blockBuff);
		}
	}

WriteFile_CreateNew:
	//将buff从StartPos ~ len之间的部分写入新块
	while (StartPos < len) {
		if (len - StartPos >= MAX_BLOCK_SPACE) { //写满本块
			blockSize(blockBuff) = MAX_BLOCK_SPACE;
			memcpy(blockBuff + sizeof(Block), buff + StartPos, MAX_BLOCK_SPACE);
			addNewBlock((&fpr), blockBuff);
			StartPos += MAX_BLOCK_SPACE;
		}
		else { //还剩空间
			blockSize(blockBuff) = len;
			memcpy(blockBuff + sizeof(Block), buff + StartPos, len - StartPos);
			addNewBlock((&fpr), blockBuff);
			StartPos += (len - StartPos);
		}
	}

WriteFile_end:
	free(blockBuff);
	(&fpr)->FCB.ModifyTime = time(NULL);
	writeFCB(file, &(&fpr)->FCB);
	return pos + len;
}

bool deleteFile(FCBIndex file) {
	FileControlBlock thisFile;
	loadFCB(file, &thisFile);
	if (checkAccess(file,thisFile, Access::Delete, userOfBuff.UserGroup)) {
		printf("错误：访问拒绝，该文件不可删！\n");
		return false;
	}
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);
	if (thisFile.Type == FileType::File) {
		vector<BlockIndex> blocks;
		for (int i = 0; i <= 10; ++i) {
			if (thisFile.DirectBlock[i] != -1) {
				blocks.push_back(thisFile.DirectBlock[i]);
			}
		}
		if (thisFile.Pointer != -1) {
			blocks.push_back(thisFile.Pointer);
			loadDataBlock(thisFile.Pointer, blockBuff);
			for (uint64_t pos = sizeof(Block); pos < Super.BlockSizeOfByte; pos += sizeof(BlockIndex)) {
				if (*(BlockIndex*)(blockBuff + pos) != -1) {
					blocks.push_back(*(BlockIndex*)(blockBuff + pos));
				}
			}
		}
		//删除文件数据块
		for (size_t i = 0; i < blocks.size(); i++) {
			setBlockFree((DataBitMap), blocks[i]);
		}
		//删除FCB
		setBlockFree((FCBBitMap), file);
	}
	else if (thisFile.Type == FileType::Directory) {
		for (int i = 0; i <= 10; ++i) {
			if (thisFile.DirectBlock[i] != -1) {
				loadDataBlock(thisFile.DirectBlock[i], blockBuff);
				for (uint64_t pos = sizeof(Block); pos < Super.BlockSizeOfByte; pos += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos) != -1) {
						deleteFile(*(FCBIndex*)(blockBuff + pos)); //删除文件夹内数据
					}
				}
				setBlockFree((DataBitMap), thisFile.DirectBlock[i]);//删除FCB
			}
		}
		setBlockFree((FCBBitMap), file);//删除自身FCB
	}
	//删除父目录的记录
	FCBIndex parentIndex = thisFile.Parent;
	FileControlBlock parentFCB;
	loadFCB(parentIndex, &parentFCB);
	for (size_t i = 0; i < 10 && parentFCB.DirectBlock[i] != -1; i++) {
		loadDataBlock(parentFCB.DirectBlock[i], blockBuff);
		for (FCBIndex* iter = (FCBIndex*)(blockBuff + sizeof(Block)); iter < (FCBIndex*)(blockBuff + Super.BlockSizeOfByte); iter++) {
			if (*iter == file) {
				*iter = -1;
				writeDataBlock(parentFCB.DirectBlock[i], blockBuff);
				// TEST ： 如果该目录块为空，则删除此目录块
				for (uint64_t pos2 = sizeof(Block); pos2 < Super.BlockSizeOfByte; pos2 += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos2) != -1) {
						goto DeleteFile_end;
					}
				}
				//删除空的目录表
				setBlockFree((DataBitMap), parentFCB.DirectBlock[i]);
				parentFCB.DirectBlock[i] = -1;
				// DirectBlock紧凑化
				for (int j = i + 1; j < 10; j++) {
					parentFCB.DirectBlock[j - 1] = parentFCB.DirectBlock[j];
				}
				parentFCB.DirectBlock[9] = -1;
				writeFCB(parentIndex, &parentFCB);
				goto DeleteFile_end;
			}
		}
	}

DeleteFile_end:
	writeDisk((FCBBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.FCBBitMapStart, (FCBBitMap)->MapSizeOfByte);    //写入FCB的bitmap
	writeDisk((DataBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //写入DataBlock的bitmap

	free(blockBuff);
	return true;
}

bool changeAccessMode(FCBIndex file, uint8_t newMode) {
	FileControlBlock fileFCB;
	loadFCB(file, &fileFCB);
	if (checkAccess(file, fileFCB, Access::Write, userOfBuff.UserGroup))
	{
		cout << "无权限更改文件权限" << endl;
		return false;
	}
	fileFCB.AccessMode = (fileFCB.AccessMode>>4<<4)+newMode;
	writeFCB(file, &fileFCB);
	return true;
}


bool FSInit() {
	initDisk("myFileSystem.disk");
	readDisk((uint8_t*)&Super, 0, SUPER_SIZE);
	if (strcmp(Super.Version, VERSION_STRING) == 0) {
		FCBBitMap = new BitMap(Super.FCBNum);        //初始化FCB的bitmap		
		DataBitMap = new BitMap(Super.DataBlockNum); //初始化Data的bitmap
		readDisk((FCBBitMap)->MapData, (uint64_t)Super.FCBBitMapStart * Super.BlockSizeOfByte, (FCBBitMap)->MapSizeOfByte);
		readDisk((DataBitMap)->MapData, (uint64_t)Super.DataBitMapStart * Super.BlockSizeOfByte, (DataBitMap)->MapSizeOfByte);
		return true;
	}
	else {
		printf("错误：读磁盘失败，请先初始化磁盘！\n");
		return false;
	}
}
bool FSDestruction() {
	closeDisk();
	return true;
}
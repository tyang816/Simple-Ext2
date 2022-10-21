#define _CRT_SECURE_NO_WARNINGS

#include "FileSystem.h"
#include "DriverOperate.h"
#include "User.h"

//�����ⲿ������ȫ�ֱ�������
extern FCBIndex curFCBIndex;
extern User userOfBuff;
typedef FileControlBlock Inode; // fcb����Inode
SuperBlock Super;
BitMap* FCBBitMap;
BitMap* DataBitMap;

/****λʾͼ����****/
//����λʾͼ�жϿ��Ƿ�ʹ��
bool isBlockInUse(BitMap* map, uint32_t index) {
	uint16_t rowIndex = index / MAP_ROW_LEN; //λʾͼ����
	uint8_t colIndex = index % MAP_ROW_LEN; //
	uint8_t position = (1 << colIndex);
	return bool(map->MapData[rowIndex] & position);
}

//���ø�λʾͼ��ĳ���ѷ���
void setBlockInUse(BitMap* map, uint32_t index) {
	uint16_t rowIndex = index / MAP_ROW_LEN;
	uint8_t colIndex = index % MAP_ROW_LEN;
	uint8_t position = 1 << colIndex;
	map->MapData[rowIndex] = map->MapData[rowIndex] | position;
}

//�ͷŸ�λʾͼĳ��
void setBlockFree(BitMap* map, uint32_t index) {
	uint16_t rowIndex = index / MAP_ROW_LEN;
	uint8_t colIndex = index % MAP_ROW_LEN;
	uint8_t position = 1 << colIndex;
	map->MapData[rowIndex] = map->MapData[rowIndex] & ~position;
}

//�ͷ�λʾͼռ���ڴ�
bool freeBitMap(BitMap* map) {
	free(map);
	return true;
}

/****���������ݿ���ز���****/
//��ȡ�������С
uint32_t& blockSize(uint8_t* blockBuff) {
	return ((Block*)blockBuff)->Size;
}

//��ȡ�յ����ݿ�
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
	printf("������������ɾ��һЩ�ļ���\n");
	return -1;
}

//�����ݿ�������ڴ�
void loadDataBlock(BlockIndex index, uint8_t* buff) {
	readDisk(buff, ((uint64_t)index + Super.DataStart) * Super.BlockSizeOfByte, Super.BlockSizeOfByte);
}

//��dataBlockд�����
void writeDataBlock(BlockIndex index, uint8_t* buff) {
	writeDisk(buff, ((uint64_t)index + Super.DataStart) * Super.BlockSizeOfByte, Super.BlockSizeOfByte);
}

//�����ļ���
void makeDirBlock(FCBIndex firstIndex, uint8_t* buff) {
	((Block*)buff)->Size = sizeof(FCBIndex);
	*(FCBIndex*)(buff + sizeof(Block)) = firstIndex;
	for (size_t i = sizeof(Block) + sizeof(FCBIndex); i < Super.BlockSizeOfByte; i += sizeof(FCBIndex)) {
		*(FCBIndex*)(buff + i) = -1;
	}
}

/****FCB����ز���****/
//��ȡ�յ�FCB��
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
	printf("FCB����������ɾ��һЩ�ļ���\n");
	return -1;
}

//��FCB��������ڴ�
void loadFCB(FCBIndex index, FileControlBlock* buff) {
	if (index == -1) {
		printf("����FCB��������飡\n");
		assert(index);
	}
	readDisk((uint8_t*)buff, (uint64_t)Super.FCBStart * Super.BlockSizeOfByte + (uint64_t)index * FILE_CONTROL_BLOCK_SIZE, FILE_CONTROL_BLOCK_SIZE);

	buff->ReadTime = time(0);
}

//��FCB��д�����
void writeFCB(FCBIndex index, FileControlBlock* buff) {
	if (index == -1) {
		printf("�洢FCB��������飡\n");
		assert(index);
	}
	writeDisk((uint8_t*)buff, (uint64_t)Super.FCBStart * Super.BlockSizeOfByte + (uint64_t)index * FILE_CONTROL_BLOCK_SIZE, FILE_CONTROL_BLOCK_SIZE);
}


/****�ļ�ָ���ȡ��ز���****/
//����BPR
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

//��ȡ�ļ����index
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

//�����µĿ�
BlockIndex addNewBlock(FPR* fpr, uint8_t* blockBuff) {
	BlockIndex dataBlockIndex = -1;
	for (int i = 0; i < 10; i++) {
		if (fpr->FCB.DirectBlock[i] == -1) {
			dataBlockIndex = getEmptyDataBlock();
			setBlockInUse((DataBitMap), dataBlockIndex);
			writeDataBlock(dataBlockIndex, blockBuff); //д�����ݿ�
			fpr->FCB.DirectBlock[i] = dataBlockIndex;   //����FCB
			writeDisk((DataBitMap)->MapData, Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //д��DataBlock��bitmap
			writeFCB(fpr->Self_Index, &(fpr->FCB));
			return dataBlockIndex;
		}
	}
	//ǰ10����û����
	if (fpr->FCB.Pointer == -1) {
		fpr->Pointer = (uint8_t*)malloc(Super.BlockSizeOfByte);
		//�����յ�Pointer
		for (BlockIndex* iter = (BlockIndex*)(fpr->Pointer + sizeof(Block)); iter < (BlockIndex*)(fpr->Pointer + Super.BlockSizeOfByte); ++iter) {
			*iter = -1;
		}
		//д��Pointer��
		dataBlockIndex = getEmptyDataBlock();
		setBlockInUse((DataBitMap), dataBlockIndex);
		writeDataBlock(dataBlockIndex, fpr->Pointer);
		//���� FCB
		fpr->FCB.Pointer = dataBlockIndex;
	}
	else {
		loadDataBlock(fpr->FCB.Pointer, fpr->Pointer);
	}
	for (BlockIndex* iter = (BlockIndex*)(fpr->Pointer + sizeof(Block)); iter < (BlockIndex*)(fpr->Pointer + Super.BlockSizeOfByte); ++iter) {
		if (*iter == -1) {
			dataBlockIndex = getEmptyDataBlock();
			setBlockInUse((DataBitMap), dataBlockIndex);
			writeDataBlock(dataBlockIndex, blockBuff); //д�����ݿ�
			*iter = dataBlockIndex;
			writeDataBlock(fpr->FCB.Pointer, fpr->Pointer); //����pointer��
			// goto AppendNewBlock_end;
			writeDisk((DataBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //д��DataBlock��bitmap
			writeFCB(fpr->Self_Index, &(fpr->FCB));
			return dataBlockIndex;
		}
	}
	printf("������ӿ�ʧ�ܣ�ָ�벻���ã�\n");
	return -1;
}

//����indexҳ�����ڴ�
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

//����FPR
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
				printf("[%6d] %10s  %12s    @ ", *iter, childs.FileName, "<�ļ���>");
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

//�����û��ļ��м�Ŀ¼��
string userDir(FCBIndex file) {
	string curPath;
	if (file == 0) curPath = "";
	else {
		FCBIndex tempFCBIndex = file;
		FileControlBlock fcb;
		while (tempFCBIndex != 0) {
			fileInfo(tempFCBIndex, &fcb);//�״����ݵ�ǰFCB�ţ����ҵ�ǰ�ļ�����Ϣ
			tempFCBIndex = fcb.Parent;
		}
		curPath = fcb.FileName;
	}
	return curPath;
}

void printInfo(FCBIndex file) {
	FileControlBlock fcb;
	loadFCB(file, &fcb);
	printf("�ļ���    : %s\n", fcb.FileName);
	printf("����Ȩ��  : ");
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
	printf("���      : %d\n", (fcb.AccessMode>>4));
	if (fcb.Type == FileType::Directory) {
		printf("����      : �ļ���\n");
	}
	else {
		printf("����      : �ļ�\n");
		printf("��С      : %d Bytes\n", fcb.Size);
	}
	printf("����ʱ��  : %s", ctime(&fcb.CreateTime));
	printf("��ȡʱ��  : %s", ctime(&fcb.ReadTime));
	printf("���ڵ�    : %d\n", fcb.Parent);
	printf("�����ַ  : ");
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
		printf("�ļ���������\n");
		return -1;
	}
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSizeOfByte);

	//�����µ�FCB
	FileControlBlock dirFCB;
	loadFCB(dir, &dirFCB);
	if (checkAccess(dir,dirFCB,Access::Write,access)) {
		printf("���󣺾ܾ����ʣ���Ŀ¼����д�롣\n");
		return -1;
	}

	FileControlBlock newFileFCB(t, name.c_str(),access+(Access::Read | Access::Write | Access::Delete), dir);
	newFileFCB.CreateTime = time(NULL);
	FCBIndex fcbIndex = getEmptyFCB();

	//���������д�븸Ŀ¼(��fcbIndexд�븸Ŀ¼)

	FileControlBlock NameCheck;
	for (size_t page = 0; page < 10; page++) {

		if (dirFCB.DirectBlock[page] == -1) { //�����µ�Ŀ¼ҳ
			//����д��FCB
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
		else { //���Լ�������Ŀ¼ҳ��˳��������
			loadDataBlock(dirFCB.DirectBlock[page], BlockBuff);
			for (FCBIndex* iter = (FCBIndex*)(BlockBuff + sizeof(Block)); iter != (FCBIndex*)(BlockBuff + Super.BlockSizeOfByte); iter++) {
				if (*iter == -1) {
					//����д��FCB
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
						printf("�����ļ����ظ���[%s]�Ѿ�����\n", NameCheck.FileName);
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

	writeDisk((FCBBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.FCBBitMapStart, (FCBBitMap)->MapSizeOfByte);    //д��FCB��bitmap
	writeDisk((DataBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //д��DataBlock��bitmap
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
		printf("������ʼλ��+����Ϊ %lld �������ļ���С %lu��\n", pos + len, ThisFileFCB.Size);
		return -1;
	}
	else if (pos < 0) {
		printf("������ʼλ��Ϊ %lld ��Ӧ���ڵ���0��\n", pos);
		return -1;
	}
	else if (len < 0) {
		printf("���󣺳���Ϊ %lld ��Ӧ���ڵ���0��\n", len);
		return -1;
	}
	else if (checkAccess(file,ThisFileFCB, Access::Read, userOfBuff.UserGroup)) {
		printf("���󣺾ܾ����ʣ��ļ����ɶ���\n");
		return -1;
	}
	else if (ThisFileFCB.Type == FileType::Directory) {
		printf("����%s ��Ŀ¼��\n", ThisFileFCB.FileName);
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

		if (inBlockPos < MAX_BLOCK_SPACE) { //�Ӵ˿鿪ʼ��
			goto ReadFile_StartToRead;
		}
		else { //��ȡ��㻹�ں���
			inBlockPos -= blockSize(blockBuff);
		}
	}

ReadFile_StartToRead:
	//���ļ��ĵ�PageNum��inBlockOffset�ֽڿ�ʼ����ȡBytesToBeRead�ֽڵ�����
	for (; BytesToBeRead > 0; ++PageNum) {
		BlockIndex block = readPage(&fpr, PageNum, blockBuff);
		if (BytesToBeRead >= blockSize(blockBuff)) { //��������
			memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), blockSize(blockBuff));
			BytesToBeRead -= blockSize(blockBuff);
		}
		else { //��ȡһ����
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
		printf("����д��λ�� = %lld �����ļ���С %lu.\n", pos, ThisFileFCB.Size);
		return -1;
	}
	else if (pos < 0) {
		printf("����д��λ�� = %lld Ӧ���ڵ���0.\n", pos);
		return -1;
	}
	else if (len < 0) {
		printf("����д�볤�� = %lld Ӧ���ڵ���0.\n", len);
		return -1;
	}
	else if (checkAccess(file,ThisFileFCB, Access::Write, userOfBuff.UserGroup)){
		printf("���󣺾ܾ����ʣ��ļ�����д.\n");
		return -1;
	}
	else if (ThisFileFCB.Type == FileType::Directory) {
		printf("����%s ��һ���ļ���.\n", ThisFileFCB.FileName);
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
		if (block == -1) { //д��������µĿ�
			goto WriteFile_CreateNew;
		}
		else if (inBlockPos < MAX_BLOCK_SPACE) {     //д������ǵ�ǰ��
			if (inBlockPos + len <= MAX_BLOCK_SPACE) { //�ڿ�϶�д���
				memcpy(blockBuff + sizeof(Block) + inBlockPos, buff, len);
				blockSize(blockBuff) = MAX(blockSize(blockBuff), inBlockPos + len);
				writeDataBlock(block, blockBuff);
				goto WriteFile_end;
			}
			else { //�治��->������϶
				memcpy(blockBuff + sizeof(Block) + inBlockPos, buff, MAX_BLOCK_SPACE - inBlockPos);
				blockSize(blockBuff) = MAX_BLOCK_SPACE;
				writeDataBlock(block, blockBuff);
				StartPos += MAX_BLOCK_SPACE - inBlockPos; //д��MAX_BLOCK_SPACE - inBlockPos���ֽ�
				goto WriteFile_CreateNew;
			}

		}
		else { //д����㻹�ں���
			inBlockPos -= blockSize(blockBuff);
		}
	}

WriteFile_CreateNew:
	//��buff��StartPos ~ len֮��Ĳ���д���¿�
	while (StartPos < len) {
		if (len - StartPos >= MAX_BLOCK_SPACE) { //д������
			blockSize(blockBuff) = MAX_BLOCK_SPACE;
			memcpy(blockBuff + sizeof(Block), buff + StartPos, MAX_BLOCK_SPACE);
			addNewBlock((&fpr), blockBuff);
			StartPos += MAX_BLOCK_SPACE;
		}
		else { //��ʣ�ռ�
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
		printf("���󣺷��ʾܾ������ļ�����ɾ��\n");
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
		//ɾ���ļ����ݿ�
		for (size_t i = 0; i < blocks.size(); i++) {
			setBlockFree((DataBitMap), blocks[i]);
		}
		//ɾ��FCB
		setBlockFree((FCBBitMap), file);
	}
	else if (thisFile.Type == FileType::Directory) {
		for (int i = 0; i <= 10; ++i) {
			if (thisFile.DirectBlock[i] != -1) {
				loadDataBlock(thisFile.DirectBlock[i], blockBuff);
				for (uint64_t pos = sizeof(Block); pos < Super.BlockSizeOfByte; pos += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos) != -1) {
						deleteFile(*(FCBIndex*)(blockBuff + pos)); //ɾ���ļ���������
					}
				}
				setBlockFree((DataBitMap), thisFile.DirectBlock[i]);//ɾ��FCB
			}
		}
		setBlockFree((FCBBitMap), file);//ɾ������FCB
	}
	//ɾ����Ŀ¼�ļ�¼
	FCBIndex parentIndex = thisFile.Parent;
	FileControlBlock parentFCB;
	loadFCB(parentIndex, &parentFCB);
	for (size_t i = 0; i < 10 && parentFCB.DirectBlock[i] != -1; i++) {
		loadDataBlock(parentFCB.DirectBlock[i], blockBuff);
		for (FCBIndex* iter = (FCBIndex*)(blockBuff + sizeof(Block)); iter < (FCBIndex*)(blockBuff + Super.BlockSizeOfByte); iter++) {
			if (*iter == file) {
				*iter = -1;
				writeDataBlock(parentFCB.DirectBlock[i], blockBuff);
				// TEST �� �����Ŀ¼��Ϊ�գ���ɾ����Ŀ¼��
				for (uint64_t pos2 = sizeof(Block); pos2 < Super.BlockSizeOfByte; pos2 += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos2) != -1) {
						goto DeleteFile_end;
					}
				}
				//ɾ���յ�Ŀ¼��
				setBlockFree((DataBitMap), parentFCB.DirectBlock[i]);
				parentFCB.DirectBlock[i] = -1;
				// DirectBlock���ջ�
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
	writeDisk((FCBBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.FCBBitMapStart, (FCBBitMap)->MapSizeOfByte);    //д��FCB��bitmap
	writeDisk((DataBitMap)->MapData, (uint64_t)Super.BlockSizeOfByte * Super.DataBitMapStart, (DataBitMap)->MapSizeOfByte); //д��DataBlock��bitmap

	free(blockBuff);
	return true;
}

bool changeAccessMode(FCBIndex file, uint8_t newMode) {
	FileControlBlock fileFCB;
	loadFCB(file, &fileFCB);
	if (checkAccess(file, fileFCB, Access::Write, userOfBuff.UserGroup))
	{
		cout << "��Ȩ�޸����ļ�Ȩ��" << endl;
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
		FCBBitMap = new BitMap(Super.FCBNum);        //��ʼ��FCB��bitmap		
		DataBitMap = new BitMap(Super.DataBlockNum); //��ʼ��Data��bitmap
		readDisk((FCBBitMap)->MapData, (uint64_t)Super.FCBBitMapStart * Super.BlockSizeOfByte, (FCBBitMap)->MapSizeOfByte);
		readDisk((DataBitMap)->MapData, (uint64_t)Super.DataBitMapStart * Super.BlockSizeOfByte, (DataBitMap)->MapSizeOfByte);
		return true;
	}
	else {
		printf("���󣺶�����ʧ�ܣ����ȳ�ʼ�����̣�\n");
		return false;
	}
}
bool FSDestruction() {
	closeDisk();
	return true;
}
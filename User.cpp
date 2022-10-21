#include "User.h"
#include "DriverOperate.h"
#include "FileSystem.h"

User userOfBuff;
extern SuperBlock Super;
extern BitMap DataBitMap;
extern BitMap FCBBitMap;

//����û�
bool addUser(const char* userName,const char* password, uint8_t userGroup) {
	assert(Super.UserNum >= 0 && Super.UserNum <= 30);
	User newUser(userName,password,userGroup);
	writeDisk((uint8_t*)&newUser, SUPER_SIZE + Super.UserNum * USER_LEN, USER_LEN);
	Super.UserNum++;
	writeDisk((uint8_t*)&Super, 0, SUPER_SIZE);
	return true;
}

//�ҵ��û������ڵ�index
int16_t findUserName(const char* userName) {
	User tempUser;
	for (uint16_t index = 0; index < Super.UserNum; ++index) {
		readDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
		if (strcmp(tempUser.UserName, userName) == 0) {
			return index;
		}
	}
	return -1;
}

//��ӡ�û���Ϣ
void printUserInfo(uint16_t index) {
	User tempUser;
	readDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
	printf("�û���     :%s\n", tempUser.UserName);
	printf("����       :%s\n", tempUser.Password);
	printf("�û���     :%d\n", tempUser.UserGroup >> 4);
	printf("Ȩ��       :");
	if (tempUser.UserGroup & Access::Read) {
		printf("r");
	}
	if (tempUser.UserGroup & Access::Write) {
		printf("w");
	}
	if (tempUser.UserGroup & Access::Delete) {
		printf("d");
	}
	printf("\n\n");
}

//�ı��û�Ȩ��
bool changeUserAccess(uint16_t index,uint8_t access) {
	if (userOfBuff.UserGroup != 0xff)
	{
		cout << "��Ȩ�޸����û�Ȩ��" << endl;
		return false;
	}
	User tempUser;
	readDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
	tempUser.UserGroup = access;
	writeDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
	return true;
}

//У���û�
bool checkUser(User* user) {
	User tempUser;
	int16_t index = findUserName(user->UserName);
	if (index != -1) {
		readDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
		if (strcmp(tempUser.Password, user->Password) == 0) {
			return true;
		}
		else {
			printf("�û�����ȷ�����������\n");
		}
	}
	else {
		printf("�û������ڣ�\n");
	}
	return false;
}

//��ʽ�������û�
bool formatUser() {
	uint8_t empty[30];
	writeDisk(empty, SUPER_SIZE, 30);
	return true;
}
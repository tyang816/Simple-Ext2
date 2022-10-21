#include "User.h"
#include "DriverOperate.h"
#include "FileSystem.h"

User userOfBuff;
extern SuperBlock Super;
extern BitMap DataBitMap;
extern BitMap FCBBitMap;

//添加用户
bool addUser(const char* userName,const char* password, uint8_t userGroup) {
	assert(Super.UserNum >= 0 && Super.UserNum <= 30);
	User newUser(userName,password,userGroup);
	writeDisk((uint8_t*)&newUser, SUPER_SIZE + Super.UserNum * USER_LEN, USER_LEN);
	Super.UserNum++;
	writeDisk((uint8_t*)&Super, 0, SUPER_SIZE);
	return true;
}

//找到用户名所在的index
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

//打印用户信息
void printUserInfo(uint16_t index) {
	User tempUser;
	readDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
	printf("用户名     :%s\n", tempUser.UserName);
	printf("密码       :%s\n", tempUser.Password);
	printf("用户组     :%d\n", tempUser.UserGroup >> 4);
	printf("权限       :");
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

//改变用户权限
bool changeUserAccess(uint16_t index,uint8_t access) {
	if (userOfBuff.UserGroup != 0xff)
	{
		cout << "无权限更改用户权限" << endl;
		return false;
	}
	User tempUser;
	readDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
	tempUser.UserGroup = access;
	writeDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
	return true;
}

//校验用户
bool checkUser(User* user) {
	User tempUser;
	int16_t index = findUserName(user->UserName);
	if (index != -1) {
		readDisk((uint8_t*)&tempUser, SUPER_SIZE + index * USER_LEN, USER_LEN);
		if (strcmp(tempUser.Password, user->Password) == 0) {
			return true;
		}
		else {
			printf("用户名正确，但密码错误！\n");
		}
	}
	else {
		printf("用户不存在！\n");
	}
	return false;
}

//格式化磁盘用户
bool formatUser() {
	uint8_t empty[30];
	writeDisk(empty, SUPER_SIZE, 30);
	return true;
}
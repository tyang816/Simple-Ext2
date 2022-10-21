#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <bits/stdc++.h>
#define USER_LEN sizeof(User)

/****定义用户****/
typedef struct User {
	char UserName[16]; // 用户名
	char Password[15];  // 密码
	uint8_t UserGroup; //用户所在组
	
	User() {};
	User(const char* userName, const char* password, uint8_t userGroup) {
		strcpy(UserName, userName);
		strcpy(Password, password);
		this->UserGroup = userGroup;
	};
}User;

//用户相关操作函数声明
bool addUser(const char* userName, const char* password, uint8_t userGroup);
bool checkUser(User* user);
bool changeUserAccess(uint16_t index, uint8_t access);
int16_t findUserName(const char* userName);
bool formatUser();
void printUserInfo(uint16_t index);
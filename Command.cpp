/*
    命令文件，主要包含类linux的常用命令的实现
*/
#define _CRT_SECURE_NO_WARNINGS
#include "Command.h"
#include "CommandUtil.h"
#include "User.h"
#include "FileSystem.h"
#include <bits/stdc++.h>
#include <conio.h>


string cmdStr, paraStr;//分别存储命令与参数

FCBIndex curFCBIndex;   //当前文件夹FCB号
char writeBuff[10 * BLOCKSIZE];  //退出时写入缓冲区                                                                                                                              // 文件系统名称
FILE* UserFile;            // 打开文件指针

extern User userOfBuff;// 暂存登录用户


//命令定义，一共19个linux命令
vector<string> Commands = { "help", "cd", "ll", "mkdir", "touch",
"cat", "vim", "rm", "su", "clear", "format", "exit", "rmdir",
"info", "register","fi","ui","chmod","chmodu" }; // 19个

//功能：登录系统
void login() {
    User user;
    char* word;
    bool flag = 0;
    do {
        printf("用户名：");
        gets_s(user.UserName);//用户输入用户名，回显
        if (strcmp(user.UserName, "") == 0) {
            printf("用户名不能为空！\n");
            flag = 1;
            break;
        }
        printf("密码：");
        word = user.Password;
        while (*word = _getch()) {  //输入密码，不回显
            if (*word == 0x0d) {  //当输入回车键时，0x0d为回车键的ASCII码
                *word = '\0'; //将输入的回车键转换成空格
                break;
            }
            printf("*"); //将输入的密码以"*"号显示
            word++;
        }
        if (strcmp(user.Password, "") == 0) {
            printf("抱歉，密码不能为空！\n");
            flag = 1;
            break;
        }
        printf("\n");
    } while (!checkUser(&user));
    if (!flag) {
        readDisk((uint8_t*)&userOfBuff, SUPER_SIZE + findUserName(user.UserName) * USER_LEN, USER_LEN);
        if (strcmp(user.UserName, "root") != 0) {
            cd(user.UserName);//进入用户文件夹
            return; //登陆成功，直接跳出登陆函数
        }
    }
}

//功能：注册新用户
void reg() {
    User user;
    char* word;
    if (userOfBuff.UserGroup != 0xff) {
        printf("非root用户，无权限注册新用户！\n");
        return;
    }
    printf("用户名：");
    gets_s(user.UserName);//用户输入用户名，回显
    if (strcmp(user.UserName, "") == 0) {
        printf("用户名不能为空！\n");
        return;
    }
    if (findUserName(user.UserName) != -1) {
        printf("抱歉，用户已存在！\n");
        return;
    }
    printf("密码：");
    word = user.Password;
    while (*word = _getch()) {  //输入密码，不回显
        if (*word == 0x0d) {  //当输入回车键时，0x0d为回车键的ASCII码
            *word = '\0'; //将输入的回车键转换成空格
            break;
        }
        printf("*"); //将输入的密码以"*"号显示
        word++;
    }
    if (strcmp(user.Password, "") == 0) {
        printf("抱歉，密码不能为空！\n");
        return;
    }
    printf("\n");
    printf("设置用户组号（[0,14]）：");
    int group;
    scanf("%d", &group);
    if (group > 14 || group < 0) {
        cout << "组号错误！" << endl;
        return;
    }
    getchar();//scanf后，这里需要吸收一个回车
    if (addUser(user.UserName, user.Password, ((group << 4) + 15))) {
        printf("用户[%s]创建成功！\n", user.UserName);
        createDirectory(user.UserName, 0, group);//在根目录下创建用户文件夹，仅供该用户使用
        return;
    }
    else if (sizeof(user.UserName) > 16) {
        printf("抱歉，用户名过长！\n");
        return;
    }
    else if (sizeof(user.Password) > 15) {
        printf("抱歉，密码过长！\n");
        return;
    }
}

//功能: 切换登录用户
void su() {
    User user;
    char* word;
    printf("用户名：");
    gets_s(user.UserName);//用户输入用户名，回显
    if (strcmp(user.UserName, "") == 0) {
        printf("用户名不能为空！\n");
        return;
    }
    if (findUserName(user.UserName) == -1) {
        printf("抱歉，用户不存在！\n");
        return;
    }
    printf("密码：");
    word = user.Password;
    while (*word = _getch()) {
        if (*word == 0x0d) { 		//当输入回车键时，0x0d为回车键的ASCII码
            *word = '\0'; 			//将输入的回车键转换成空格
            break;
        }
        printf("*");   //将输入的密码以"*"号显示
        word++;
    }
    if (strcmp(user.Password, "") == 0) {
        printf("抱歉，密码不能为空！\n");
        return;
    }
    printf("\n");
    if (checkUser(&user)) {
        init();//将inum_cur初始化为0，回到根目录
        readDisk((uint8_t*)&userOfBuff, SUPER_SIZE + findUserName(user.UserName) * USER_LEN, USER_LEN);
        if (strcmp(user.UserName, "root") != 0) cd(user.UserName);//进入用户文件夹
        return;     //登陆成功，直接跳出登陆函数
    }
}


//功能: 显示帮助命令
void help() {
    printf("命令帮助: \n\
    help      ---       显示帮助菜单 \n\
    cd        ---       改变文件夹 \n\
    clear     ---       清屏 \n\
    ll        ---       显示在当前目录下所有文件及文件夹 \n\
    mkdir     ---       创建新文件夹   \n\
    touch     ---       创建新文件 \n\
    cat       ---       打开并读取已存在文件 \n\
    vim       ---       打开已存在文件并往里写入 \n\
    rm        ---       删除已存在文件 \n\
    su        ---       切换当前用户 \n\
    format    ---       格式化当前文件系统 \n\
    exit      ---       退出系统 \n\
    rmdir     ---       删除文件夹 \n\
    info      ---       显示整个系统信息 \n\
    register  ---       注册新用户\n\
    fi        ---       显示文件信息\n\
    ui        ---       显示用户信息\n\
    chmod     ---       更改文件权限\n\
    chmodu    ---       更改用户权限\n ");
}

// 功能: 切换目录(cd .. 或者 cd dir1/dir2)
void cd(string path) {
    int temp_cur;
    if (path.empty()) {
        printf("没有输入路径！\n");
        return;
    }
    else {
        if (path.back() != '/')
            path += '/';
        temp_cur = readby(path);
    }
    if (temp_cur != -1) curFCBIndex = temp_cur;
    else printf("没有该目录！\n");
}

//功能：显示当前文件下所有文件
void ll(string path) { // path为空则列出当前文件夹下的全部子文件，不为空则列出path路径下的子文件
    int temp_cur;
    if (path.empty()) temp_cur = curFCBIndex;
    else
    {
        if (path.back() != '/')
            path += '/';
        temp_cur = readby(path);
        if (temp_cur == -1) {
            cout << "没有该文件夹！\n" << endl;
            return;
        }
    }
    if (temp_cur != -1) {
        printf(" %6s | %8s | %12s | %10s\n", "FCB", "文件名", "大小", "物理路径");
        printDir(temp_cur);
    }
    cout << endl;
}

//cmd创建文件函数，在当前目录下创建文件夹
void mkdir() {
    if (paraStr.empty()) {
        printf("请输入文件夹名\n");
        return;
    }
    int temp_cur;
    string temps1, temps2;
    if (paraStr.find('/') != -1) {  // 要创建的文件夹不在当前目录下，而是在路径中的指定文件下
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        temp_cur = readby(temps1);
        if (temp_cur == -1) {
            printf("没有该文件夹！\n");
        }
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex index = createDirectory(temps2, temp_cur, userOfBuff.UserGroup >> 4);
    if (index != -1) {
        printf("创建文件夹成功！\n");
    }
    else {
        printf("创建文件夹失败！\n");
    }
}

// 功能: 在当前目录下创建文件(creat file1)
void touch()
{
    if (paraStr.length() == 0) {
        printf("请输入文件名！\n");
        return;
    }
    int temp_cur;
    string temps1, temps2;
    if (paraStr.find('/') != -1) {  // 要创建的file不在当前目录下，而是在路径中的指定文件下
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);//temps2是文件名；+1吧'/'去掉
        temp_cur = readby(temps1);
        if (temp_cur == -1) {
            printf("没有该文件夹！\n");
        }
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex index = createFile(temps2, temp_cur, userOfBuff.UserGroup >> 4);
    if (index != -1) {
        printf("创建文件成功！\n");
    }
    else {
        printf("创建文件失败！\n");
    }
}

// an exist file
void cat() {
    string temps1, temps2; int temp_cur;
    if (paraStr.empty()) {
        cout << "文件不为空！" << endl;
        return;
    }
    if (paraStr.find('/') != -1) {  // 传入的不是文件名而是路径
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        temp_cur = readby(temps1);
    }
    else {
        temps2 = paraStr;//将文件名缓存到temps2中
        temp_cur = curFCBIndex;
    }
    FCBIndex file_cur = find(temp_cur, temps2);
    if (file_cur == -1) {//如果未找到该文件，则返回
        cout << "文件不存在！" << endl;
        return;
    }
    FileControlBlock fcb;
    fileInfo(file_cur, &fcb);
    uint8_t* buff = (uint8_t*)malloc(fcb.Size + 1);//多加一个字节，增加‘\0’
    int64_t res = readFile(file_cur, 0, fcb.Size, buff);//指针类型传指针类型参数不用加&
    buff[fcb.Size] = '\0';//
    if (res != -1) {
        printf("%s\n", buff);
    }
    else {
        printf("读入失败！\n");
    }
    free(buff);
}

// open and write something to a particular file
void vim() {
    int64_t res;
    string temps1, temps2; int temp_cur;
    char temp[5 * BLOCKSIZE];
    //uint8_t* temp;
    if (paraStr.find('/') != -1) {  // 传入的不是文件名而是路径
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        //string temps = s2;
        //s2 = temps2;
        temp_cur = readby(temps1);
        //s2 = temps;
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex file_cur = find(temp_cur, temps2);//根据当前文件夹号，和文件名查找文件FCB号
    if (file_cur == -1) {//如果未找到该文件，则返回
        cout << "文件不存在！" << endl;
        return;
    }
    FileControlBlock fcb;
    fileInfo(file_cur, &fcb);//根据查找到的FCB号获得文件FCB
    if (fcb.Size == 0) {
        printf("请输入： ");//这里不要加换行符
        gets_s(temp);
        res = writeFile(file_cur, 0, strlen((char*)(temp)), (uint8_t*)(temp));
    }
    else {
        char choice;
        printf("文件已有数据 \n");
        printf("覆盖或追加？(o/a):");
        scanf("%c", &choice);
        getchar();//scanf后需要加这句，吸收回车键，否则gets_s会将回车直接存到后面的temp缓存中
        if (choice == 'o') {
            printf("请输入： ");//这里不要加换行符
            gets_s(temp);
            res = writeFile(file_cur, 0, strlen((char*)(temp)) + 1, (uint8_t*)(temp));//将temp最后的'\0'存入完成覆盖
        }
        else if (choice == 'a') {
            printf("请输入： ");//这里不要加换行符
            gets_s(temp);
            res = writeFile(file_cur, fcb.Size, strlen((char*)(temp)), (uint8_t*)(temp));//fcb.Size不需要+1，从0开始


        }
        else {
            printf("退出写入！\n");
        }
    }
    if (res != -1) {
        printf("成功写入文件！\n");
    }
    else {
        printf("写入失败！\n");
    }

}

// 功能: 删除文件
void rm(void)
{
    if (paraStr.length() == 0) {
        printf("该文件不存在！\n");
        return;
    }
    int temp_cur; string temps1, temps2;
    if (paraStr.find('/') != -1) {
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        paraStr = temps1;
        temp_cur = readby(temps1);
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex file_cur = find(temp_cur, temps2);
    bool suc = deleteFile(file_cur);
    if (suc) {
        printf("删除成功！\n");
    }
    else {
        printf("删除失败！\n");
    }
}


// 功能: 删除文件夹
void rmdir(void)
{
    if (paraStr.length() == 0) {
        printf("This file doesn't exist.\n");
        return;
    }
    int temp_cur; string temps1, temps2;
    if (paraStr.find('/') != -1) {
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        paraStr = temps1;
        temp_cur = readby(temps1);
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex file_cur = find(temp_cur, temps2);
    bool suc = deleteFile(file_cur);
    if (suc) {
        printf("删除成功！\n");
    }
    else {
        printf("删除失败！\n");
    }
}

// 功能: exit退出文件系统(quit)
void quit() {
    char choice;
    printf("确定退出？(y/n):");
    scanf("%c", &choice);
    gets_s(writeBuff);
    if ((choice == 'y') || (choice == 'Y')) {
        FSDestruction();//将缓存中的内容写入磁盘
        exit(-1);
    }

}

//cmd下的format函数，包括用户的的格式化
void format() {
    char userAnswer;// 暂存
    printf("确定格式化文件系统？(Y/N)");
    scanf("%c", &userAnswer);
    getchar();//scanf后，这里需要吸收一个回车
    if ((userAnswer == 'y') || (userAnswer == 'Y')) {
        //调用底层函数，格式化磁盘，默认将磁盘超级块等信息存入内存
        formatDisk();
        //清除用户信息

        printf("文件系统创建成功。\n");
    }

}

// 功能: 显示磁盘管理系统基本信息
void info()
{
    printDiskInfo();
}

// 功能: root用户可使用该命令查看其它用户属性
void ui() {
    if (userOfBuff.UserGroup != 0xff)
    {
        cout << "无权限查看用户信息" << endl;
        return;
    }
    uint16_t index = findUserName(paraStr.c_str());//根据用户名查找用户Index
    if (index == -1) {
        printf("该用户不存在！\n");
        return;
    }
    else
        printUserInfo(index);//输出用户信息
}

//功能：改变文件权限
void chmod() {
    if (paraStr.length() == 0) {
        printf("该文件不存在！\n");
        return;
    }
    int temp_cur; string temps1, temps2;
    if (paraStr.find('/') != -1) {//找文件
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        paraStr = temps1;
        temp_cur = readby(temps1);
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex file_cur = find(temp_cur, temps2);
    printf("输入修改权限(r:read;w:write;d:delete):");
    char temp; uint8_t access = 0;
    while ((temp = getchar()) != '\n') {
        switch (temp) {
        case 'r':
            access += 2;
            break;
        case 'w':
            access += 4;
            break;
        case 'd':
            access += 8;
            break;
        default:
            break;
        }
    }
    if (changeAccessMode(file_cur, access))//修改权限写入
        printf("权限修改成功！\n");
    else
        printf("权限修改失败！\n");
}

//功能：以root用户登录时可使用该命令改变其它普通用户权限
void chmodu() {
    int16_t index = findUserName(paraStr.c_str());//根据用户名找用户
    if (index == -1) {
        printf("该用户不存在！\n");
        return;
    }
    char temp; uint8_t access = 0; uint16_t group;
    printf("输入修改组号[0,14]:");
    cin >> group;
    if (group < 0 || group>14) {
        printf("组号输入错误\n");
        return;
    }
    temp = getchar();
    printf("输入修改权限(r:read;w:write;d:delete):");
    while ((temp = getchar()) != '\n') {
        switch (temp) {
        case 'r':
            access += 2;
            break;
        case 'w':
            access += 4;
            break;
        case 'd':
            access += 8;
            break;
        default:
            break;
        }
    }
    access = group * 16 + access;
    if (changeUserAccess(index, access))//写入用户权限
    {
        printf("权限修改成功！\n");
        FileControlBlock fileFCB;//更改用户文件夹相关组号信息，以适配更新的用户权限
        int temp_cur = curFCBIndex;
        curFCBIndex = 0;
        int userfile_cur = readby(paraStr + '/');
        loadFCB(userfile_cur, &fileFCB);
        fileFCB.AccessMode = fileFCB.AccessMode - (fileFCB.AccessMode >> 4 << 4) + group * 16;
        writeFCB(userfile_cur, &fileFCB);
        curFCBIndex = temp_cur;
    }
    else
        printf("权限修改失败！\n");
}

//功能：显示文件属性信息
void fi() {
    if (paraStr.length() == 0) {
        printf("该文件不存在！\n");
        return;
    }
    int temp_cur; string temps1, temps2;//找文件
    if (paraStr.find('/') != -1) {
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        paraStr = temps1;
        temp_cur = readby(temps1);
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex file_cur = find(temp_cur, temps2);
    printInfo(file_cur);//输出文件信息
}

// 功能: 循环执行用户输入的命令, 直到logout
//  0"help", 1"cd", 2"ll", 3"mkdir", 4"touch", 5"cat", 6"vi", 7"rm", 8"su", 9"clear", 10"format",11"exit",12"rmdir",13"info",14"register",15"fi",16"ui",17"chmod",18"chmodu"

void command() {
    system("cls");
    do {
        pathset();
        switch (parseCommand()) {
        case -1:
            printf("\n");
            break;
        case 0:
            help();// 显示可用命令
            break;
        case 1:
            cd(paraStr);// 切换目录路径
            break;
        case 2:
            ll(paraStr);// 显示路径文件夹下信息
            break;
        case 3:
            mkdir();// 创建文件夹
            break;
        case 4:
            touch();// 创建文件
            break;
        case 5:
            cat();//读取文件
            break;
        case 6:
            vim();// 写入文件
            break;
        case 7:
            rm();// 删除文件
            break;
        case 8:
            su();// 切换用户
            break;
        case 9:
            system("cls");// 清屏
            break;
        case 10:
            format();// 格式化
            init();
            freeUserOfBuff();
            login();
            break;
        case 11:
            quit();// 退出
            break;
        case 12:
            rmdir();// 删除文件夹
            break;
        case 13:
            info();// 显示磁盘信息
            break;
        case 14:
            reg();// 注册
            break;
        case 15:
            fi();// 显示文件属性
            break;
        case 16:
            ui();// 显示用户属性
            break;
        case 17:
            chmod();// 修改文件权限
            break;
        case 18:
            chmodu();// 修改用户权限
            break;
        default:
            errcmd();
            break;
        }
    } while (1);
}

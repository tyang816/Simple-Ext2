/*
    命令文件的工具包，主要包含实现command的辅助工具函数
*/
#include "Command.h"
#include "CommandUtil.h"
#include "User.h"
#include "DriverOperate.h"
#include "FileSystem.h"
#include <bits/stdc++.h>
#include <conio.h>

extern string cmdStr, paraStr;
extern FCBIndex curFCBIndex;   //当前文件夹FCB号
extern User userOfBuff;     //用于缓存内存中的用户                                                                                                                             // 文件系统名称
extern FILE* UserFile;            // 打开文件指针

//命令定义，一共21个linux命令
extern vector<string> Commands;
extern SuperBlock Super;

//初始化
void init(){
    // 当前目录为根目录
    curFCBIndex = 0;
}

//设置文件路径，用于回显
void pathset(){
    string curPath;
    if (curFCBIndex == 0) curPath = "";
    else{
        FCBIndex tempFCBIndex = curFCBIndex;
        FileControlBlock fcb;
        while (tempFCBIndex != 0){
            fileInfo(tempFCBIndex, &fcb);//首次依据当前FCB号，查找当前文件夹信息
            curPath = fcb.FileName + curPath;
            curPath = '/' + curPath;
            tempFCBIndex = fcb.Parent;
        }
    }
    printf("[%s]@tyang:~", userOfBuff.UserName);
    cout << curPath << "#";
}

//分析函数，分析命令及参数
//结果: 0-16为系统命令, 17为命令错误
int parseCommand(){
    string line = "";
    cmdStr = "";
    paraStr = "";          // s存全部输入；cmdStr用于存命令；paraStr用于存参数
    int cmdId = 0;      //命令编号
    while (1){
        cmdStr = line; //更新为全部输入
        if (line.find(' ') == -1)
            paraStr = ""; //输入不存在空格，无参数
        else{ //输入存在空格，表示存在参数
            while (!cmdStr.empty() && cmdStr.back() == ' '){  // cmdStr末尾为空格(cmdStr.back())，还未读取参数                            
                cmdStr = cmdStr.substr(0, cmdStr.length() - 1); //将cmdStr的命令部分提取出
            }
            while (!cmdStr.empty() > 0 && cmdStr.front() == ' '){  //命令开始空格去掉
                cmdStr = cmdStr.substr(1); //从第一个字符开始
            }
            if (cmdStr.find(' ') == -1) paraStr = ""; // cmdStr无空格
            else paraStr = cmdStr.substr(cmdStr.find_first_of(' ') + 1); //将参数部分传入paraStr
            while (!paraStr.empty() && paraStr.back() == ' '){ //将参数末尾多余空格去掉
                paraStr = paraStr.substr(0, paraStr.length() - 1);
            }
            while (paraStr.length() > 0 && paraStr[0] == ' '){ //将参数开头空格去除
                paraStr = paraStr.substr(1);
            }
            cmdStr = cmdStr.substr(0, cmdStr.find_first_of(' ')); //将while循环开始的全部输入cmdStr，更新为命令
                                                      //第二个参数是第一个空格的位置，与命令长度相等
        }
        int ch = _getch(); //从控制台读取一个字符，接受一个任意键的输入，不用按回车就返回，但不显示在屏幕上，在conio.h中
        if (ch == 8){ //退格
            if (!line.empty()){
                printf("%c", 8);                 //回退至前一个字符的位置
                printf(" ");                     //空格取代
                printf("%c", 8);                 //再回退至前一个字符的位置，增加交互友好
                line = line.substr(0, line.length() - 1); //去掉最后一个字符
            }
        }
        else if (ch == 13){ //回车
            for (cmdId = 0; cmdId < Commands.size(); cmdId++){//若未匹配成功，则id为18，此时将判断命令错误
                if (cmdStr == Commands[cmdId]) break; //匹配命令
            }
            break; //结束输入
        }
        else if (ch == 9)
        { // tab
        }
        else if (ch == ' ')
        { //空格回显，存入s中
            printf("%c", ch);
            line.push_back(ch);
        }
        else
        { //其他字符回显，存入s中
            printf("%c", ch);
            line.push_back(ch);
        }
    }
    if (cmdStr == ""){
        return -1;
    }
    printf("\n");
    return cmdId; //返回命令编号
}

// result_cur 最终cd到的文件节点号
// paraStr cd 后面的路径串
// inum_cur  当前文件夹的节点号
int readby(string path)
{ //根据当前目录和第二个参数确定转过去的目录
    int result_cur = 0;
    string s = path;
    if (s.find('/') != -1)
    {
        s = s.substr(0, s.find_last_of('/') + 1);
    }
    else
    {
        s = "";
    }
    int temp_cur = curFCBIndex;
    vector<string> v;
    while (s.find('/') != -1)
    {                                                   // 将路径的每一级文件夹存入vector
        v.push_back(s.substr(0, s.find_first_of('/'))); // 截取第1个斜杠之前的字符串
        s = s.substr(s.find_first_of('/') + 1);         //
    }
    if (v.empty())
    { // 说明没有子目录，直接返回
        return curFCBIndex;
    }
    if (v[0].empty())
    { // 没有任何移动，依旧在当前文件夹
        temp_cur = 0;
    }
    else if (v[0] == ".."){    // 返回到上一级目录
        temp_cur = find(curFCBIndex, v[0]); // 返回上一级目录的目录号
    }
    else{
        temp_cur = find(curFCBIndex, v[0]);
    }
    for (unsigned int count = 1; count < v.size(); count++){ // 逐级找到cd的最终文件夹
        temp_cur = find(temp_cur, v[count]);
    }
    result_cur = temp_cur;
    return result_cur;
}

//清空内存中存在的用户名和密码
void freeUserOfBuff(){
    for (int i = 0; i < 14; i++) {
        userOfBuff.UserName[i] = '\0';
        userOfBuff.Password[i] = '\0';
    }
}

// 功能: 显示错误
void errcmd(){
    printf("命令不存在！请查看help \n");
}


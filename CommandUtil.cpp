/*
    �����ļ��Ĺ��߰�����Ҫ����ʵ��command�ĸ������ߺ���
*/
#include "Command.h"
#include "CommandUtil.h"
#include "User.h"
#include "DriverOperate.h"
#include "FileSystem.h"
#include <bits/stdc++.h>
#include <conio.h>

extern string cmdStr, paraStr;
extern FCBIndex curFCBIndex;   //��ǰ�ļ���FCB��
extern User userOfBuff;     //���ڻ����ڴ��е��û�                                                                                                                             // �ļ�ϵͳ����
extern FILE* UserFile;            // ���ļ�ָ��

//����壬һ��21��linux����
extern vector<string> Commands;
extern SuperBlock Super;

//��ʼ��
void init(){
    // ��ǰĿ¼Ϊ��Ŀ¼
    curFCBIndex = 0;
}

//�����ļ�·�������ڻ���
void pathset(){
    string curPath;
    if (curFCBIndex == 0) curPath = "";
    else{
        FCBIndex tempFCBIndex = curFCBIndex;
        FileControlBlock fcb;
        while (tempFCBIndex != 0){
            fileInfo(tempFCBIndex, &fcb);//�״����ݵ�ǰFCB�ţ����ҵ�ǰ�ļ�����Ϣ
            curPath = fcb.FileName + curPath;
            curPath = '/' + curPath;
            tempFCBIndex = fcb.Parent;
        }
    }
    printf("[%s]@�ڰ���:~", userOfBuff.UserName);
    cout << curPath << "#";
}

//���������������������
//���: 0-16Ϊϵͳ����, 17Ϊ�������
int parseCommand(){
    string line = "";
    cmdStr = "";
    paraStr = "";          // s��ȫ�����룻cmdStr���ڴ����paraStr���ڴ����
    int cmdId = 0;      //������
    while (1){
        cmdStr = line; //����Ϊȫ������
        if (line.find(' ') == -1)
            paraStr = ""; //���벻���ڿո��޲���
        else{ //������ڿո񣬱�ʾ���ڲ���
            while (!cmdStr.empty() && cmdStr.back() == ' '){  // cmdStrĩβΪ�ո�(cmdStr.back())����δ��ȡ����                            
                cmdStr = cmdStr.substr(0, cmdStr.length() - 1); //��cmdStr���������ȡ��
            }
            while (!cmdStr.empty() > 0 && cmdStr.front() == ' '){  //���ʼ�ո�ȥ��
                cmdStr = cmdStr.substr(1); //�ӵ�һ���ַ���ʼ
            }
            if (cmdStr.find(' ') == -1) paraStr = ""; // cmdStr�޿ո�
            else paraStr = cmdStr.substr(cmdStr.find_first_of(' ') + 1); //���������ִ���paraStr
            while (!paraStr.empty() && paraStr.back() == ' '){ //������ĩβ����ո�ȥ��
                paraStr = paraStr.substr(0, paraStr.length() - 1);
            }
            while (paraStr.length() > 0 && paraStr[0] == ' '){ //��������ͷ�ո�ȥ��
                paraStr = paraStr.substr(1);
            }
            cmdStr = cmdStr.substr(0, cmdStr.find_first_of(' ')); //��whileѭ����ʼ��ȫ������cmdStr������Ϊ����
                                                      //�ڶ��������ǵ�һ���ո��λ�ã�����������
        }
        int ch = _getch(); //�ӿ���̨��ȡһ���ַ�������һ������������룬���ð��س��ͷ��أ�������ʾ����Ļ�ϣ���conio.h��
        if (ch == 8){ //�˸�
            if (!line.empty()){
                printf("%c", 8);                 //������ǰһ���ַ���λ��
                printf(" ");                     //�ո�ȡ��
                printf("%c", 8);                 //�ٻ�����ǰһ���ַ���λ�ã����ӽ����Ѻ�
                line = line.substr(0, line.length() - 1); //ȥ�����һ���ַ�
            }
        }
        else if (ch == 13){ //�س�
            for (cmdId = 0; cmdId < Commands.size(); cmdId++){//��δƥ��ɹ�����idΪ18����ʱ���ж��������
                if (cmdStr == Commands[cmdId]) break; //ƥ������
            }
            break; //��������
        }
        else if (ch == 9)
        { // tab
        }
        else if (ch == ' ')
        { //�ո���ԣ�����s��
            printf("%c", ch);
            line.push_back(ch);
        }
        else
        { //�����ַ����ԣ�����s��
            printf("%c", ch);
            line.push_back(ch);
        }
    }
    if (cmdStr == ""){
        return -1;
    }
    printf("\n");
    return cmdId; //����������
}

// result_cur ����cd�����ļ��ڵ��
// paraStr cd �����·����
// inum_cur  ��ǰ�ļ��еĽڵ��
int readby(string path)
{ //���ݵ�ǰĿ¼�͵ڶ�������ȷ��ת��ȥ��Ŀ¼
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
    {                                                   // ��·����ÿһ���ļ��д���vector
        v.push_back(s.substr(0, s.find_first_of('/'))); // ��ȡ��1��б��֮ǰ���ַ���
        s = s.substr(s.find_first_of('/') + 1);         //
    }
    if (v.empty())
    { // ˵��û����Ŀ¼��ֱ�ӷ���
        return curFCBIndex;
    }
    if (v[0].empty())
    { // û���κ��ƶ��������ڵ�ǰ�ļ���
        temp_cur = 0;
    }
    else if (v[0] == ".."){    // ���ص���һ��Ŀ¼
        temp_cur = find(curFCBIndex, v[0]); // ������һ��Ŀ¼��Ŀ¼��
    }
    else{
        temp_cur = find(curFCBIndex, v[0]);
    }
    for (unsigned int count = 1; count < v.size(); count++){ // ���ҵ�cd�������ļ���
        temp_cur = find(temp_cur, v[count]);
    }
    result_cur = temp_cur;
    return result_cur;
}

//����ڴ��д��ڵ��û���������
void freeUserOfBuff(){
    for (int i = 0; i < 14; i++) {
        userOfBuff.UserName[i] = '\0';
        userOfBuff.Password[i] = '\0';
    }
}

// ����: ��ʾ����
void errcmd(){
    printf("������ڣ���鿴help \n");
}


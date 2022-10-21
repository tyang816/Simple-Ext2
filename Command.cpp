/*
    �����ļ�����Ҫ������linux�ĳ��������ʵ��
*/
#define _CRT_SECURE_NO_WARNINGS
#include "Command.h"
#include "CommandUtil.h"
#include "User.h"
#include "FileSystem.h"
#include <bits/stdc++.h>
#include <conio.h>


string cmdStr, paraStr;//�ֱ�洢���������

FCBIndex curFCBIndex;   //��ǰ�ļ���FCB��
char writeBuff[10 * BLOCKSIZE];  //�˳�ʱд�뻺����                                                                                                                              // �ļ�ϵͳ����
FILE* UserFile;            // ���ļ�ָ��

extern User userOfBuff;// �ݴ��¼�û�


//����壬һ��19��linux����
vector<string> Commands = { "help", "cd", "ll", "mkdir", "touch",
"cat", "vim", "rm", "su", "clear", "format", "exit", "rmdir",
"info", "register","fi","ui","chmod","chmodu" }; // 19��

//���ܣ���¼ϵͳ
void login() {
    User user;
    char* word;
    bool flag = 0;
    do {
        printf("�û�����");
        gets_s(user.UserName);//�û������û���������
        if (strcmp(user.UserName, "") == 0) {
            printf("�û�������Ϊ�գ�\n");
            flag = 1;
            break;
        }
        printf("���룺");
        word = user.Password;
        while (*word = _getch()) {  //�������룬������
            if (*word == 0x0d) {  //������س���ʱ��0x0dΪ�س�����ASCII��
                *word = '\0'; //������Ļس���ת���ɿո�
                break;
            }
            printf("*"); //�������������"*"����ʾ
            word++;
        }
        if (strcmp(user.Password, "") == 0) {
            printf("��Ǹ�����벻��Ϊ�գ�\n");
            flag = 1;
            break;
        }
        printf("\n");
    } while (!checkUser(&user));
    if (!flag) {
        readDisk((uint8_t*)&userOfBuff, SUPER_SIZE + findUserName(user.UserName) * USER_LEN, USER_LEN);
        if (strcmp(user.UserName, "root") != 0) {
            cd(user.UserName);//�����û��ļ���
            return; //��½�ɹ���ֱ��������½����
        }
    }
}

//���ܣ�ע�����û�
void reg() {
    User user;
    char* word;
    if (userOfBuff.UserGroup != 0xff) {
        printf("��root�û�����Ȩ��ע�����û���\n");
        return;
    }
    printf("�û�����");
    gets_s(user.UserName);//�û������û���������
    if (strcmp(user.UserName, "") == 0) {
        printf("�û�������Ϊ�գ�\n");
        return;
    }
    if (findUserName(user.UserName) != -1) {
        printf("��Ǹ���û��Ѵ��ڣ�\n");
        return;
    }
    printf("���룺");
    word = user.Password;
    while (*word = _getch()) {  //�������룬������
        if (*word == 0x0d) {  //������س���ʱ��0x0dΪ�س�����ASCII��
            *word = '\0'; //������Ļس���ת���ɿո�
            break;
        }
        printf("*"); //�������������"*"����ʾ
        word++;
    }
    if (strcmp(user.Password, "") == 0) {
        printf("��Ǹ�����벻��Ϊ�գ�\n");
        return;
    }
    printf("\n");
    printf("�����û���ţ�[0,14]����");
    int group;
    scanf("%d", &group);
    if (group > 14 || group < 0) {
        cout << "��Ŵ���" << endl;
        return;
    }
    getchar();//scanf��������Ҫ����һ���س�
    if (addUser(user.UserName, user.Password, ((group << 4) + 15))) {
        printf("�û�[%s]�����ɹ���\n", user.UserName);
        createDirectory(user.UserName, 0, group);//�ڸ�Ŀ¼�´����û��ļ��У��������û�ʹ��
        return;
    }
    else if (sizeof(user.UserName) > 16) {
        printf("��Ǹ���û���������\n");
        return;
    }
    else if (sizeof(user.Password) > 15) {
        printf("��Ǹ�����������\n");
        return;
    }
}

//����: �л���¼�û�
void su() {
    User user;
    char* word;
    printf("�û�����");
    gets_s(user.UserName);//�û������û���������
    if (strcmp(user.UserName, "") == 0) {
        printf("�û�������Ϊ�գ�\n");
        return;
    }
    if (findUserName(user.UserName) == -1) {
        printf("��Ǹ���û������ڣ�\n");
        return;
    }
    printf("���룺");
    word = user.Password;
    while (*word = _getch()) {
        if (*word == 0x0d) { 		//������س���ʱ��0x0dΪ�س�����ASCII��
            *word = '\0'; 			//������Ļس���ת���ɿո�
            break;
        }
        printf("*");   //�������������"*"����ʾ
        word++;
    }
    if (strcmp(user.Password, "") == 0) {
        printf("��Ǹ�����벻��Ϊ�գ�\n");
        return;
    }
    printf("\n");
    if (checkUser(&user)) {
        init();//��inum_cur��ʼ��Ϊ0���ص���Ŀ¼
        readDisk((uint8_t*)&userOfBuff, SUPER_SIZE + findUserName(user.UserName) * USER_LEN, USER_LEN);
        if (strcmp(user.UserName, "root") != 0) cd(user.UserName);//�����û��ļ���
        return;     //��½�ɹ���ֱ��������½����
    }
}


//����: ��ʾ��������
void help() {
    printf("�������: \n\
    help      ---       ��ʾ�����˵� \n\
    cd        ---       �ı��ļ��� \n\
    clear     ---       ���� \n\
    ll        ---       ��ʾ�ڵ�ǰĿ¼�������ļ����ļ��� \n\
    mkdir     ---       �������ļ���   \n\
    touch     ---       �������ļ� \n\
    cat       ---       �򿪲���ȡ�Ѵ����ļ� \n\
    vim       ---       ���Ѵ����ļ�������д�� \n\
    rm        ---       ɾ���Ѵ����ļ� \n\
    su        ---       �л���ǰ�û� \n\
    format    ---       ��ʽ����ǰ�ļ�ϵͳ \n\
    exit      ---       �˳�ϵͳ \n\
    rmdir     ---       ɾ���ļ��� \n\
    info      ---       ��ʾ����ϵͳ��Ϣ \n\
    register  ---       ע�����û�\n\
    fi        ---       ��ʾ�ļ���Ϣ\n\
    ui        ---       ��ʾ�û���Ϣ\n\
    chmod     ---       �����ļ�Ȩ��\n\
    chmodu    ---       �����û�Ȩ��\n ");
}

// ����: �л�Ŀ¼(cd .. ���� cd dir1/dir2)
void cd(string path) {
    int temp_cur;
    if (path.empty()) {
        printf("û������·����\n");
        return;
    }
    else {
        if (path.back() != '/')
            path += '/';
        temp_cur = readby(path);
    }
    if (temp_cur != -1) curFCBIndex = temp_cur;
    else printf("û�и�Ŀ¼��\n");
}

//���ܣ���ʾ��ǰ�ļ��������ļ�
void ll(string path) { // pathΪ�����г���ǰ�ļ����µ�ȫ�����ļ�����Ϊ�����г�path·���µ����ļ�
    int temp_cur;
    if (path.empty()) temp_cur = curFCBIndex;
    else
    {
        if (path.back() != '/')
            path += '/';
        temp_cur = readby(path);
        if (temp_cur == -1) {
            cout << "û�и��ļ��У�\n" << endl;
            return;
        }
    }
    if (temp_cur != -1) {
        printf(" %6s | %8s | %12s | %10s\n", "FCB", "�ļ���", "��С", "����·��");
        printDir(temp_cur);
    }
    cout << endl;
}

//cmd�����ļ��������ڵ�ǰĿ¼�´����ļ���
void mkdir() {
    if (paraStr.empty()) {
        printf("�������ļ�����\n");
        return;
    }
    int temp_cur;
    string temps1, temps2;
    if (paraStr.find('/') != -1) {  // Ҫ�������ļ��в��ڵ�ǰĿ¼�£�������·���е�ָ���ļ���
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        temp_cur = readby(temps1);
        if (temp_cur == -1) {
            printf("û�и��ļ��У�\n");
        }
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex index = createDirectory(temps2, temp_cur, userOfBuff.UserGroup >> 4);
    if (index != -1) {
        printf("�����ļ��гɹ���\n");
    }
    else {
        printf("�����ļ���ʧ�ܣ�\n");
    }
}

// ����: �ڵ�ǰĿ¼�´����ļ�(creat file1)
void touch()
{
    if (paraStr.length() == 0) {
        printf("�������ļ�����\n");
        return;
    }
    int temp_cur;
    string temps1, temps2;
    if (paraStr.find('/') != -1) {  // Ҫ������file���ڵ�ǰĿ¼�£�������·���е�ָ���ļ���
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);//temps2���ļ�����+1��'/'ȥ��
        temp_cur = readby(temps1);
        if (temp_cur == -1) {
            printf("û�и��ļ��У�\n");
        }
    }
    else {
        temps2 = paraStr;
        temp_cur = curFCBIndex;
    }
    FCBIndex index = createFile(temps2, temp_cur, userOfBuff.UserGroup >> 4);
    if (index != -1) {
        printf("�����ļ��ɹ���\n");
    }
    else {
        printf("�����ļ�ʧ�ܣ�\n");
    }
}

// an exist file
void cat() {
    string temps1, temps2; int temp_cur;
    if (paraStr.empty()) {
        cout << "�ļ���Ϊ�գ�" << endl;
        return;
    }
    if (paraStr.find('/') != -1) {  // ����Ĳ����ļ�������·��
        temps1 = paraStr.substr(0, paraStr.find_last_of('/') + 1);
        temps2 = paraStr.substr(paraStr.find_last_of('/') + 1);
        temp_cur = readby(temps1);
    }
    else {
        temps2 = paraStr;//���ļ������浽temps2��
        temp_cur = curFCBIndex;
    }
    FCBIndex file_cur = find(temp_cur, temps2);
    if (file_cur == -1) {//���δ�ҵ����ļ����򷵻�
        cout << "�ļ������ڣ�" << endl;
        return;
    }
    FileControlBlock fcb;
    fileInfo(file_cur, &fcb);
    uint8_t* buff = (uint8_t*)malloc(fcb.Size + 1);//���һ���ֽڣ����ӡ�\0��
    int64_t res = readFile(file_cur, 0, fcb.Size, buff);//ָ�����ʹ�ָ�����Ͳ������ü�&
    buff[fcb.Size] = '\0';//
    if (res != -1) {
        printf("%s\n", buff);
    }
    else {
        printf("����ʧ�ܣ�\n");
    }
    free(buff);
}

// open and write something to a particular file
void vim() {
    int64_t res;
    string temps1, temps2; int temp_cur;
    char temp[5 * BLOCKSIZE];
    //uint8_t* temp;
    if (paraStr.find('/') != -1) {  // ����Ĳ����ļ�������·��
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
    FCBIndex file_cur = find(temp_cur, temps2);//���ݵ�ǰ�ļ��кţ����ļ��������ļ�FCB��
    if (file_cur == -1) {//���δ�ҵ����ļ����򷵻�
        cout << "�ļ������ڣ�" << endl;
        return;
    }
    FileControlBlock fcb;
    fileInfo(file_cur, &fcb);//���ݲ��ҵ���FCB�Ż���ļ�FCB
    if (fcb.Size == 0) {
        printf("�����룺 ");//���ﲻҪ�ӻ��з�
        gets_s(temp);
        res = writeFile(file_cur, 0, strlen((char*)(temp)), (uint8_t*)(temp));
    }
    else {
        char choice;
        printf("�ļ��������� \n");
        printf("���ǻ�׷�ӣ�(o/a):");
        scanf("%c", &choice);
        getchar();//scanf����Ҫ����䣬���ջس���������gets_s�Ὣ�س�ֱ�Ӵ浽�����temp������
        if (choice == 'o') {
            printf("�����룺 ");//���ﲻҪ�ӻ��з�
            gets_s(temp);
            res = writeFile(file_cur, 0, strlen((char*)(temp)) + 1, (uint8_t*)(temp));//��temp����'\0'������ɸ���
        }
        else if (choice == 'a') {
            printf("�����룺 ");//���ﲻҪ�ӻ��з�
            gets_s(temp);
            res = writeFile(file_cur, fcb.Size, strlen((char*)(temp)), (uint8_t*)(temp));//fcb.Size����Ҫ+1����0��ʼ


        }
        else {
            printf("�˳�д�룡\n");
        }
    }
    if (res != -1) {
        printf("�ɹ�д���ļ���\n");
    }
    else {
        printf("д��ʧ�ܣ�\n");
    }

}

// ����: ɾ���ļ�
void rm(void)
{
    if (paraStr.length() == 0) {
        printf("���ļ������ڣ�\n");
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
        printf("ɾ���ɹ���\n");
    }
    else {
        printf("ɾ��ʧ�ܣ�\n");
    }
}


// ����: ɾ���ļ���
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
        printf("ɾ���ɹ���\n");
    }
    else {
        printf("ɾ��ʧ�ܣ�\n");
    }
}

// ����: exit�˳��ļ�ϵͳ(quit)
void quit() {
    char choice;
    printf("ȷ���˳���(y/n):");
    scanf("%c", &choice);
    gets_s(writeBuff);
    if ((choice == 'y') || (choice == 'Y')) {
        FSDestruction();//�������е�����д�����
        exit(-1);
    }

}

//cmd�µ�format�����������û��ĵĸ�ʽ��
void format() {
    char userAnswer;// �ݴ�
    printf("ȷ����ʽ���ļ�ϵͳ��(Y/N)");
    scanf("%c", &userAnswer);
    getchar();//scanf��������Ҫ����һ���س�
    if ((userAnswer == 'y') || (userAnswer == 'Y')) {
        //���õײ㺯������ʽ�����̣�Ĭ�Ͻ����̳��������Ϣ�����ڴ�
        formatDisk();
        //����û���Ϣ

        printf("�ļ�ϵͳ�����ɹ���\n");
    }

}

// ����: ��ʾ���̹���ϵͳ������Ϣ
void info()
{
    printDiskInfo();
}

// ����: root�û���ʹ�ø�����鿴�����û�����
void ui() {
    if (userOfBuff.UserGroup != 0xff)
    {
        cout << "��Ȩ�޲鿴�û���Ϣ" << endl;
        return;
    }
    uint16_t index = findUserName(paraStr.c_str());//�����û��������û�Index
    if (index == -1) {
        printf("���û������ڣ�\n");
        return;
    }
    else
        printUserInfo(index);//����û���Ϣ
}

//���ܣ��ı��ļ�Ȩ��
void chmod() {
    if (paraStr.length() == 0) {
        printf("���ļ������ڣ�\n");
        return;
    }
    int temp_cur; string temps1, temps2;
    if (paraStr.find('/') != -1) {//���ļ�
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
    printf("�����޸�Ȩ��(r:read;w:write;d:delete):");
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
    if (changeAccessMode(file_cur, access))//�޸�Ȩ��д��
        printf("Ȩ���޸ĳɹ���\n");
    else
        printf("Ȩ���޸�ʧ�ܣ�\n");
}

//���ܣ���root�û���¼ʱ��ʹ�ø�����ı�������ͨ�û�Ȩ��
void chmodu() {
    int16_t index = findUserName(paraStr.c_str());//�����û������û�
    if (index == -1) {
        printf("���û������ڣ�\n");
        return;
    }
    char temp; uint8_t access = 0; uint16_t group;
    printf("�����޸����[0,14]:");
    cin >> group;
    if (group < 0 || group>14) {
        printf("����������\n");
        return;
    }
    temp = getchar();
    printf("�����޸�Ȩ��(r:read;w:write;d:delete):");
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
    if (changeUserAccess(index, access))//д���û�Ȩ��
    {
        printf("Ȩ���޸ĳɹ���\n");
        FileControlBlock fileFCB;//�����û��ļ�����������Ϣ����������µ��û�Ȩ��
        int temp_cur = curFCBIndex;
        curFCBIndex = 0;
        int userfile_cur = readby(paraStr + '/');
        loadFCB(userfile_cur, &fileFCB);
        fileFCB.AccessMode = fileFCB.AccessMode - (fileFCB.AccessMode >> 4 << 4) + group * 16;
        writeFCB(userfile_cur, &fileFCB);
        curFCBIndex = temp_cur;
    }
    else
        printf("Ȩ���޸�ʧ�ܣ�\n");
}

//���ܣ���ʾ�ļ�������Ϣ
void fi() {
    if (paraStr.length() == 0) {
        printf("���ļ������ڣ�\n");
        return;
    }
    int temp_cur; string temps1, temps2;//���ļ�
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
    printInfo(file_cur);//����ļ���Ϣ
}

// ����: ѭ��ִ���û����������, ֱ��logout
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
            help();// ��ʾ��������
            break;
        case 1:
            cd(paraStr);// �л�Ŀ¼·��
            break;
        case 2:
            ll(paraStr);// ��ʾ·���ļ�������Ϣ
            break;
        case 3:
            mkdir();// �����ļ���
            break;
        case 4:
            touch();// �����ļ�
            break;
        case 5:
            cat();//��ȡ�ļ�
            break;
        case 6:
            vim();// д���ļ�
            break;
        case 7:
            rm();// ɾ���ļ�
            break;
        case 8:
            su();// �л��û�
            break;
        case 9:
            system("cls");// ����
            break;
        case 10:
            format();// ��ʽ��
            init();
            freeUserOfBuff();
            login();
            break;
        case 11:
            quit();// �˳�
            break;
        case 12:
            rmdir();// ɾ���ļ���
            break;
        case 13:
            info();// ��ʾ������Ϣ
            break;
        case 14:
            reg();// ע��
            break;
        case 15:
            fi();// ��ʾ�ļ�����
            break;
        case 16:
            ui();// ��ʾ�û�����
            break;
        case 17:
            chmod();// �޸��ļ�Ȩ��
            break;
        case 18:
            chmodu();// �޸��û�Ȩ��
            break;
        default:
            errcmd();
            break;
        }
    } while (1);
}

#include "FileSystem.h"
#include "DriverOperate.h"
#include "Command.h"
#include "CommandUtil.h"

int main(void){
    while (!FSInit()) {//���ش��̣������̵ĳ�����ȿ�����Ϣ�����ڴ棬û��format()������Ҫ����һ��
        format();
        FSDestruction();
    }
    printf("��ӭ�����ļ�����ϵͳ��\n");
    init(); //����inum_cur=0
    login(); //��¼
    command();
    FSDestruction(); //�������е�����д�����
    return 0;
}

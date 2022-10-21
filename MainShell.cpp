#include "FileSystem.h"
#include "DriverOperate.h"
#include "Command.h"
#include "CommandUtil.h"

int main(void){
    while (!FSInit()) {//挂载磁盘，将磁盘的超级块等控制信息放入内存，没有format()，必须要有这一步
        format();
        FSDestruction();
    }
    printf("欢迎进入文件管理系统！\n");
    init(); //设置inum_cur=0
    login(); //登录
    command();
    FSDestruction(); //将缓存中的内容写入磁盘
    return 0;
}

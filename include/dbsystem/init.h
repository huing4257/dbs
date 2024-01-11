//
// Created by 胡 on 2023/12/30.
//

#ifndef DBS_INIT_H
#define DBS_INIT_H
#include <sys/stat.h>
#include <iostream>
int init_dir(){
    mkdir("data", 0777);
    mkdir("data/global", 0777);
    mkdir("data/base", 0777);
    printf("@Directory structure created.\n");
    return 0;
}
#endif//DBS_INIT_H

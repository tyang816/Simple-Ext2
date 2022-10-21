#pragma once
#include "FileSystem.h"
#include "DriverOperate.h"
#include <bits/stdc++.h>
#include <conio.h>
using namespace std;

void init();
void pathset();
int parseCommand();
int readby(string path);
void freeUserOfBuff();
void errcmd();
void command();
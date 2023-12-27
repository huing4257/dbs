#include <iostream>
#include <cstdio>
#include "antlr4-runtime.h"
using namespace std;

int main()
{
    printf("Hello, world!\n");

    for (std::string s; s != "exit"; ){
        std::cin >> s;
        cout << s << endl;
    }
    printf("exit");
    return 0;
}
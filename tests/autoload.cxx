#include <cstdio>
using namespace std;

void __attribute__((constructor)) myinit();
//extern int testvar;

void myinit()
{
  printf("myinit \n");
}

void __attribute__((destructor)) myfini();

void myfini()
{
  printf("myfini\n");
}

int main(int argc, char* argv[])
{
  printf("main\n");
}


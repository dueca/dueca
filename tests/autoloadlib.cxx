#include <cstdio>

extern "C" {
  void lmyinit() __attribute__ ((constructor)) ;
  void lmyfini() __attribute__ ((destructor)) ;
}

void lmyinit()
{
  printf("libinit\n");
}



void lmyfini()
{
  printf("libfini\n");
}

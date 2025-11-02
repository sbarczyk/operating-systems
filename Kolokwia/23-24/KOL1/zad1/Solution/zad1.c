#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dlfcn.h>

int main (int l_param, char * wparam[]){
  int i;
  int tab[20]={1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0};

  /*
1) otworz biblioteke
2) przypisz wskaznikom f1 i f2 adresy funkcji sumuj i dziel z biblioteki
3) stworz Makefile kompilujacy biblioteke 'bibl1' ladowana dynamicznie
   oraz kompilujacy ten program
*/

  void *handle = dlopen("./bibl1.so", RTLD_LAZY);

  if (!handle){
    perror("dlopen error");
  }
  int (*f1)(int *, int);
  f1 = dlsym(handle, "sumuj");
  char * error1 = dlerror();
  if (error1 != NULL){
    perror("dlsym f1 error");
  }

  
  double (*f2)(int, int);
  f2 = dlsym(handle, "dziel");
  char * error2 = dlerror();
  if (error2 != NULL){
    perror("dlsym f2 error");
  }

  for (i=0; i<3; i++) printf("Wynik: %d, %lf\n", f1(tab+i, 20-i), f2(tab[i], tab[i+1]));
  dlclose(handle);
  return 0;
}

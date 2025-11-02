#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dlfcn.h>

int main (int l_param, char * wparam[]){
  int i;
  int tab[20]={1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0};
/*
1) otworz biblioteke
2) przypisz wskaznikom f1 i f2 adresy funkcji z biblioteki sumuj i srednia
3) stworz Makefile kompilujacy biblioteke 'bibl1' ladowana dynamicznie oraz kompilujacy ten program
4) Stosowne pliki powinny znajdowac sie w folderach '.', './bin', './'lib'. Nalezy uzyc: LD_LIBRARY_PATH
*/

  void *handle = dlopen("./bibl1.so", RTLD_LAZY);

  if (!handle){
    perror("dlopen error");
  }

  int (*f1)(int *, int);
  f1 = dlsym(handle, "sumuj");

  if (!f1){
    perror("f1 dlsym error");
  }

  double (*f2)(int *, int);
  f2 = dlsym(handle, "srednia");
  if (!f2){
    perror("f2 dlsym eror");
  }

  for (i=0; i<5; i++) printf("Wynik: %d, %lf\n", f1(tab+i, 20-i), f2(tab+i, 20-i));
  dlclose(handle);
  return 0;
}

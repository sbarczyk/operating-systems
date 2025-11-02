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
3) stworz Makefile kompilujacy biblioteke 'bibl1' ladowana dynamicznie
   oraz kompilujacy ten program
*/

    void *handle = dlopen("./bibl1.so", RTLD_LAZY);
    if (!handle){
        fprintf(stderr, "błąd w czasie otwierania biblioteki");
        return EXIT_FAILURE;
    }

    int (*f1)(int *, int);
    f1 = dlsym(handle, "sumuj");

    char *error = dlerror();
    if (error != NULL){
        fprintf(stderr, "błąd w czasie wyszukiwania funkcji: sumuj");
        dlclose(handle);
        return EXIT_FAILURE;
    }

    double (*f2)(int *, int);
    f2 = dlsym(handle, "srednia");

    char *error2 = dlerror();
    if (error2 != NULL){
        fprintf(stderr, "błąd w czasie wyszukiwania funkcji: srednia");
        dlclose(handle);
        return EXIT_FAILURE;
    }
    
  
    for (i=0; i<5; i++) printf("Wynik: suma = %d, średnia = %lf\n", f1(tab+i, 20-i), f2(tab+i, 20-i));
    dlclose(handle);
  return 0;
}
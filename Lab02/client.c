#include <stdio.h>
#include <stdlib.h>

#ifdef DYNAMIC_LOADING
#include <dlfcn.h>
#else
#include "collatz.h"
#endif

#define MAX_ITER 1000

void run_test(int num)
{
    int steps[MAX_ITER + 1];
    int steps_count = 0;

#ifdef DYNAMIC_LOADING
    void *handle = dlopen("./libcollatz.so", RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "Nie udało się załadować biblioteki: %s\n", dlerror());
        return;
    }

    int (*test_collatz_convergence_ptr)(int, int, int *);
    test_collatz_convergence_ptr = dlsym(handle, "test_collatz_convergence");
    char *error;
    if ((error = dlerror()) != NULL)
    {
        fprintf(stderr, "Nie udało się pobrać symbolu: %s\n", error);
        dlclose(handle);
        return;
    }

    steps_count = test_collatz_convergence_ptr(num, MAX_ITER, steps);
    dlclose(handle);
#else
    steps_count = test_collatz_convergence(num, MAX_ITER, steps);
#endif

    if (steps_count > 0)
    {
        printf("Liczba %d zbiega do 1 w %d krokach:\n", num, steps_count);
        for (int i = 0; i < steps_count; i++)
        {
            printf("%d ", steps[i]);
        }
        printf("\n");
    }
    else
    {
        printf("Liczba %d nie zbiega do 1 w %d iteracjach.\n", num, MAX_ITER);
        printf("\n");
    }
}

int main(void)
{
    int test_numbers[] = {3, 7, 27, 97};
    int count = sizeof(test_numbers) / sizeof(test_numbers[0]);
    for (int i = 0; i < count; i++)
    {
        run_test(test_numbers[i]);
    }
    return 0;
}
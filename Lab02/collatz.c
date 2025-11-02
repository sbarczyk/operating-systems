#include <stdio.h> 
#include "collatz.h"
#include <dlfcn.h>

int collatz_conjecture(int input) {
    if (input <= 0) return -1;
    if (input % 2 == 0) {
        return input / 2;
    } else {
        return 3 * input + 1;
    }
}

int test_collatz_convergence(int input, int max_iter, int *steps) {
    if (input <= 0 || max_iter <= 0 || steps == NULL) return 0;

    int current = input;

    for (int count = 0; count < max_iter; count++) {
        steps[count] = current;

        if (current == 1) return count + 1;

        current = collatz_conjecture(current);
        if (current == -1) return 0;
    }

    return 0;
}
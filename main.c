#include "co.h"
#include <stdio.h>

struct args {
    int n;
};

static void foo2(struct schedule* S, int i) {
    printf("foo2 start: %d\n", i);
    co_yield(S);
    printf("foo2 end: %d\n", i);
}

static void foo(struct schedule* S, void* ud) {
    struct args* arg = ud;
    int start = arg->n;
    int i;
    for (i = 0; i < 5; i++) {
        // printf("coroutine %d: %d\n", co_running(S), start + i);
        //co_yield(S);
        foo2(S, start + i);
    }
}

static void test(struct schedule* S) {
    struct args arg1 = {0};
    struct args arg2 = {100};

    int co1 = co_new(S, foo, &arg1);
    int co2 = co_new(S, foo, &arg2);

    printf("=== start ===\n");
    while(co_status(S, co1) && co_status(S, co2)) {
        co_resume(S, co1);
        co_resume(S, co2);
    }
    printf("=== end ===\n");
}

int main() {
    struct schedule* S = co_open();
    test(S);
    co_close(S);
    
    return 0;
}


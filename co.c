// https://github.com/chenyahui/AnnotatedCode/blob/master/coroutine/coroutine.c  
#include "co.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <ucontext.h>

#define STACK_SIZE (1024*1024)
#define DEFAULT_COROUTINE 16

struct coro; // coroutine

struct schedule {
    char stack[STACK_SIZE];
    ucontext_t main;
    int nco;
    int cap;
    int running_id;
    struct coro** co_arr;
};

struct coro {
    co_func f;
    void *ud;
    ucontext_t ctx;
    struct schedule* sch;
    ptrdiff_t cap; // 已经分配的内存大小
    ptrdiff_t size; // 当前协程运行时栈，保存起来后的大小
    int status;
    char *stack;
};

struct coro* _co_new(struct schedule* S, co_func f, void* ud) {
    struct coro* co = malloc(sizeof(*co));
    co->f = f;
    co->ud = ud;
    co->sch = S;
    co->cap = 0;
    co->size = 0;
    co->status = CO_READY;
    co->stack = NULL;
    return co;
}

void _co_del(struct coro* co) {
    free(co->stack);
    free(co);
}

struct schedule* co_open(void) {
    struct schedule* S = malloc(sizeof(*S));  // sizeof(*S) ??
    S->nco = 0;
    S->cap = DEFAULT_COROUTINE;
    S->running_id = -1;
    S->co_arr = malloc(sizeof(struct coro*) * S->cap);
    memset(S->co_arr, 0, sizeof(struct coro*) * S->cap);
    return S;
}

void co_close(struct schedule* S) {
    struct coro* cor = NULL;
    int i;
    for (i = 0; i < S->cap; i++) {
        cor = S->co_arr[i];
        if (cor) {
            _co_del(cor);
        }
    }
    free(S->co_arr);
    S->co_arr = NULL;
    free(S);
}

int co_new(struct schedule* S, co_func f, void* ud) {
    struct coro* co = _co_new(S, f, ud);
    if (S->nco >= S->cap) {
        // 扩容
        int id = S->cap;
        S->co_arr = realloc(S->co_arr, S->cap * 2 * sizeof(struct coro*));
        memset(S->co_arr + S->cap, 0, sizeof(struct coro*) * S->cap);
        S->co_arr[S->cap] = co;  // ??
        S->cap *= 2;
        ++S->nco;
        return id;
    } else {
        int i;
        for (i = 0; i < S->cap; i++) {
            int id = (i + S->nco) % S->cap;
            if (S->co_arr[id] == NULL) {
                S->co_arr[id] = co;
                ++S->nco;
                return id;
            }
        }
    }
    assert(0);
    return -1;
}

static void mainfunc(uint32_t low32, uint32_t hi32) {
    printf("mainfunc\n");
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    struct schedule* S = (struct schedule*)ptr;
    int id = S->running_id;
    struct coro* co = S->co_arr[id];
    co->f(S, co->ud); // 开始执行协程函数
    _co_del(co);  // 执行完后释放协程对象
    S->co_arr[id] = NULL;
    --S->nco;
    S->running_id = -1;
}

void co_resume(struct schedule* S, int id) {
    assert(S->running_id == -1);
    assert(id >= 0 && id < S->cap);

    // get coroutine
    struct coro* co = S->co_arr[id];
    if (co == NULL)
        return;
    int status = co->status;
    switch(status) {
        case CO_READY:
            getcontext(&co->ctx);
            co->ctx.uc_stack.ss_sp = S->stack;
            co->ctx.uc_stack.ss_size = STACK_SIZE;
            co->ctx.uc_link = &S->main;
            S->running_id = id;
            co->status = CO_RUNNING;
            uintptr_t ptr = (uintptr_t)S;
            makecontext(&co->ctx, (void(*)(void)) mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
            swapcontext(&S->main, &co->ctx);
            break;
        case CO_SUSPEND:
            memcpy(S->stack + STACK_SIZE - co->size, co->stack, co->size);
            S->running_id = id;
            co->status = CO_RUNNING;
            swapcontext(&S->main, &co->ctx);
            break;
        default:
            assert(0); 
    }
}

static void _save_stack(struct coro* co, char* top) {
    char dummy = 0;
    assert(top - &dummy <= STACK_SIZE);
    if (co->cap < top - &dummy) {
        free(co->stack);
        co->cap = top - &dummy;
        co->stack = malloc(co->cap);
    }
    co->size = top - &dummy;
    memcpy(co->stack, &dummy, co->size);
}

void co_yield(struct schedule* S) {
    int id = S->running_id;
    assert(id >= 0);

    struct coro* co = S->co_arr[id];
    assert((char*)&co > S->stack);

    _save_stack(co, S->stack + STACK_SIZE);

    co->status = CO_SUSPEND;
    S->running_id = -1;

    swapcontext(&co->ctx, &S->main);
}

int co_status(struct schedule* S, int id) {
    assert(id >= 0 && id <= S->cap);
    if (S->co_arr[id] == NULL) {
        return CO_DEAD;
    }
    return S->co_arr[id]->status;
}

int co_running(struct schedule* S) {
    return S->running_id;
}

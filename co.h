#ifndef __CO_H
#define __CO_H

#define CO_DEAD     0
#define CO_READY    1
#define CO_RUNNING  2
#define CO_SUSPEND  3


struct schedule;

typedef void (*co_func)(struct schedule*, void* ud);

struct schedule* co_open(void);
void co_close(struct schedule*);

int co_new(struct schedule*, co_func, void* ud);
void co_resume(struct schedule*, int fd);
int co_status(struct schedule*, int fd);
int co_running(struct schedule*);
void co_yield(struct schedule*);

#endif // __CO_H
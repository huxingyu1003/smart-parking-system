#ifndef __CAR_SERVE_H__
#define __CAR_SERVE_H__

void *net_serve(void *arg);
int net_status_take(char *msg, int size, int *online);

#endif
#ifndef __carnum_H__
#define __carnum_H__

typedef struct carplatnum
{
    int car_Park_num;
    char plat_num[16];
    int time_entry[5];
    int time_exit[5];
    struct carplatnum *next;
    struct carplatnum *prev;
}carplatnum;

typedef struct carlist
{
    carplatnum *first;
    carplatnum *last;
}carlist;

typedef struct systime
{
    int y;
    int mo;
    int d;
    int h;
    int mi;
    int s;
}systime;

extern carlist car;
extern systime g_time;
extern int actual_time_entry[5];
extern int actual_time_exit[5];
extern int g_last_fee_x100;

void time_tick(void);
int calc_fee_x100(int en[5], int ex[5]);
int car_add(const char *plate, int park_id);
carplatnum *car_find_plate(const char *plate);
carplatnum *car_find_park(int park_id);
int free_park_id(void);
int car_count(void);
int car_del(const char *plate);

int income_today_x100(void);
int order_count(void);

int car_snapshot(int *parks, char (*plates)[16], int (*entries)[5], int max);
int car_find_plate_copy(const char *plate, int *park, char (*plate_out)[16], int (*entry)[5]);

int file_forsave(void);
int file_load(void);

#endif
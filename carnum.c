#include "carnum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


carlist car;
systime g_time;
int actual_time_entry[5];
int actual_time_exit[5];
int g_last_fee_x100 = 0;

static pthread_mutex_t g_car_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_order_count = 0;
static int g_today_income_x100 = 0;
static int g_income_day[3] = {0,0,0};
static void income_check_day(void); 

static int days_in_month(int y, int m)
{
    static const int dm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if(m == 2 && (y % 4 == 0 && y % 100 != 0 || y % 400 == 0))
    {
        return 29;
    }
    return dm[m - 1];
}

void time_tick(void)
{
    g_time.s++;
    if(g_time.s >= 60)
    {
        g_time.s = 0; g_time.mi++;
    }
    if(g_time.mi >= 60)
    {
        g_time.mi = 0; g_time.h++;
    }
    if(g_time.h >= 24)
    {
        g_time.h = 0; g_time.d++;
    }
    if(g_time.d > days_in_month(g_time.y, g_time.mo))
    {
        g_time.d = 1; g_time.mo++;
    }
    if(g_time.mo > 12)
    {
        g_time.mo = 1; g_time.y++;
    }
}

static long long t2min(int t[5])
{
    long long days = 0;
    for(int y = 2000; y < t[0]; y++)
    {
        days += 365 + (y % 4 == 0 && y % 100 != 0 || y % 400 == 0);
    }
    for(int m = 1; m < t[1]; m++)
    {
        days += days_in_month(t[0], m);
    }
    days += t[2] - 1;
    return days * 1440 + t[3] * 60 + t[4];
}

int calc_fee_x100(int en[5], int ex[5])
{
    long long mins = t2min(ex) - t2min(en);
    if(mins < 0) mins = 0;
    if(mins <= 15) return 0;
    int hours = (int)((mins + 59) / 60);
    int fee = 500 + (hours - 1) * 100;
    if(fee > 2800) fee = 2800;
    return fee;
}

static carplatnum *car_find_plate_nolock(const char *plate)
{
    carplatnum *pk = car.first;
    while(pk != NULL)
    {
        if(strcmp(pk->plat_num, plate) == 0)
            return pk;
        pk = pk->next;
    }
    return NULL;
}

static carplatnum *car_find_park_nolock(int park_id)
{
    carplatnum *pk = car.first;
    while(pk != NULL)
    {
        if(pk->car_Park_num == park_id)
            return pk;
        pk = pk->next;
    }
    return NULL;
}

static int car_count_nolock(void)
{
    int n = 0;
    carplatnum *pk = car.first;
    while(pk != NULL) { n++; pk = pk->next; }
    return n;
}

carplatnum *car_find_plate(const char *plate)
{
    if(plate == NULL) return NULL;
    pthread_mutex_lock(&g_car_mutex);
    carplatnum *pk = car_find_plate_nolock(plate);
    pthread_mutex_unlock(&g_car_mutex);
    return pk;
}

carplatnum *car_find_park(int park_id)
{
    pthread_mutex_lock(&g_car_mutex);
    carplatnum *pk = car_find_park_nolock(park_id);
    pthread_mutex_unlock(&g_car_mutex);
    return pk;
}

int free_park_id(void)
{
    pthread_mutex_lock(&g_car_mutex);
    for(int i = 1; i <= 8; i++)
    {
        if(car_find_park_nolock(i) == NULL)
        {
            pthread_mutex_unlock(&g_car_mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&g_car_mutex);
    return -1;
}

int car_count(void)
{
    pthread_mutex_lock(&g_car_mutex);
    int n = car_count_nolock();
    pthread_mutex_unlock(&g_car_mutex);
    return n;
}

int car_add(const char *plate, int park_id)
{
    if(plate == NULL || plate[0] == 0) return -1;
    if(park_id < 1 || park_id > 8) return -1;

    pthread_mutex_lock(&g_car_mutex);

    if(car_find_plate_nolock(plate) != NULL)
    {
        pthread_mutex_unlock(&g_car_mutex);
        return -2;
    }
    if(car_find_park_nolock(park_id) != NULL)
    {
        pthread_mutex_unlock(&g_car_mutex);
        return -3;
    }

    carplatnum *pnew = (carplatnum *)malloc(sizeof(carplatnum));
    if(pnew == NULL)
    {
        pthread_mutex_unlock(&g_car_mutex);
        return -1;
    }
    memset(pnew, 0, sizeof(carplatnum));

    pnew->car_Park_num = park_id;
    strncpy(pnew->plat_num, plate, 15);
    pnew->plat_num[15] = '\0';

    pnew->time_entry[0] = g_time.y;
    pnew->time_entry[1] = g_time.mo;
    pnew->time_entry[2] = g_time.d;
    pnew->time_entry[3] = g_time.h;
    pnew->time_entry[4] = g_time.mi;

    pnew->next = NULL;
    pnew->prev = NULL;

    if(car.first == NULL)
    {
        car.first = pnew;
        car.last = pnew;
    }
    else
    {
        carplatnum *pk = car.first;
        while(pk != NULL && pk->car_Park_num < park_id)
            pk = pk->next;

        if(pk == NULL)
        {
            car.last->next = pnew;
            pnew->prev = car.last;
            car.last = pnew;
        }
        else if(pk == car.first)
        {
            pnew->next = car.first;
            car.first->prev = pnew;
            car.first = pnew;
        }
        else
        {
            pnew->next = pk;
            pnew->prev = pk->prev;
            pk->prev->next = pnew;
            pk->prev = pnew;
        }
    }

    pthread_mutex_unlock(&g_car_mutex);
    file_forsave();
    return 0;
}

int car_del(const char *plate)
{
    if(plate == NULL) return -1;

    pthread_mutex_lock(&g_car_mutex);

    carplatnum *pk = car_find_plate_nolock(plate);
    if(pk == NULL)
    {
        pthread_mutex_unlock(&g_car_mutex);
        return -1;
    }

    pk->time_exit[0] = g_time.y;
    pk->time_exit[1] = g_time.mo;
    pk->time_exit[2] = g_time.d;
    pk->time_exit[3] = g_time.h;
    pk->time_exit[4] = g_time.mi;

    for(int i = 0; i < 5; i++)
    {
        actual_time_entry[i] = pk->time_entry[i];
        actual_time_exit[i] = pk->time_exit[i];
    }

    int fee = calc_fee_x100(pk->time_entry, pk->time_exit);
    g_last_fee_x100 = fee;

    income_check_day();
    g_today_income_x100 += fee;
    g_order_count++;

    if(pk->prev != NULL) pk->prev->next = pk->next;
    else car.first = pk->next;

    if(pk->next != NULL) pk->next->prev = pk->prev;
    else car.last = pk->prev;

    free(pk);

    pthread_mutex_unlock(&g_car_mutex);
    file_forsave();
    return fee;
}

static void income_check_day(void)
{
    if(g_income_day[0] != g_time.y ||
       g_income_day[1] != g_time.mo ||
       g_income_day[2] != g_time.d)
    {
        g_income_day[0] = g_time.y;
        g_income_day[1] = g_time.mo;
        g_income_day[2] = g_time.d;
        g_today_income_x100 = 0;
    }
}

int income_today_x100(void)
{
    pthread_mutex_lock(&g_car_mutex);
    income_check_day();
    int v = g_today_income_x100;
    pthread_mutex_unlock(&g_car_mutex);
    return v;
}

int order_count(void)
{
    pthread_mutex_lock(&g_car_mutex);
    int v = g_order_count;
    pthread_mutex_unlock(&g_car_mutex);
    return v;
}

int car_snapshot(int *parks, char (*plates)[16], int (*entries)[5], int max)
{
    if(parks == NULL || plates == NULL || entries == NULL || max <= 0)
        return 0;

    pthread_mutex_lock(&g_car_mutex);

    int n = 0;
    carplatnum *pk = car.first;
    while(pk != NULL && n < max)
    {
        parks[n] = pk->car_Park_num;
        strncpy(plates[n], pk->plat_num, 15);
        plates[n][15] = '\0';
        for(int i = 0; i < 5; i++)
            entries[n][i] = pk->time_entry[i];
        n++;
        pk = pk->next;
    }

    pthread_mutex_unlock(&g_car_mutex);
    return n;
}

int car_find_plate_copy(const char *plate, int *park, char (*plate_out)[16], int (*entry)[5])
{
    if(plate == NULL) return 0;

    pthread_mutex_lock(&g_car_mutex);

    carplatnum *pk = car_find_plate_nolock(plate);
    if(pk == NULL)
    {
        pthread_mutex_unlock(&g_car_mutex);
        return 0;
    }

    if(park != NULL) *park = pk->car_Park_num;

    if(plate_out != NULL)
    {
        strncpy(*plate_out, pk->plat_num, 15);
        (*plate_out)[15] = '\0';
    }

    if(entry != NULL)
    {
        for(int i = 0; i < 5; i++)
            (*entry)[i] = pk->time_entry[i];
    }

    pthread_mutex_unlock(&g_car_mutex);
    return 1;
}

#define DATA_DIR       "/userdata"
#define DATA_FILE      DATA_DIR "/park.dat"
#define TMP_FILE       DATA_DIR "/park.dat.tmp"
#define DATA_VERSION   1

static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    size_t done = 0;
    while (done < len)
    {
        ssize_t n = write(fd, p + done, len - done);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

static uint16_t crc16_update(uint16_t crc, const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)p[i] << 8;
        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static int put_bytes(int fd, uint16_t *crc, const void *buf, size_t len)
{
    if (write_all(fd, buf, len) != 0) return -1;
    *crc = crc16_update(*crc, buf, len);
    return 0;
}

int file_forsave(void)
{
    int ret = 0;

    pthread_mutex_lock(&g_car_mutex);

    income_check_day();
    uint16_t version      = DATA_VERSION;
    uint16_t count        = (uint16_t)car_count_nolock();
    int32_t  today_income = g_today_income_x100;
    int32_t  order_cnt    = g_order_count;
    int32_t  income_day[3] = { g_income_day[0], g_income_day[1], g_income_day[2] };
    int32_t  clk[6] = { g_time.y, g_time.mo, g_time.d, g_time.h, g_time.mi, g_time.s };

    int fd = open(TMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("open tmp error");
        pthread_mutex_unlock(&g_car_mutex);
        return -1;
    }

    uint16_t crc = 0xFFFF;

    char magic[4] = {'P', 'A', 'R', 'K'};
    if (put_bytes(fd, &crc, magic, 4) != 0)
    { perror("write magic error"); ret = -1; goto out; }

    if (put_bytes(fd, &crc, &version, 2) != 0)
    { perror("write version error"); ret = -1; goto out; }
    if (put_bytes(fd, &crc, &count, 2) != 0)
    { perror("write count error"); ret = -1; goto out; }

    if (put_bytes(fd, &crc, &today_income, 4) != 0)
    { perror("write today income error"); ret = -1; goto out; }
    if (put_bytes(fd, &crc, &order_cnt, 4) != 0)
    { perror("write order count error"); ret = -1; goto out; }

    if (put_bytes(fd, &crc, income_day, sizeof(income_day)) != 0)
    { perror("write income day error"); ret = -1; goto out; }

    if (put_bytes(fd, &crc, clk, sizeof(clk)) != 0)
    { perror("write clock error"); ret = -1; goto out; }

    carplatnum *pk = car.first;
    while (pk != NULL)
    {
        int32_t park_id = pk->car_Park_num;
        char    plate[16];
        int32_t entry[5];

        memset(plate, 0, sizeof(plate));
        strncpy(plate, pk->plat_num, 15);
        plate[15] = '\0';
        for (int i = 0; i < 5; i++)
            entry[i] = pk->time_entry[i];

        if (put_bytes(fd, &crc, &park_id, 4) != 0)
        { perror("write park id error"); ret = -1; goto out; }
        if (put_bytes(fd, &crc, plate, 16) != 0)
        { perror("write plate error"); ret = -1; goto out; }
        if (put_bytes(fd, &crc, entry, sizeof(entry)) != 0)
        { perror("write entry error"); ret = -1; goto out; }

        pk = pk->next;
    }

    if (write_all(fd, &crc, sizeof(crc)) != 0)
    { perror("write crc error"); ret = -1; goto out; }

    if(fsync(fd) == -1)
    {
        perror("fsync error!");
        ret = -1;
        goto out;
    }

    int r = rename(TMP_FILE, DATA_FILE);
    if (r == -1)
    {
        perror("rename error");
        ret = -1;
        goto out;
    }
    int dfd = open(DATA_DIR, O_RDONLY | O_DIRECTORY);

    if (dfd == -1)
    {
        perror("open dir error");
        ret = -1;
        goto out;
    }

    if (fsync(dfd) == -1)
    {
        perror("fsync dir error");
        ret = -1;
    }
    close(dfd);

out:
    close(fd);
    pthread_mutex_unlock(&g_car_mutex);
    return ret;
}

static int read_all(int fd, void *buf, size_t len)
{
    char *p = (char *)buf;
    size_t done = 0;
    while (done < len)
    {
        ssize_t n = read(fd, p + done, len - done);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        done += (size_t)n;
    }
    return 0;
}

static int get_bytes(int fd, uint16_t *crc, void *buf, size_t len)
{
    if (read_all(fd, buf, len) != 0) return -1;
    *crc = crc16_update(*crc, buf, len);
    return 0;
}

int file_load(void)
{
    int fd = open(DATA_FILE, O_RDONLY);
    if (fd == -1)
    {
        if (errno != ENOENT)
            perror("open data file error");
        return 0;
    }

    char     magic[4];
    uint16_t version = 0;
    uint16_t count   = 0;
    int32_t  today_income = 0;
    int32_t  order_cnt    = 0;
    int32_t  income_day[3] = {0, 0, 0};
    int32_t  clk[6]       = {0};
    uint16_t crc        = 0xFFFF;
    uint16_t crc_stored = 0;

    if (get_bytes(fd, &crc, magic, 4) != 0 ||
        get_bytes(fd, &crc, &version, 2) != 0 ||
        get_bytes(fd, &crc, &count, 2) != 0 ||
        get_bytes(fd, &crc, &today_income, 4) != 0 ||
        get_bytes(fd, &crc, &order_cnt, 4) != 0 ||
        get_bytes(fd, &crc, income_day, sizeof(income_day)) != 0 ||
        get_bytes(fd, &crc, clk, sizeof(clk)) != 0)
    {
        perror("read header error");
        goto fail;
    }

    if (memcmp(magic, "PARK", 4) != 0)
    {
        fprintf(stderr, "bad magic\n");
        goto fail;
    }
    if (version != DATA_VERSION || count > 8 ||
        today_income < 0 || order_cnt < 0)
    {
        fprintf(stderr, "bad header: version=%u count=%u\n", version, count);
        goto fail;
    }

    int32_t parks[8];
    char    plates[8][16];
    int32_t entries[8][5];

    for (uint16_t i = 0; i < count; i++)
    {
        int32_t park_id = 0;
        char    plate[16];
        int32_t entry[5];
        if (get_bytes(fd, &crc, &park_id, 4) != 0 ||
            get_bytes(fd, &crc, plate, 16) != 0 ||
            get_bytes(fd, &crc, entry, sizeof(entry)) != 0)
        {
            perror("read record error");
            goto fail;
        }
        if (park_id < 1 || park_id > 8)
        {
            fprintf(stderr, "bad park id %d\n", (int)park_id);
            goto fail;
        }
        parks[i] = park_id;
        memcpy(plates[i], plate, 16);
        memcpy(entries[i], entry, sizeof(entry));
    }

    if (read_all(fd, &crc_stored, 2) != 0)
    {
        perror("read crc error");
        goto fail;
    }

    char tail;
    if (read(fd, &tail, 1) != 0)
    {
        fprintf(stderr, "file size mismatch\n");
        goto fail;
    }

    if (crc != crc_stored)
    {
        fprintf(stderr, "crc mismatch: calc=%04x stored=%04x\n", crc, crc_stored);
        goto fail;
    }

    close(fd);

    pthread_mutex_lock(&g_car_mutex);

    g_today_income_x100 = today_income;
    g_order_count       = order_cnt;
    g_income_day[0] = income_day[0];
    g_income_day[1] = income_day[1];
    g_income_day[2] = income_day[2];
    g_time.y = clk[0]; g_time.mo = clk[1]; g_time.d = clk[2];
    g_time.h = clk[3]; g_time.mi = clk[4]; g_time.s = clk[5];

    for (uint16_t i = 0; i < count; i++)
    {
        carplatnum *pnew = (carplatnum *)malloc(sizeof(carplatnum));
        if (pnew == NULL)
        {
            perror("malloc error");
            carplatnum *t = car.first;
            while (t != NULL)
            {
                carplatnum *nx = t->next;
                free(t);
                t = nx;
            }
            car.first = NULL;
            car.last  = NULL;
            pthread_mutex_unlock(&g_car_mutex);
            return -1;
        }
        memset(pnew, 0, sizeof(carplatnum));
        pnew->car_Park_num = parks[i];
        memcpy(pnew->plat_num, plates[i], 16);
        for (int j = 0; j < 5; j++)
            pnew->time_entry[j] = entries[i][j];
        pnew->next = NULL;
        pnew->prev = NULL;

        if (car.first == NULL)
        {
            car.first = pnew;
            car.last  = pnew;
        }
        else
        {
            car.last->next = pnew;
            pnew->prev = car.last;
            car.last = pnew;
        }
    }

    pthread_mutex_unlock(&g_car_mutex);
    printf("state loaded: %u cars, today %d fen, %04d-%02d-%02d %02d:%02d:%02d\n",
           count, (int)today_income,
           g_time.y, g_time.mo, g_time.d, g_time.h, g_time.mi, g_time.s);
    return 0;

fail:
    close(fd);
    fprintf(stderr, "data file invalid, start with default empty state\n");
    return -1;
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>

#include "car_serve.h"
#include "carnum.h"

#define SERVER_PORT     8888
#define SERVER_BACKLOG  8

static pthread_mutex_t s_net_mutex = PTHREAD_MUTEX_INITIALIZER;
static int  s_net_online_count = 0;
static int  s_net_flag         = 0;
static int  s_net_msg_flag     = 0;
static char s_net_msg[256]     = {0};

static void net_online_add(int delta)
{
    pthread_mutex_lock(&s_net_mutex);
    s_net_online_count += delta;
    if(s_net_online_count < 0) s_net_online_count = 0;
    s_net_flag = (s_net_online_count > 0) ? 1 : 0;
    pthread_mutex_unlock(&s_net_mutex);
}

static void net_msg_set(const char *msg)
{
    pthread_mutex_lock(&s_net_mutex);
    if(msg == NULL) msg = "";
    strncpy(s_net_msg, msg, sizeof(s_net_msg) - 1);
    s_net_msg[sizeof(s_net_msg) - 1] = '\0';
    s_net_msg_flag = 1;
    pthread_mutex_unlock(&s_net_mutex);
}

int net_status_take(char *msg, int size, int *online)
{
    int has = 0;

    pthread_mutex_lock(&s_net_mutex);

    if(online != NULL) *online = s_net_online_count;

    if(msg != NULL && size > 0)
    {
        msg[0] = '\0';
        if(s_net_msg_flag)
        {
            strncpy(msg, s_net_msg, size - 1);
            msg[size - 1] = '\0';
            s_net_msg_flag = 0;
            has = 1;
        }
    }

    pthread_mutex_unlock(&s_net_mutex);
    return has;
}

static void trim_line(char *s)
{
    int len = (int)strlen(s);

    while(len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n' ||
                      s[len - 1] == ' '  || s[len - 1] == '\t'))
    {
        s[--len] = '\0';
    }

    char *p = s;
    while(*p == ' ' || *p == '\t') p++;
    if(p != s) memmove(s, p, strlen(p) + 1);
}

static void upper_str(char *s)
{
    for(; *s; s++)
        *s = (char)toupper((unsigned char)*s);
}

static int send_all(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while(sent < len)
    {
        ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);
        if(n < 0)
        {
            if(errno == EINTR) continue;
            return -1;
        }
        if(n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int send_line(int fd, const char *line)
{
    if(send_all(fd, line, strlen(line)) < 0) return -1;
    if(send_all(fd, "\n", 1) < 0) return -1;
    return 0;
}

static int send_end_mark(int fd)
{
    return send_line(fd, "###END###");
}

static int handle_cmd(int fd, char *line)
{
    trim_line(line);
    if(line[0] == '\0') return 1;

    upper_str(line);

    if(strcmp(line, "QUIT") == 0)
    {
        return 0;
    }

    if(strcmp(line, "STATUS") == 0)
    {
        int parks[8];
        char plates[8][16];
        int entries[8][5];

        int used = car_snapshot(parks, plates, entries, 8);

        char buf[128];
        snprintf(buf, sizeof(buf), "总车位:8 已用:%d 空闲:%d", used, 8 - used);
        if(send_line(fd, buf) < 0) return 0;

        for(int i = 0; i < used; i++)
        {
            snprintf(buf, sizeof(buf), "%d %s 入场:%04d-%02d-%02d %02d:%02d", parks[i], plates[i], entries[i][0], entries[i][1], entries[i][2], entries[i][3], entries[i][4]);
            if(send_line(fd, buf) < 0) return 0;
        }

        if(send_end_mark(fd) < 0) return 0;
        return 1;
    }

    if(strcmp(line, "REPORT") == 0)
    {
        int fen    = income_today_x100();
        int used   = car_count();
        int orders = order_count();

        char buf[160];
        snprintf(buf, sizeof(buf), "今日收入:%d.%02d元 在场:%d辆 累计订单:%d单", fen / 100, fen % 100, used, orders);

        net_msg_set(buf);

        if(send_line(fd, buf) < 0) return 0;
        if(send_end_mark(fd) < 0) return 0;
        return 1;
    }

    if(strncmp(line, "FIND", 4) == 0 && (line[4] == ' ' || line[4] == '\0'))
    {
        char *plate = line + 4;
        while(*plate == ' ') plate++;

        if(*plate == '\0')
        {
            if(send_line(fd, "usage: FIND 车牌") < 0) return 0;
            if(send_end_mark(fd) < 0) return 0;
            return 1;
        }

        int  park = 0;
        char plate_out[16];
        int  entry[5];

        if(car_find_plate_copy(plate, &park, &plate_out, &entry))
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "%d/%s/%04d-%02d-%02d %02d:%02d", park, plate_out, entry[0], entry[1], entry[2], entry[3], entry[4]);
            if(send_line(fd, buf) < 0) return 0;
        }
        else
        {
            if(send_line(fd, "not found") < 0) return 0;
        }

        if(send_end_mark(fd) < 0) return 0;
        return 1;
    }

    if(send_line(fd, "unknown command") < 0) return 0;
    if(send_end_mark(fd) < 0) return 0;
    return 1;
}

static void *net_client_thread(void *arg)
{
    int fd = (int)(intptr_t)arg;

    net_online_add(1);

    char   rbuf[512];
    size_t len = 0;

    while(1)
    {
        char tmp[256];
        ssize_t n = recv(fd, tmp, sizeof(tmp), 0);

        if(n == 0)
        {
            break;
        }
        if(n < 0)
        {
            if(errno == EINTR) continue;
            break;
        }

        for(ssize_t i = 0; i < n; i++)
        {
            char c = tmp[i];

            if(c == '\n')
            {
                rbuf[len] = '\0';
                len = 0;

                if(handle_cmd(fd, rbuf) == 0)
                {
                    close(fd);
                    net_online_add(-1);
                    return NULL;
                }
            }
            else
            {
                if(len < sizeof(rbuf) - 1)
                    rbuf[len++] = c;
                else
                    len = 0;
            }
        }
    }

    close(fd);
    net_online_add(-1);
    return NULL;
}

void *net_serve(void *arg)
{
    (void)arg;

    signal(SIGPIPE, SIG_IGN);

    int sk = socket(AF_INET, SOCK_STREAM, 0);
    if(sk < 0)
    {
        perror("socket error");
        return NULL;
    }

    int one = 1;
    setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in serve;
    memset(&serve, 0, sizeof(serve));
    serve.sin_family      = AF_INET;
    serve.sin_port        = htons(SERVER_PORT);
    serve.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sk, (struct sockaddr *)&serve, sizeof(serve)) < 0)
    {
        perror("bind error");
        close(sk);
        return NULL;
    }

    if(listen(sk, SERVER_BACKLOG) < 0)
    {
        perror("listen error");
        close(sk);
        return NULL;
    }

    printf("[net_serve] listening on 0.0.0.0:%d\n", SERVER_PORT);
    fflush(stdout);

    while(1)
    {
        struct sockaddr_in cli;
        socklen_t cli_len = sizeof(cli);

        int fd = accept(sk, (struct sockaddr *)&cli, &cli_len);
        if(fd < 0)
        {
            if(errno == EINTR) continue;
            perror("accept error");
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        printf("[net_serve] client connected: %s:%d\n", ip, ntohs(cli.sin_port));
        fflush(stdout);

        pthread_t tid;
        if(pthread_create(&tid, NULL, net_client_thread, (void *)(intptr_t)fd) != 0)
        {
            perror("pthread_create error");
            close(fd);
            continue;
        }
        pthread_detach(tid);
    }

    close(sk);
    return NULL;
}
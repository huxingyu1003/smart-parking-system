#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define RECV_TIMEOUT_SEC 5

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

static void strip_crlf(char *s)
{
    int len = (int)strlen(s);
    while(len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n' || s[len - 1] == ' '))
        s[--len] = '\0';
}

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("用法: %s <服务器IP> <端口>\n", argv[0]);
        return -1;
    }

    signal(SIGPIPE, SIG_IGN);

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    if(port <= 0 || port > 65535)
    {
        printf("端口非法: %s\n", argv[2]);
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        perror("socket error");
        return -1;
    }

    struct timeval tv;
    tv.tv_sec  = RECV_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((unsigned short)port);
    if(inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        printf("IP 地址非法: %s\n", ip);
        close(fd);
        return -1;
    }

    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect error");
        close(fd);
        return -1;
    }

    printf("已连接 %s:%d，支持命令：STATUS / REPORT / FIND 车牌 / QUIT\n", ip, port);

    char line[256];

    while(1)
    {
        printf("> ");
        fflush(stdout);

        if(fgets(line, sizeof(line), stdin) == NULL)
            break;

        strip_crlf(line);
        if(line[0] == '\0')
            continue;

        if(send_all(fd, line, strlen(line)) < 0 ||
           send_all(fd, "\n", 1) < 0)
        {
            perror("send error");
            break;
        }

        if(strcasecmp(line, "QUIT") == 0)
        {
            printf("已退出。\n");
            break;
        }

        int    server_closed = 0;
        int    round_done    = 0;
        char   rbuf[2048];
        size_t rlen = 0;

        while(!round_done)
        {
            char tmp[512];
            ssize_t n = recv(fd, tmp, sizeof(tmp), 0);

            if(n == 0)
            {
                printf("\n服务器已断开连接。\n");
                server_closed = 1;
                break;
            }
            if(n < 0)
            {
                if(errno == EINTR) continue;
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    printf("\n[超时] 未收到完整响应。\n");
                else
                    perror("recv error");
                break;
            }

            for(ssize_t i = 0; i < n; i++)
            {
                char c = tmp[i];
                if(c == '\n')
                {
                    rbuf[rlen] = '\0';
                    rlen = 0;
                    strip_crlf(rbuf);

                    if(strcmp(rbuf, "###END###") == 0)
                    {
                        round_done = 1;
                        break;
                    }
                    printf("%s\n", rbuf);
                }
                else
                {
                    if(rlen < sizeof(rbuf) - 1)
                        rbuf[rlen++] = c;
                    else
                        rlen = 0;
                }
            }
        }

        if(server_closed)
            break;
    }

    close(fd);
    return 0;
}
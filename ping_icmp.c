#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#define ICMP_HEADER_LENGTH 8
#define MESSAGE_BUFFER_SIZE 1024

#define ICMP_ECHO 8
#define ICMP_ECHO_REPLY 0
#define ICMP6_ECHO_REPLY 129

#define REQUEST_TIMEOUT 1000000
#define REQUEST_INTERVAL 1000000

#pragma pack(push, 1)
struct icmp
{
    uint8_t icmp_type;
    uint8_t icmp_code;
    uint16_t icmp_cksum;
    uint16_t icmp_id;
    uint16_t icmp_seq;
};

#pragma pack(pop)

static LPFN_WSARECVMSG WSARecvMsg;
static void WinsockErrorHandler(const char *s);
static uint64_t utime(void);
static void WindowsSocketInitialize(void);
static void WinsockExtensionInitialize(SOCKET sockfd);
static uint16_t ComputeChecksum(const char *buf, size_t size);

int main(int argc, const char *argv[])
{
    WindowsSocketInitialize();

    const char *hostname = "soft98.ir";
    struct addrinfo *addrinfo_list = NULL;
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;
    int error = getaddrinfo(hostname, NULL, &hints, &addrinfo_list);

    if (error != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        goto exit_error;
    }

    struct addrinfo *addrinfo;
    SOCKET sockfd = -1;
    for (addrinfo = addrinfo_list; addrinfo != NULL; addrinfo = addrinfo->ai_next)
    {
        sockfd = WSASocketW(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol, NULL, 0, 0);
        if (sockfd >= 0)
            break;
    }

    if ((int)sockfd < 0)
    {
        WinsockErrorHandler("socket");
        goto exit_error;
    }

    struct sockaddr_storage addr;
    memcpy(&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
    socklen_t dst_addr_len = (socklen_t)addrinfo->ai_addrlen;

    freeaddrinfo(addrinfo_list);
    addrinfo = NULL;
    addrinfo_list = NULL;

    WinsockExtensionInitialize(sockfd);

    char addr_str[INET6_ADDRSTRLEN] = {0};
    inet_ntop(addr.ss_family, (void *)&((struct sockaddr_in *)&addr)->sin_addr, addr_str, sizeof(addr_str));

    printf("PING %s (%s)\n", hostname, addr_str);

    uint16_t id = (uint16_t)_getpid();
    uint64_t start_time = 0;
    uint64_t delay = 0;
    for (uint16_t seq = 0;; seq++)
    {
        struct icmp request;

        request.icmp_type = ICMP_ECHO;
        request.icmp_code = 0;
        request.icmp_cksum = 0;
        request.icmp_id = htons(id);
        request.icmp_seq = htons(seq);
        request.icmp_cksum = ComputeChecksum((char *)&request, sizeof(request));

        error = (int)sendto(sockfd, (char *)&request, sizeof(request), 0, (struct sockaddr *)&addr, (int)dst_addr_len);
        if (error < 0)
        {
            WinsockErrorHandler("sendto");
            goto exit_error;
        }

        start_time = utime();

        for (;;)
        {
            char msg_buf[MESSAGE_BUFFER_SIZE] = {0};
            char packet_info_buf[MESSAGE_BUFFER_SIZE] = {0};

            WSABUF msg_buf_struct = {sizeof(msg_buf), msg_buf};
            WSAMSG msg = {NULL, 0, &msg_buf_struct, 1, {sizeof(packet_info_buf), packet_info_buf}, 0};
            DWORD msg_len = 0;

            error = WSARecvMsg(sockfd, &msg, &msg_len, NULL, NULL);

            delay = utime() - start_time;

            if (error < 0)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    if (delay > REQUEST_TIMEOUT)
                    {
                        printf("Request timed out\n");
                        goto next;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    WinsockErrorHandler("recvmsg");
                    goto next;
                }
            }

            size_t ip_hdr_len = ((*(uint8_t *)msg_buf) & 0x0F) * 4;

            struct icmp *reply = (struct icmp *)(msg_buf + ip_hdr_len);
            int reply_id = ntohs(reply->icmp_id);
            int reply_seq = ntohs(reply->icmp_seq);

            if (addr.ss_family != AF_INET || reply->icmp_type != ICMP_ECHO_REPLY)
                continue;
            if (reply_id != id || reply_seq != seq)
                continue;

            uint16_t reply_checksum = reply->icmp_cksum;
            reply->icmp_cksum = 0;

            uint16_t checksum = ComputeChecksum(msg_buf + ip_hdr_len, msg_len - ip_hdr_len);

            printf("Received reply from %s: seq=%d, time=%.3f ms%s\n", addr_str, seq, (double)delay / 1000.0, reply_checksum != checksum ? " (bad checksum)" : "");
            break;
        }

    next:
        if (delay < REQUEST_INTERVAL)
            Sleep((DWORD)((REQUEST_INTERVAL - delay) / 1000));
    }

    closesocket(sockfd);
    return EXIT_SUCCESS;

exit_error:
    if (addrinfo_list != NULL)
        freeaddrinfo(addrinfo_list);
    closesocket(sockfd);

    return EXIT_FAILURE;
}

void WinsockErrorHandler(const char *s)
{
    char *message = NULL;
    DWORD format_flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK;
    DWORD result;

    result = FormatMessageA(format_flags, NULL, WSAGetLastError(), 0, (char *)&message, 0, NULL);
    if (result > 0)
    {
        fprintf(stderr, "%s: %s\n", s, message);
        LocalFree(message);
    }
    else
    {
        fprintf(stderr, "%s: Unknown error\n", s);
    }
}

uint64_t utime(void)
{
    LARGE_INTEGER count;
    LARGE_INTEGER frequency;
    if (QueryPerformanceCounter(&count) == 0 || QueryPerformanceFrequency(&frequency) == 0)
        return 0;
    return count.QuadPart * 1000000 / frequency.QuadPart;
}

void WindowsSocketInitialize(void)
{
    int error;
    WSADATA wsa_data;

    error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (error != 0)
    {
        fprintf(stderr, "Failed to initialize WinSock: %d\n", error);
        exit(EXIT_FAILURE);
    }
}

void WinsockExtensionInitialize(SOCKET sockfd)
{
    int error;
    GUID recvmsg_id = WSAID_WSARECVMSG;
    DWORD size;

    error = WSAIoctl(sockfd, SIO_GET_EXTENSION_FUNCTION_POINTER, &recvmsg_id, sizeof(recvmsg_id), &WSARecvMsg, sizeof(WSARecvMsg), &size, NULL, NULL);
    if (error == SOCKET_ERROR)
    {
        WinsockErrorHandler("WSAIoctl");
        exit(EXIT_FAILURE);
    }
}

uint16_t ComputeChecksum(const char *buf, size_t size)
{
    size_t i;
    uint64_t sum = 0;

    for (i = 0; i < size; i += 2)
    {
        sum += *(uint16_t *)buf;
        buf += 2;
    }
    if (size - i > 0)
        sum += *(uint8_t *)buf;

    while ((sum >> 16) != 0)
        sum = (sum & 0xffff) + (sum >> 16);

    return (uint16_t)~sum;
}
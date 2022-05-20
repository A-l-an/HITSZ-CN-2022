#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

char buf[MAX_SIZE+1];

void recv_mail()
{
    const char* host_name = "";             // TODO: Specify the mail server domain name
    const unsigned short port = 110;        // POP3 server port
    const char* user = "";                  // TODO: Specify the user
    const char* pass = "";                  // TODO: Specify the password
    char dest_ip[16];
    int s_fd;                               // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    struct sockaddr_in* servaddr;
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = swap16(port);
    bzero(servaddr->sin_zero, 8);
    servaddr->sin_addr.s_addr = inet_addr(dest_ip);

    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    connect(s_fd, servaddr, strlen(servaddr));

    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);

    // TODO: Send user and password and print server response
    send(s_fd, user, strlen(user), 0);
    int recv1 = recv(s_fd, user, strlen(user), 0);
    printf("user: %d", recv1);
    send(s_fd, pass, strlen(pass), 0);
    int recv2 = recv(s_fd, pass, strlen(pass), 0);
    printf("password: %d", recv2);

    // TODO: Send STAT command and print server response
    // 返回邮箱统计信息，包括邮箱邮件数和邮件占用的大小。
    const char* stat = 'STAT\r\n';
    send(s_fd, stat, strlen(stat), 0);
    int recv3 = recv(s_fd, stat, strlen(stat), 0);
    printf("stat: %d", recv3);

    // TODO: Send LIST command and print server response
    // 返回邮件信息。
    const char* list = 'LIST\r\n';
    send(s_fd, list, strlen(list), 0);
    int recv4 = recv(s_fd, list, strlen(list), 0);
    printf("list: %d", recv4);

    // TODO: Retrieve the first mail and print its content
    // 获取编号为msg的邮件正文。服务器返回的内容里第一行是邮件大小（以字节为单位），之后是邮件内容，最后一行是“.”，表示结束。
    int recv5 = recv(s_fd, msg, strlen(msg), 0);


    // TODO: Send QUIT command and print server response
    const char* QCommand = 'QUIT\r\n';
    send(s_fd, QCommand, strlen(QCommand), 0);
    int recv9 = recv(s_fd, QCommand, strlen(QCommand), 0);
    // printf("quit: %d", recv9);
    
    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}

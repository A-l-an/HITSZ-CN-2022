#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095

char buf[MAX_SIZE+1];

// receiver: mail address of the recipient
// subject:  mail subject
// msg:      content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.qq.com";                 // TODO: Specify the mail server domain name
    const unsigned short port = 25;             // SMTP server port
    const char* user = "alan";                  // TODO: Specify the user
    const char* pass = "";        // TODO: Specify the password
    const char* from = "alan@hitsz-lab.com";    // TODO: Specify the mail address of the sender
    char dest_ip[16];                           // Mail server IP address
    int s_fd;                                   // socket file descriptor
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

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
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

    // Send EHLO command and print server response
    const char* EHLO = "EHLO Alan\r\n";               // TODO: Enter EHLO command here
    send(s_fd, EHLO, strlen(EHLO), 0);

    // TODO: Print server response to EHLO command
    int recv1 = recv(s_fd, buf, MAX_SIZE, 0);
    if (recv1 == -1)
        printf('Not received from server.');
    else
        printf("%d", recv1);

    // TODO: Authentication. Server response should be printed out.
    const char* login = 'AUTH LOGIN\r\n';
    send(s_fd, login, strlen(login), 0);
    int recv2 = recv(s_fd, buf, MAX_SIZE, 0);
    printf("login: %d", recv2);

    const char* username = encode_str(user);
    send(s_fd, username, strlen(username), 0);
    int recv3 = recv(s_fd, username, strlen(username), 0);
    printf("user: %d", recv3);
    free(user);
    const char* password = encode_str(pass);
    send(s_fd, password, strlen(password), 0);
    int recv4 = recv(s_fd, buf, MAX_SIZE, 0);
    printf("password: %d", recv4);
    free(pass);

    // TODO: Send MAIL FROM command and print server response
    send(s_fd, from, strlen(from), 0);
    int recv5 = recv(s_fd, buf, MAX_SIZE, 0);
    printf("mail from: %d", recv5);

    // TODO: Send RCPT TO command and print server response
    send(s_fd, receiver, strlen(receiver), 0);
    int recv6 = recv(s_fd, buf, MAX_SIZE, 0);
    printf("rcpt to: %d", recv6);

    // TODO: Send DATA command and print server response
    const char* data = 'DATA\r\n';
    send(s_fd, data, strlen(data), 0);
    int recv7 = recv(s_fd, buf, MAX_SIZE, 0);
    printf("data: %d", recv7);

    // TODO: Send message data
    strcat(msg, 'From: ');     strcat(msg, from);      strcat(msg, '\r\n');
    strcat(msg, 'To: ');       strcat(msg, receiver);  strcat(msg, '\r\n');
    strcat(msg, 'Subject: ');  strcat(msg, subject);   strcat(msg, '\r\n');
    strcat(msg, 'Content: ');  strcat(msg, 'msg');     strcat(msg, '\r\n');
    strcat(msg, 'Attach: ');   strcat(msg, att_path);  strcat(msg, '\r\n');

    send(s_fd, msg, strlen(msg), 0);

    // TODO: Message ends with a single period
    send(s_fd, end_msg, strlen(end_msg), 0);
    int recv8 = recv(s_fd, buf, MAX_SIZE, 0);
    printf("mail: %d", recv8);

    // TODO: Send QUIT command and print server response
    const char* QCommand = 'QUIT\r\n';
    send(s_fd, QCommand, strlen(QCommand), 0);
    int recv9 = recv(s_fd, buf, MAX_SIZE, 0);
    printf("quit: %d", recv9);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}

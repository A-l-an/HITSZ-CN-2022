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
    const char* host_name = "";                 // TODO: Specify the mail server domain name
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

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    connect(clientSocket, servaddr, len(servaddr));

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
    clientSocket.send(heloCommand.encode())
    recv1 = clientSocket.recv(1024).decode()
    print(recv1)

    // TODO: Authentication. Server response should be printed out.
    if recv1[:3] != '250':
    print('250 reply not received from server.')

    // TODO: Send MAIL FROM command and print server response
    login = 'AUTH LOGIN\r\n'
    clientSocket.send(login.encode())
    recv2 = clientSocket.recv(1024).decode()
    print('login: ', recv2)

    clientSocket.send(user)
    recv2 = clientSocket.recv(1024).decode()
    print('user: ', recv2)
    clientSocket.send(password)
    recv3 = clientSocket.recv(1024).decode()
    print('password: ', recv3)

    mailFrom = 'MAIL FROM: <50627****@qq.com>\r\n'
    clientSocket.send(mailFrom.encode())
    recv4 = clientSocket.recv(1024).decode()
    print('mail from: ', recv4)

    // TODO: Send RCPT TO command and print server response
    reptTo = 'RCPT TO: <kairos****@163.com>\r\n'
    clientSocket.send(reptTo.encode())
    recv5 = clientSocket.recv(1024).decode()
    print('rcpt to: ', recv5)

    // TODO: Send DATA command and print server response
    data = 'DATA\r\n'
    clientSocket.send(data.encode())
    recv6 = clientSocket.recv(1024).decode()
    print('data: ', recv6)

    // TODO: Send message data
    msg_content = 'From: ' + from + '\r\n'
    msg_content += 'To: ' + receiver + '\r\n'
    msg_content += 'Subject: ' + subject + '\r\n'
    msg_content += 'Content: ' + msg + '\r\n'
    msg_content += 'Attach: ' + att_path + '\r\n'
    clientSocket.send(msg_content.encode())

    // TODO: Message ends with a single period
    clientSocket.send(endmsg.encode())
    recv9 = clientSocket.recv(1024).decode()
    print('mail: ', recv9)

    // TODO: Send QUIT command and print server response
    quitCommand = 'QUIT\r\n';
    clientSocket.send(quitCommand.encode());
    recv10 = clientSocket.recv(1024).decode();
    // print('quit: ', recv10);

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

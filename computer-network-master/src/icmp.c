#include "net.h"
#include "icmp.h"
#include "ip.h"

/**
 * @brief 发送icmp响应
 * 
 * @param req_buf 收到的icmp请求包
 * @param src_ip 源ip地址
 */
static void icmp_resp(buf_t *req_buf, uint8_t *src_ip)
{
    // Step1 ：封装报头和数据
    buf_init(&txbuf, req_buf->len);     // 初始化txbuf
    memcpy(txbuf.data, req_buf->data, req_buf->len);

    // Step2 ：填写校验和
    icmp_hdr_t *icmp_resp_hdr = (icmp_hdr_t *)txbuf.data;
    icmp_hdr_t *icmp_in_hdr = (icmp_hdr_t *)req_buf->data;

    icmp_resp_hdr->type = ICMP_TYPE_ECHO_REPLY;
    icmp_resp_hdr->code = ICMP_TYPE_ECHO_REPLY;
    icmp_resp_hdr->id16 = icmp_in_hdr->id16;
    icmp_resp_hdr->seq16 = icmp_in_hdr->seq16;
    icmp_resp_hdr->checksum16 = 0;

    icmp_resp_hdr->checksum16 = checksum16((uint16_t *)icmp_resp_hdr, txbuf.len);

    // Step3 ：将数据报发送出去。
    ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);
}

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_ip 源ip地址
 */
void icmp_in(buf_t *buf, uint8_t *src_ip)
{
    // Step1 ：报头检测，如果接收到的包长小于ICMP头部长度，则丢弃不处理。
    if (buf->len < 8)
        return ;

    // Step2 ：查看该报文的ICMP类型是否为回显请求。
    icmp_hdr_t *icmp_in_hdr = (icmp_hdr_t *)buf->data;
    uint16_t temp = icmp_in_hdr->checksum16;    // 先把头部校验和字段缓存起来     
    icmp_in_hdr->checksum16 = 0;
    uint16_t checksum = checksum16((uint16_t *)buf->data, buf->len);    // 比较计算的结果与之前缓存的校验和是否一致

    if (checksum != temp)
        return;
    else
        icmp_in_hdr->checksum16 = temp;

    // Step3 ：是，则调用icmp_resp()函数回送一个回显应答（ping 应答）。
    if (icmp_in_hdr->type == ICMP_TYPE_ECHO_REQUEST)
        icmp_resp(buf, src_ip);
}

/**
 * @brief 发送icmp不可达
 * 
 * @param recv_buf 收到的ip数据包
 * @param src_ip 源ip地址
 * @param code icmp code，协议不可达或端口不可达
 */
void icmp_unreachable(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code)
{
    // Step1 ：填写ICMP报头首部。
    buf_init(&txbuf, 8+20+8);           // ICMP头部 + IP头部 + 原始IP数据报中的前8字节

    // Step2 ：填写ICMP数据部分，包括IP数据报首部和IP数据报的前8个字节的数据字段，填写校验和。
    icmp_hdr_t *icmp_un_hdr = (icmp_hdr_t *)(&txbuf)->data;
    icmp_un_hdr->type = ICMP_TYPE_UNREACH;
    icmp_un_hdr->code = code;
    icmp_un_hdr->id16 = 0;
    icmp_un_hdr->seq16 = 0;
    icmp_un_hdr->checksum16 = 0;

    // Step3 ：调用ip_out()函数将数据报发送出去。
    memcpy(txbuf.data + sizeof(icmp_hdr_t), recv_buf->data, sizeof(ip_hdr_t) + 8);
    icmp_un_hdr->checksum16 = checksum16((uint16_t *)icmp_un_hdr, txbuf.len);
    ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);

}

/**
 * @brief 初始化icmp协议
 * 
 */
void icmp_init(){
    net_add_protocol(NET_PROTOCOL_ICMP, icmp_in);
}
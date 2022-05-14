#include "udp.h"
#include "ip.h"
#include "icmp.h"

/**
 * @brief udp处理程序表
 * 
 */
map_t udp_table;

/**
 * @brief udp伪校验和计算
 * 
 * @param buf 要计算的包
 * @param src_ip 源ip地址
 * @param dst_ip 目的ip地址
 * @return uint16_t 伪校验和
 * 
 * Step1 ：首先调用buf_add_header()函数增加UDP伪头部。
 * Step2 ：将被UDP伪头部覆盖的IP头部拷贝出来，暂存IP头部，以免被覆盖。
 * Step3 ：填写UDP伪头部的12字节字段。
 * Step4 ：计算UDP校验和。
 * Step5 ：再将 Step2 中暂存的IP头部拷贝回来。
 * Step6 ：调用buf_remove_header()函数去掉UDP伪头部。
 * Step7 ：返回计算出来的校验和值。
 */
static uint16_t udp_checksum(buf_t *buf, uint8_t *src_ip, uint8_t *dst_ip)
{
    buf_add_header(buf, sizeof(ip_hdr_t));         // IP层向上层传递数据包之前去掉了ip报头, 这里要增加UDP伪头部(即IP)。
    
    int flag = 0;                                  // 奇偶标记位，默认为偶。 
    ip_hdr_t ip_hdr;
    memcpy(&ip_hdr, buf->data, 20);                // 暂存IP头部的数据
    buf_remove_header(buf, 8);                     // UDP伪头部只需要后12字节
    udp_peso_hdr_t udp_preso_hdr;
    memcpy(udp_preso_hdr.src_ip, src_ip, 4);
    memcpy(udp_preso_hdr.dst_ip, dst_ip, 4);
    udp_preso_hdr.placeholder = 0;
    udp_preso_hdr.protocol = NET_PROTOCOL_UDP;
    udp_preso_hdr.total_len16 = swap16(buf->len - 12);     // 12字节的伪头部
    memcpy(buf->data, &udp_preso_hdr, sizeof(udp_peso_hdr_t));

    if(buf->len %2){                                //长度为奇数则补零
        buf_add_padding(buf,1);
        flag = 1;
    }
    
    uint16_t checksum = checksum16((uint16_t *)buf->data, buf->len);

    buf_add_header(buf, 8);                        // 将暂存的IP头部拷贝回来
    memcpy(buf->data, &ip_hdr, sizeof(ip_hdr_t));
    buf_remove_header(buf, sizeof(ip_hdr_t));

    if (flag == 1){
        buf_remove_padding(buf, 1);                // 计算后要除去填充字段
        flag = 0;
    }
    
    return checksum;
}

/**
 * @brief 处理一个收到的udp数据包
 * 
 * @param buf 要处理的包
 * @param src_ip 源ip地址
 * 
 * Step1 ：首先做包检查，检测该数据报的长度是否小于UDP首部长度，或者接收到的包长度小于UDP首部长度字段给出的长度，如果是，则丢弃不处理。
 * Step2 ：接着重新计算校验和，先把首部的校验和字段保存起来，然后把该字段填充0，调用udp_checksum()函数计算出校验和，如果该值与接收到的UDP数据报的校验和不一致，则丢弃不处理。
 * Step3 ：调用map_get()函数查询udp_table是否有该目的端口号对应的处理函数（回调函数）。
 * Step4 ：如果没有找到，则调用buf_add_header()函数增加IPv4数据报头部，再调用icmp_unreachable()函数发送一个端口不可达的ICMP差错报文。
 * Step5 ：如果能找到，则去掉UDP报头，调用处理函数来做相应处理。
 */
void udp_in(buf_t *buf, uint8_t *src_ip)
{
    udp_hdr_t *udp_in_hdr = (udp_hdr_t *)buf->data;
    uint16_t checksum16_ori   = udp_in_hdr->checksum16;     // 校验和不能换

    if (buf->len < sizeof(udp_hdr_t) || buf->len < swap16(udp_in_hdr->total_len16))
        return;

    udp_in_hdr->checksum16 = 0;
    uint16_t checksum16_now = udp_checksum(buf, src_ip, net_if_ip);
    if (checksum16_ori != checksum16_now)
        return;

    uint16_t port = swap16(udp_in_hdr->dst_port16);
    udp_handler_t *handler = map_get(&udp_table, &port);   // 若存在，返回的是map中的value，即处理程序。
    if(handler == NULL)     // 若该目的端口号对应的处理函数不存在
    {
        buf_add_header(buf, sizeof(ip_hdr_t));
        icmp_unreachable(buf, src_ip, ICMP_CODE_PORT_UNREACH);  // 发送一个端口不可达的ICMP差错报文
    }
    else
    {
        buf_remove_header(buf, sizeof(udp_hdr_t));
        (*handler)(buf->data, buf->len, src_ip, swap16(udp_in_hdr->src_port16));
        return;
    }

}

/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的包
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 * 
 * Step1 ：首先调用buf_add_header()函数添加UDP报头。
 * Step2 ：接着，填充UDP首部字段。
 * Step3 ：先将校验和字段填充0，然后调用udp_checksum()函数计算出校验和，再将计算出来的校验和结果填入校验和字段。
 * Step4 ：调用ip_out()函数发送UDP数据报。
 */
void udp_out(buf_t *buf, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    buf_add_header(buf, 8);
    udp_hdr_t udp_out_hdr;
    udp_out_hdr.src_port16  = swap16(src_port);
    udp_out_hdr.dst_port16  = swap16(dst_port);
    udp_out_hdr.total_len16 = swap16(buf->len);
    udp_out_hdr.checksum16  = swap16(0);
    memcpy(buf->data, &udp_out_hdr, sizeof(udp_hdr_t));

    uint16_t checksum16 = udp_checksum(buf, net_if_ip, dst_ip);
    udp_out_hdr.checksum16 = checksum16;
    memcpy(buf->data, &udp_out_hdr, sizeof(udp_hdr_t));      // 包括校验和，再复制一次。

    ip_out(buf, dst_ip, NET_PROTOCOL_UDP);
}

/**
 * @brief 初始化udp协议
 * 
 */
void udp_init()
{
    map_init(&udp_table, sizeof(uint16_t), sizeof(udp_handler_t), 0, 0, NULL);
    net_add_protocol(NET_PROTOCOL_UDP, udp_in);
}

/**
 * @brief 打开一个udp端口并注册处理程序
 * 
 * @param port 端口号
 * @param handler 处理程序
 * @return int 成功为0，失败为-1
 */
int udp_open(uint16_t port, udp_handler_t handler)
{
    return map_set(&udp_table, &port, &handler);
}

/**
 * @brief 关闭一个udp端口
 * 
 * @param port 端口号
 */
void udp_close(uint16_t port)
{
    map_delete(&udp_table, &port);
}

/**
 * @brief 发送一个udp包
 * 
 * @param data 要发送的数据
 * @param len 数据长度
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 */
void udp_send(uint8_t *data, uint16_t len, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    buf_init(&txbuf, len);
    memcpy(txbuf.data, data, len);
    udp_out(&txbuf, src_port, dst_ip, dst_port);
}
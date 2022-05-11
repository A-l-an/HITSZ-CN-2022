#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

int ip_id = 0;                  // IP协议标识符

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void ip_in(buf_t *buf, uint8_t *src_mac)
{
    // Step1: 如果数据长度小于IP头部长度，丢弃不处理。
    if (buf->len < 20)
        return ;
    // Step2: 报头检测
    ip_hdr_t *ip_hdr = (ip_hdr_t *)buf->data;
    if( (ip_hdr->version != IP_VERSION_4) 
     || (ip_hdr->total_len16 > buf->len )              // 总长度字段>收到的包的长度
     ) return;     

    // Step3: 头部校验
    uint16_t temp_checksum = ip_hdr->hdr_checksum16;    // 先把头部校验和字段缓存起来
    ip_hdr->hdr_checksum16 = 0;                         // 再将校验和字段清零

    // 调用checksum16函数来计算头部校验和，比较计算的结果与之前缓存的校验和是否一致。
    uint16_t checksum = checksum16((uint16_t *)buf->data, ip_hdr->hdr_len * IP_HDR_LEN_PER_BYTE);
    if (checksum != temp_checksum)                      // 不一致，丢弃不处理.
        return;
    else                                                // 一致，则将该头部校验和字段恢复成原来的值。
        ip_hdr->hdr_checksum16 = temp_checksum;

    // Step4: 对比目的IP地址是否为本机的IP地址
    uint8_t check_ip_dest = memcmp(ip_hdr->dst_ip, net_if_ip, NET_IP_LEN);
    if (check_ip_dest != 0)                                 // 不是，则丢弃不处理。
        return;

    // Step5: 去除填充字段
    if (buf->len > ip_hdr->total_len16)                           // 如果接收到的数据包的长度大于IP头部的总长度字段
        buf_remove_padding(buf, buf->len - swap16(ip_hdr->total_len16));  // 说明该数据包有填充字段，应去除。

    // Step6: 去掉IP报头。
    buf_remove_header(buf, 20);

    // Step7: 向上层传递数据包。                                              
    if (net_in(buf, ip_hdr->protocol, ip_hdr->src_ip) < 0)       // 协议类型不可识别，返回ICMP协议不可达信息。
        icmp_unreachable(buf, ip_hdr->src_ip, ICMP_CODE_PROTOCOL_UNREACH);
}

/**
 * @brief 处理一个要发送的ip分片
 * 
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // Step1: 增加IP数据报头部缓存空间。
    buf_add_header(buf, 20);

    // Step2: 填写IP数据报头部字段。
    ip_hdr_t *ip_hdr = (ip_hdr_t *)buf->data;

    ip_hdr->version = IP_VERSION_4;
    ip_hdr->hdr_len = 5;
    ip_hdr->tos = 0;
    ip_hdr->total_len16 = swap16(buf->len);              // 记得大小端转换
    ip_hdr->id16 = swap16(id);
    ip_hdr->flags_fragment16 = swap16(offset | mf<<13);  // 位2表禁止分片;位3标识更多分片,除了数据报的最后一个分片外都要标记为1
    ip_hdr->ttl = 64;
    ip_hdr->protocol = protocol;


    // Step3: 填写首部校验和
    ip_hdr->hdr_checksum16 = 0;                             // 先填0

    memcpy(ip_hdr->src_ip, net_if_ip, NET_IP_LEN);          // 拷贝要在计算校验和之前完成，否则校验和会计算失误。
    memcpy(ip_hdr->dst_ip, ip, NET_IP_LEN);

    uint16_t temp = checksum16((uint16_t *)ip_hdr, 20);     // 再计算校验和
    ip_hdr->hdr_checksum16 = temp;
    
    // Step4: 调用arp_out函数()将封装后的IP头部和数据发送出去。
    arp_out(buf, ip);
}

/**
 * @brief 处理一个要发送的ip数据包
 * 
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    int max_len = 1500 - 20;                           // 1500字节 - IP首部长度
    uint16_t offset = 0;
    buf_t ip_buf;
    // 从上层传递下来的数据报包长大于IP协议最大负载包长, 分片发送。
    if (buf->len > max_len)
    {
        // 若截断后最后的一个分片<=IP协议最大负载包长，跳出循环。
        while (buf->len > max_len)
        {
            buf_init(&ip_buf, max_len);                // 每个截断后的包长 = IP协议最大负载包长
            memcpy(ip_buf.data, buf->data, max_len);
            ip_fragment_out(&ip_buf, ip, protocol, ip_id, offset, 1);
            buf_remove_header(buf, max_len);           // 删除已发送内容
            offset = offset + max_len/8;               // 实际的偏移值为该值左移3位后得到的，所以除了最后一个外，每个IP分片的数据部分的长度都必须是8的整数。
        }
        // 若最后的一个分片是存在的（未发送）
        if (buf->len > 0)
        {
            buf_init(&ip_buf, buf->len);
            memcpy(ip_buf.data, buf->data, buf->len);
            ip_fragment_out(&ip_buf, ip, protocol, ip_id, offset, 0);   // 最后一个分片MF=0
        }
    }
    // 数据报包没有超过IP协议最大负载包长，则直接发送。
    else
    {
        buf_init(&ip_buf, buf->len);
        memcpy(ip_buf.data, buf->data, buf->len);
        ip_fragment_out(&ip_buf, ip, protocol, ip_id, offset, 0);
    }
    
    ip_id++;
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}
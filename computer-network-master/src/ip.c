#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

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
    // ip_hdr_t ip_hdr;
    // ip_hdr.version = (buf->data[0] & 0xf0) >> 4;
    // ip_hdr.total_len16 = (buf->data[2] << 8) + buf->data[3];
    // if( (ip_hdr.version != IP_VERSION_4) 
    //  && (ip_hdr.total_len16 <= buf->len )) return;

    ip_hdr_t *ip_hdr = (ip_hdr_t *)buf->data;
    if( (ip_hdr->version != IP_VERSION_4) 
     && (ip_hdr->total_len16 > buf->len )              // 总长度字段>收到的包的长度
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
    if (!check_ip_dest)                                 // 不是，则丢弃不处理。
        return;

    // Step5: 去除填充字段
    if (buf->len > ip_hdr->hdr_len)                           // 如果接收到的数据包的长度大于IP头部的总长度字段
        buf_remove_padding(buf, buf->len - ip_hdr->hdr_len);  // 说明该数据包有填充字段，应去除。

    // Step6: 去掉IP报头。
    buf_remove_header(buf, 20);

    // Step7: 向上层传递数据包。
    if(ip_hdr->protocol == NET_PROTOCOL_ICMP
    || ip_hdr->protocol == NET_PROTOCOL_UDP
    || ip_hdr->protocol == NET_PROTOCOL_TCP
    || ip_hdr->protocol == NET_PROTOCOL_ARP
    || ip_hdr->protocol == NET_PROTOCOL_IP)
        net_in(buf, ip_hdr->protocol, ip_hdr->src_ip);
    else                                                       // 协议类型不可识别，返回ICMP协议不可达信息。
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
    // TO-DO
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
    // TO-DO
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}
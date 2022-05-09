#include "ethernet.h"
#include "utils.h"
#include "driver.h"
#include "arp.h"
#include "ip.h"
/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 */
void ethernet_in(buf_t *buf)
{
    // 判断数据长度，如果数据长度小于以太网头部长度，则认为数据包不完整，丢弃不处理。
    if (buf->len < sizeof(ether_hdr_t))             //14 == 6字节源mac+6字节目的mac+2字节协议类型
        return;
    ether_hdr_t *hdr = (ether_hdr_t *)buf->data;
    uint16_t type = swap16(hdr->protocol16);        // 注意大小端的转换
    // 移除加以太网包头。
    buf_remove_header(buf, 14);                     // sizeof(ether_hdr_t)==14
    // 调用net_in()函数向上层传递数据包。 
    net_in(buf, type, hdr->src);
}
/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的数据包
 * @param mac 目标MAC地址
 * @param protocol 上层协议
 */
void ethernet_out(buf_t *buf, const uint8_t *mac, net_protocol_t protocol)
{
    // 判断数据长度，如果不足46则显式填充0.
    if (buf->len < 46)
        buf_add_padding(buf, 46 - buf->len);
    // 添加以太网包头
    buf_add_header(buf, sizeof(ether_hdr_t));
    ether_hdr_t *hdr = (ether_hdr_t *)buf->data;
    // 填写目的MAC地址
	memcpy(hdr->dst, mac, NET_MAC_LEN);
    // 填写源MAC地址，即本机的MAC地址。
	memcpy(hdr->src, net_if_mac, NET_MAC_LEN);
    // 填写协议类型 protocol -- 大小端的转换
    hdr->protocol16 = swap16(protocol);
    // 将添加了以太网包头的数据帧发送到驱动层。
    driver_send(buf);
}
/**
 * @brief 初始化以太网协议
 * 
 */
void ethernet_init()
{
    buf_init(&rxbuf, ETHERNET_MAX_TRANSPORT_UNIT + sizeof(ether_hdr_t));
}

/**
 * @brief 一次以太网轮询
 * 
 */
void ethernet_poll()
{
    if (driver_recv(&rxbuf) > 0)
        ethernet_in(&rxbuf);
}

#include <string.h>
#include <stdio.h>
#include "net.h"
#include "arp.h"
#include "ethernet.h"
/**
 * @brief 初始的arp包
 * 
 */
static const arp_pkt_t arp_init_pkt = {
    .hw_type16 = swap16(ARP_HW_ETHER),
    .pro_type16 = swap16(NET_PROTOCOL_IP),
    .hw_len = NET_MAC_LEN,
    .pro_len = NET_IP_LEN,
    .sender_ip = NET_IF_IP,
    .sender_mac = NET_IF_MAC,
    .target_mac = {0}};

/**
 * @brief arp地址转换表，<ip,mac>的容器
 * 
 */
map_t arp_table;

/**
 * @brief arp buffer，<ip,buf_t>的容器
 * 
 */
map_t arp_buf;

/**
 * @brief 打印一条arp表项
 * 
 * @param ip 表项的ip地址
 * @param mac 表项的mac地址
 * @param timestamp 表项的更新时间
 */
void arp_entry_print(void *ip, void *mac, time_t *timestamp)
{
    printf("%s | %s | %s\n", iptos(ip), mactos(mac), timetos(*timestamp));
}

/**
 * @brief 打印整个arp表
 * 
 */
void arp_print()
{
    printf("===ARP TABLE BEGIN===\n");
    map_foreach(&arp_table, arp_entry_print);
    printf("===ARP TABLE  END ===\n");
}

/**
 * @brief 发送一个arp请求
 * 
 * @param target_ip 想要知道的目标的ip地址
 */
void arp_req(uint8_t *target_ip)
{
    buf_t *buf;
	buf = &txbuf;
    buf_init(buf, sizeof(arp_pkt_t));
    arp_pkt_t arp_pkt;
    arp_pkt = arp_init_pkt;                     // 填写ARP报头
    arp_pkt.opcode16 = swap16(ARP_REQUEST);     // ARP操作类型为ARP_REQUEST，注意大小端转换。
    // ARP请求报文都是广播报文，目标MAC地址应该是广播地址.
    memcpy(arp_pkt.target_ip, target_ip, NET_IP_LEN);
    memcpy(buf->data, &arp_pkt, sizeof(arp_pkt_t));
	uint8_t dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    ethernet_out(buf, dst_mac, NET_PROTOCOL_ARP);   // 将ARP报文发送出去,注意,固定是ARP协议了。
}

/**
 * @brief 发送一个arp响应
 * 
 * @param target_ip 目标ip地址
 * @param target_mac 目标mac地址
 */
void arp_resp(uint8_t *target_ip, uint8_t *target_mac)
{
    buf_t *buf = &txbuf;
    buf_init(buf, sizeof(arp_pkt_t));
    // 填写ARP报头首部
    arp_pkt_t arp_pkt;
    arp_pkt = arp_init_pkt;
    arp_pkt.opcode16 = swap16(ARP_REPLY);
    memcpy(arp_pkt.target_ip, target_ip, NET_IP_LEN);
    memcpy(arp_pkt.target_mac, target_mac, NET_MAC_LEN);
    memcpy(buf->data, &arp_pkt, sizeof(arp_pkt_t));
    // 将填充好的ARP报文发送出去
    ethernet_out(buf, target_mac, NET_PROTOCOL_ARP);
}

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 * STEP4: 调用map_get()函数查看该接收报文的IP地址是否有对应的arp_buf缓存。
 * 如果有，则说明ARP分组队列里面有待发送的数据包。
 * 也就是上一次调用arp_out()函数发送来自IP层的数据包时，
 * 由于没有找到对应的MAC地址进而先发送的ARP request报文，此时收到了该request的应答报文。
 * 然后，将缓存的数据包arp_buf再发送给以太网层，即调用ethernet_out()函数直接发出去，
 * 接着调用map_delete()函数将这个缓存的数据包删除掉。
 * 如果该接收报文的IP地址没有对应的arp_buf缓存，还需要判断接收到的报文是否为ARP_REQUEST请求报文，
 * 并且该请求报文的target_ip是本机的IP，则认为是请求本主机MAC地址的ARP请求报文，
 * 则调用arp_resp()函数回应一个响应报文。
 */
void arp_in(buf_t *buf, uint8_t *src_mac)
{
    // 如果数据长度小于ARP头部长度，则认为数据包不完整，丢弃不处理。
    if (buf->len < 8)       // 8=2硬件类型 + 2上层协议类型 + 1MAC地址长度 + 1IP地址长度 + 2操作类型
        return ;
    // 做报头检查，查看报文是否完整。
    arp_pkt_t *arp_pkt = (arp_pkt_t *)buf->data;
	uint16_t opcode = swap16(arp_pkt->opcode16);
	// 不完整
	if (arp_pkt->hw_type16 != swap16(ARP_HW_ETHER)
		|| arp_pkt->pro_type16 != swap16(NET_PROTOCOL_IP)
		|| arp_pkt->hw_len != NET_MAC_LEN
		|| arp_pkt->pro_len != NET_IP_LEN
		|| (opcode != ARP_REQUEST && opcode != ARP_REPLY)
	)   return;
    // 完整，更新ARP表项。
    map_set(&arp_table,arp_pkt->sender_ip, src_mac);
    // 查看该接收报文的IP地址是否有对应的arp_buf缓存
    buf_t *buf_t = map_get(&arp_buf, arp_pkt->sender_ip);
    if (buf_t != NULL){     // 有arp_buf
            ethernet_out(buf_t, arp_pkt->sender_mac, NET_PROTOCOL_IP);  // 将数据包直接发送给以太网层(sender_ip的包对应的MAC地址就是sender_mac)
            map_delete(&arp_buf, arp_pkt->sender_ip);                   // 将这个缓存的数据包删除掉
            return ;
    }
    else{                   // 没有arp_buf
        if ((opcode == ARP_REQUEST) && (!memcmp(net_if_ip, arp_pkt->target_ip, NET_IP_LEN))){   // 是本主机MAC地址的ARP请求报文
            // uint8_t this_ip[NET_IP_LEN] = NET_IF_IP;
            // uint8_t this_mac[NET_MAC_LEN] = NET_IF_MAC;
            // uint8_t target_ip_t[NET_IP_LEN];
            // memcpy(target_ip_t, arp_pkt->target_ip, NET_IP_LEN);
            // uint8_t sender_ip_t[NET_IP_LEN];
            // memcpy(sender_ip_t, arp_pkt->sender_ip, NET_IP_LEN);
            // uint8_t sender_mac_t[NET_MAC_LEN];
            // memcpy(sender_mac_t, arp_pkt->sender_mac, NET_MAC_LEN);
            arp_resp(arp_pkt->sender_ip, arp_pkt->sender_mac);          // 回应一个本主机的响应报文(而不应该是net_if_ip,因为要发送给原来的)
        }
    }


}

/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的数据包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void arp_out(buf_t *buf, uint8_t *ip)
{
    // 根据IP地址来查找ARP表
    uint8_t *dst_mac = map_get(&arp_table, ip);
    if(dst_mac != NULL){    // found MAC
        ethernet_out(buf, dst_mac, NET_PROTOCOL_IP);    // 将数据包直接发送给以太网层
        return ;
    }  
    else{                   // not found
        // 判断arp_buf是否已经有包
        if(map_get(&arp_buf, ip) == NULL){  // 没有(有则说明正在等待该ip回应ARP请求，此时不能再发送arp请求)
            map_set(&arp_buf, ip, buf);     // 将来自IP层的数据包缓存到arp_buf
            arp_req(ip);                    // 发送请求目标IP地址对应的MAC地址的ARP request报文。
            return ;
        }
    }
}

/**
 * @brief 初始化arp协议
 * 
 */
void arp_init()
{
    map_init(&arp_table, NET_IP_LEN, NET_MAC_LEN, 0, ARP_TIMEOUT_SEC, NULL);        // 初始化用于存储IP地址和MAC地址的ARP表arp_table，并设置超时时间
    map_init(&arp_buf, NET_IP_LEN, sizeof(buf_t), 0, ARP_MIN_INTERVAL, buf_copy);   // 初始化用于缓存来自IP层的数据包，并设置超时时间
    net_add_protocol(NET_PROTOCOL_ARP, arp_in);     // 增加key：NET_PROTOCOL_ARP和vaule：arp_in的键值对
    arp_req(net_if_ip);                             // a,即广播包,告诉所有人自己的IP地址和MAC地址
}
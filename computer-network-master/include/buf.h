#ifndef BUF_H
#define BUF_H

#include <stdlib.h>
#include <stdint.h>
#include "config.h"

typedef struct buf //协议栈的通用数据包buffer, 可以在头部装卸数据，以供协议头的添加和去除
{
    size_t len;                   // 包中有效数据大小
    uint8_t *data;                // 包的数据起始地址
    uint8_t payload[BUF_MAX_LEN]; // 最大负载数据量
} buf_t;

int buf_init(buf_t *buf, size_t len);           //初始化buffer为给定的长度，用于装载数据包
int buf_add_header(buf_t *buf, size_t len);     //为buffer在头部增加一段长度，用于添加协议头
int buf_remove_header(buf_t *buf, size_t len);  //为buffer在头部减少一段长度，去除协议头
int buf_add_padding(buf_t *buf, size_t len);    //为buffer在尾部添加一段长度，填充0
int buf_remove_padding(buf_t *buf, size_t len); //为buffer在尾部减少一段长度，去除填充
void buf_copy(void *pdst, const void *psrc, size_t len);

#endif
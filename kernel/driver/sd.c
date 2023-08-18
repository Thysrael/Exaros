#include "mem_layout.h"
#include "spi.h"
#include "types.h"
#include "driver.h"
#include "sd.h"
#include "debug.h"
#include "process.h"
#include "file.h"
#include "arch.h"

int sdDevRead(int isUser, u64 dst, u64 startAddr, u64 n);
int sdDevWrite(int isUser, u64 dst, u64 startAddr, u64 n);
u8 sdCmd(u8 index, u32 arg, u8 crc);
int sdCmd0();
int sdCmd8();
int sdCmd12();
int sdCmd16();
int sdCmd55();
int sdCmd58();
int sdAcmd41();
void sdCmdEnd();
void spiRegSet();
void sdReset();
u8 spiTransfer(u8);
u8 spiReceive();

u8 response[5];

/**
 * @brief SD 卡初始化
 *
 */
void sdInit()
{
    printk("sdInit Staft\n");
    // REG32(uart, UART_REG_TXCTRL) = UART_TXEN; // 开启 UART TX
    spiRegSet();
    sdReset();
    printk("[SD card] SD card init finish!\n");

    *(volatile u32 *)SPI_REG_SCKDIV = 60;
    // asm volatile("fence.i");

    extern struct devsw devsw[];
    devsw[DEV_SD].read = sdDevRead;
    devsw[DEV_SD].write = sdDevWrite;
    printk("sdInit End\n");
}

/**
 * @brief 读 SD 卡
 *
 * @param buf 缓冲区
 * @param startBlock 开始 Block (512 Bytes)
 * @param blockNumber 读取的 Block 数量
 * @return int
 */
int sdRead(u8 *buf, u64 startBlock, u64 blockNumber)
{
    int readTimes = 0;
begin:;
    // command & response
    u8 *p = (void *)buf;
#if defined VIRT || defined QEMU
    if (sdCmd(18, startBlock * 512, 0xE1))
#else
    if (sdCmd(18, startBlock, 0xE1))
#endif
    {
        sdCmdEnd();
        panic("[SD Read] Read Error, retry times %x\n", readTimes);
        return 1;
    }
    u64 block = blockNumber;
    do {
        // Start Block Token
        int timeout = MAX_TIMES;
        do {
            int r = spiReceive();
            if (r == 0xFE) { break; }
            timeout--;
        } while (timeout);
        if (!timeout) { goto retry; }
        // data block
        int byte = 512;
        do {
            *p++ = spiReceive();
            byte--;
        } while (byte);
        // CRC
        spiReceive();
        spiReceive();
        block--;
    } while (block);
    // command & response
    if (sdCmd12()) { goto retry; }
    sdCmdEnd();
    return 0;
retry:
    readTimes++;
    if (readTimes > 10) { panic("[SD Read] readTimes > 10"); }
    sdCmdEnd();
    goto begin;
}

/**
 * @brief 写 SD 卡
 *
 * @param buf 缓冲区
 * @param startBlock
 * @param blockNumber
 * @return int
 */
int sdWrite(u8 *buf, u64 startBlock, u64 blockNumber)
{
    int writeTimes = 0;
begin:;
    u8 *p = (void *)buf;
    u64 block = blockNumber;
    do {
        // command & response
#if defined VIRT || defined QEMU
        if (sdCmd(24, startBlock * 512, 0))
#else
        if (sdCmd(24, startBlock, 0))
#endif
        {
            sdCmdEnd();
            panic("[SD Write] Write Error, retry times %x\n", writeTimes);
            return 1;
        }
        u8 r;
        spiReceive(); // 时序图中不需要这一步，可能是 QEMU BUG
        // Start Block Token
        spiTransfer(0xFE);
        // data block
        int byte = 512;
        do {
            spiTransfer(*p++);
            byte--;
        } while (byte);
        // data_response
        int timeout = MAX_TIMES;
        do {
            r = spiReceive();
            // CHL_DEBUG("spiReceive = 0x%lx\n", r);
            if ((r & 0x11) == 0x01) { break; }
            // if ((r & 0x1F) == 0x05) { break; }
            timeout--;
        } while (timeout);
        if (!timeout) { goto retry; }
        u8 status = (r >> 1) & 0x07;
        if (status != 0x02) { panic("[SD Write] Write Error, response = 0x%x\n", r); }
        // busy
        timeout = MAX_TIMES;
        do {
            r = spiReceive();
            // CHL_DEBUG("spiReceive = 0x%lx\n", r);
            if (r != 0x00) { break; }
            timeout--;
        } while (timeout);
        if (!timeout) { goto retry; }
        startBlock++;
        block--;
    } while (block);
    sdCmdEnd();
    return 0;
retry:
    writeTimes++;
    if (writeTimes > 10) { panic("[SD Write] writeTimes > 10"); }
    sdCmdEnd();
    goto begin;
    return 0;
}

u8 binary[1024];
int sdTest()
{
    sdInit();
    printk("### sdTest begin ###\n");
    for (int j = 0; j < 20; j += 2)
    {
        for (int i = 0; i < 1024; i++)
        {
            binary[i] = i & 7;
        }
        sdWrite(binary, j, 2);
        for (int i = 0; i < 1024; i++)
        {
            binary[i] = 0;
        }
        sdRead(binary, j, 2);
        for (int i = 0; i < 1024; i++)
        {
            if (binary[i] != (i & 7))
            {
                panic("gg: %d %d %d ", j, i, binary[i]);
                break;
            }
        }
        printk("sdTest finish %d\n", j);
    }
    printk("### sdTest end ###\n");
    return 0;
}

/**
 * @brief 设备读 SD 卡
 * 只支持 block 对齐读
 *
 * @param isUser
 * @param dst
 * @param startAddr
 * @param n
 * @return int
 */
int sdDevRead(int isUser, u64 dst, u64 startAddr, u64 n)
{
    if (n & ((1 << 9) - 1))
    {
        printk("[SD] Card Read error\n");
        return -1;
    }
    if (startAddr & ((1 << 9) - 1))
    {
        printk("[SD] Card Read error\n");
        return -1;
    }

    u64 startBlock = startAddr >> 9;
    u64 blockNumber = n >> 9;

    if (isUser)
    {
        u8 buf[512];
        do {
            sdRead(buf, startBlock, 1);
            copyout(myProcess()->pgdir, dst, (char *)buf, 512);
            dst += 512;
            startBlock++;
            blockNumber--;
        } while (blockNumber);
    }
    else { sdRead((u8 *)dst, startBlock, blockNumber); }
    return 0;
}

/**
 * @brief 设备写 SD 卡
 * 只支持 block 对齐写
 *
 * @param isUser
 * @param src
 * @param startAddr
 * @param n
 * @return int
 */
int sdDevWrite(int isUser, u64 src, u64 startAddr, u64 n)
{
    if (n & ((1 << 9) - 1))
    {
        printk("[SD] Card Read error\n");
        return -1;
    }
    if (startAddr & ((1 << 9) - 1))
    {
        printk("[SD] Card Read error\n");
        return -1;
    }

    u64 startBlock = startAddr >> 9;
    u64 blockNumber = n >> 9;

    if (isUser)
    {
        u8 buf[512];
        do {
            copyin(myProcess()->pgdir, (char *)buf, src, 512);
            sdWrite(buf, startBlock, 1);
            src += 512;
            startBlock++;
            blockNumber--;
        } while (blockNumber);
    }
    else
    {
        do {
            sdWrite((u8 *)src, startBlock, 1);
            src += 512;
            startBlock++;
            blockNumber--;
        } while (blockNumber);
    }
    return 0;
}

/**
 * @brief 初始化 SPI 寄存器
 *
 */
void spiRegSet()
{
    *(volatile u32 *)SPI_REG_SCKDIV = 3000;           // 设置时钟 f_{csk}=pclk/(2*(1+sckdiv)）
    *(volatile u32 *)SPI_REG_CSID = 0;                // 设置片选 ID
    *(volatile u32 *)SPI_REG_CSDEF |= 1;              // 设置片选
    *(volatile u32 *)SPI_REG_CSMODE = SPI_CSMODE_OFF; // 关闭片选
    *(volatile u32 *)SPI_REG_FMT = 0x80000;           // 设置数据格式 每次传输 8 位
    for (int i = 10; i > 0; i--)                      // 等待十个周期
    {
        spiReceive();
    }
    *(volatile u32 *)SPI_REG_CSMODE = SPI_CSMODE_AUTO; // 片选设置为自动模式
}

/**
 * @brief 重置 SD 卡
 *
 */
void sdReset()
{
    int n = 10;
    do {
        if (!sdCmd0())
            break;
        n--;
    } while (n);
    if (!n) { panic("[SD card] CMD0 error!\n"); }
    if (sdCmd8()) { panic("[SD card]CMD8 error!\n"); }
    if (sdAcmd41()) { panic("[SD card]ACMD41 error!\n"); }
    if (sdCmd58()) { panic("[SD card]CMD58 error!\n"); }
    if (sdCmd16()) { panic("[SD card]CMD16 error!\n"); }
}

/**
 * @brief 交换 1 byte 数据
 *
 * @param data
 * @return u8
 */
u8 spiTransfer(u8 data)
{
    i32 r;
    *(volatile u32 *)SPI_REG_TXFIFO = data;
    do {
        r = *(volatile u32 *)SPI_REG_RXFIFO;
    } while (r < 0);
    return (r & 0xff);
}

/**
 * @brief 接收 1 Byte 数据
 *
 * @return u8
 */
u8 spiReceive()
{
    return spiTransfer(0xff);
}

/**
 * @brief 发送 SD Card CMD
 * 支持 CMD 和 ACMD（自行调用 CMD55）
 *
 * @param cmd CMD index
 * @param arg CMD 参数
 * @param crc CMD 校验码
 * @return u8
 */
u8 sdCmd(u8 index, u32 arg, u8 crc)
{
    *(volatile u32 *)SPI_REG_CSMODE = SPI_CSMODE_HOLD;
    spiReceive();
    spiTransfer(index + 0x40); // start bit, transmission bit
    spiTransfer(arg >> 24);
    spiTransfer(arg >> 16);
    spiTransfer(arg >> 8);
    spiTransfer(arg);
    spiTransfer(crc);

    u32 r;
    unsigned long n = 1000;
    do {
        r = spiReceive();
        // CHL_DEBUG("cmd index = %d, return = 0x%lx\n", index, (r & 0xFF));
        if (!(r & 0x80)) { return (r & 0xFF); } // 真正命令返回值第 7 位为 0
        n--;
    } while (n);
    panic("sdCmd: timeout\n");
}

/**
 * @brief 命令模式结束
 *
 */
void sdCmdEnd()
{
    spiReceive();
    *(volatile u32 *)SPI_REG_CSMODE = SPI_CSMODE_AUTO;
}

/**
 * @brief 重置 SD 卡
 *
 * R1 (1 byte)
 * @return int
 */
int sdCmd0()
{
    response[0] = sdCmd(0, 0, 0x95);
    sdCmdEnd();
    int rc = response[0] != 0x1; // 需要在 idle 态
    return rc;
}

/**
 * @brief 发送 SD 卡接口条件
 * 询问 SD 卡是否能在提供的电压范围内运行
 *
 * R7 (5 bytes)
 * @return int
 */
int sdCmd8()
{
    response[4] = sdCmd(8, 0x1AA, 0x87); // VHS = 0x1, chech pattern = 0xAA
    response[3] = spiReceive();
    response[2] = spiReceive();
    response[1] = spiReceive();
    response[0] = spiReceive();
    sdCmdEnd();
    int rc = response[4] != 0x1; // 需要在 idle 态
    rc |= (response[1] != 0x1);  // Voltage accepted 2.7-3.6V
    rc |= (response[0] != 0xAA); // Echo-back of check pattern
    return rc;
}

/**
 * @brief 停止多块读（CMD18）
 *
 * R1b (1 byte)
 * @return int
 */
int sdCmd12()
{
    sdCmd(12, 0, 0x01);
    int timeout = MAX_TIMES;
    do {
        if (spiReceive()) { break; }
        --timeout;
    } while (timeout);
    int rc = timeout == 0;
    return rc;
}

/**
 * @brief 设置块大小
 *
 * R1 (1 byte)
 * @return int
 */
int sdCmd16()
{
    response[0] = sdCmd(16, 0x200, 0x15);
    sdCmdEnd();
    int rc = (response[0] != 0x00); // 需要不在 idle 态
    return rc;
}

/**
 * @brief 表明下一个命令是 ACMD
 *
 * R1 (1 bytes)
 */
int sdCmd55()
{
    response[0] = sdCmd(55, 0, 0x65);
    sdCmdEnd();
    return 0; // 不会出错
}

/**
 * @brief 读卡中的 OCR 寄存器
 *
 * R3 (5 bytes)
 * @return int
 */
int sdCmd58()
{
#if defined VIRT || defined QEMU
    response[4] = sdCmd(58, 0, 0xFD);
    response[3] = spiReceive();
    response[2] = spiReceive();
    response[1] = spiReceive();
    response[0] = spiReceive();
    return 0;
#endif
    response[4] = sdCmd(58, 0, 0xFD);
    response[3] = spiReceive();
    response[2] = spiReceive();
    response[1] = spiReceive();
    response[0] = spiReceive();
    sdCmdEnd();
    int rc = response[4] != 0x00;       // 需要不在 idle 态
    rc |= (response[3] & 0x80) ? 0 : 1; // 需要 card 上电完成 (BUSY)
    return rc;
}

/**
 * @brief 设置宿主支持信息
 * 并启动卡初始化进程
 *
 * 支持 SDHC or SDXC 时
 * HCS(High Capacity Support) = 1
 *
 * R1 (1 byte)
 * @return int
 */
int sdAcmd41()
{
    do {
        sdCmd55();
        response[0] = sdCmd(41, 0x40000000, 0x77);
    } while (response[0] == 0x01);
    int rc = response[0] != 0x00; // 需要不在 idle 态
    return rc;
}
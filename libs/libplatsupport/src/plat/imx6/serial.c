/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include "serial.h"
#include <string.h>

#define UART_REF_CLK           40089600

#define UART_SR1_RRDY          BIT( 9)
#define UART_SR1_TRDY          BIT(13)
/* CR1 */
#define UART_CR1_UARTEN        BIT( 0)
#define UART_CR1_RRDYEN        BIT( 9)
/* CR2 */
#define UART_CR2_SRST          BIT( 0)
#define UART_CR2_RXEN          BIT( 1)
#define UART_CR2_TXEN          BIT( 2)
#define UART_CR2_ATEN          BIT( 3)
#define UART_CR2_RTSEN         BIT( 4)
#define UART_CR2_WS            BIT( 5)
#define UART_CR2_STPB          BIT( 6)
#define UART_CR2_PROE          BIT( 7)
#define UART_CR2_PREN          BIT( 8)
#define UART_CR2_RTEC          BIT( 9)
#define UART_CR2_ESCEN         BIT(11)
#define UART_CR2_CTS           BIT(12)
#define UART_CR2_CTSC          BIT(13)
#define UART_CR2_IRTS          BIT(14)
#define UART_CR2_ESCI          BIT(15)
/* CR3 */
#define UART_CR3_RXDMUXDEL     BIT( 2)
/* FCR */
#define UART_FCR_RFDIV(x)      ((x) * BIT(7))
#define UART_FCR_RFDIV_MASK    UART_FCR_RFDIV(0x7)
#define UART_FCR_RXTL(x)       ((x) * BIT(0))
#define UART_FCR_RXTL_MASK     UART_FCR_RXTL(0x1F)
/* SR2 */
#define UART_SR2_RXFIFO_RDR    BIT(0)
#define UART_SR2_TXFIFO_EMPTY  BIT(14)
/* RXD */
#define UART_URXD_READY_MASK   BIT(15)
#define UART_BYTE_MASK         0xFF

struct imx6_uart_regs {
    uint32_t rxd;      /* 0x000 Receiver Register */
    uint32_t res0[15];
    uint32_t txd;      /* 0x040 Transmitter Register */
    uint32_t res1[15];
    uint32_t cr1;      /* 0x080 Control Register 1 */
    uint32_t cr2;      /* 0x084 Control Register 2 */
    uint32_t cr3;      /* 0x088 Control Register 3 */
    uint32_t cr4;      /* 0x08C Control Register 4 */
    uint32_t fcr;      /* 0x090 FIFO Control Register */
    uint32_t sr1;      /* 0x094 Status Register 1 */
    uint32_t sr2;      /* 0x098 Status Register 2 */
    uint32_t esc;      /* 0x09c Escape Character Register */
    uint32_t tim;      /* 0x0a0 Escape Timer Register */
    uint32_t bir;      /* 0x0a4 BRM Incremental Register */
    uint32_t bmr;      /* 0x0a8 BRM Modulator Register */
    uint32_t brc;      /* 0x0ac Baud Rate Counter Register */
    uint32_t onems;    /* 0x0b0 One Millisecond Register */
    uint32_t ts;       /* 0x0b4 Test Register */
};
typedef volatile struct imx6_uart_regs imx6_uart_regs_t;

static inline imx6_uart_regs_t*
imx6_uart_get_priv(ps_chardevice_t *d)
{
    return (imx6_uart_regs_t*)d->vaddr;
}


static int uart_getchar(ps_chardevice_t *d)
{
    imx6_uart_regs_t* regs = imx6_uart_get_priv(d);
    uint32_t reg = 0;
    int c = -1;

    if (regs->sr2 & UART_SR2_RXFIFO_RDR) {
        reg = regs->rxd;
        if (reg & UART_URXD_READY_MASK) {
            c = reg & UART_BYTE_MASK;
        }
    }
    return c;
}

static int uart_putchar(ps_chardevice_t* d, int c)
{
    imx6_uart_regs_t* regs = imx6_uart_get_priv(d);
    if (regs->sr2 & UART_SR2_TXFIFO_EMPTY) {
        if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
            uart_putchar(d, '\r');
        }
        regs->txd = c;
        return c;
    } else {
        return -1;
    }
}

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    /* TODO */
}


static ssize_t
uart_write(ps_chardevice_t* d, const void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    const char* data = (const char*)vdata;
    int i;
    for (i = 0; i < count; i++) {
        if (uart_putchar(d, *data++) < 0) {
            return i;
        }
    }
    return count;
}

static ssize_t
uart_read(ps_chardevice_t* d, void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    char* data;
    int ret;
    int i;
    data = (char*)vdata;
    for (i = 0; i < count; i++) {
        ret = uart_getchar(d);
        if (ret != EOF) {
            *data++ = ret;
        } else {
            return i;
        }
    }
    return count;
}


/*
 * BaudRate = RefFreq / (16 * (BMR + 1)/(BIR + 1) )
 * BMR and BIR are 16 bit
 */
static void
imx6_uart_set_baud(ps_chardevice_t* d, long bps)
{
    imx6_uart_regs_t* regs = imx6_uart_get_priv(d);
    uint32_t bmr, bir, fcr;
    fcr = regs->fcr;
    fcr &= ~UART_FCR_RFDIV_MASK;
    fcr |= UART_FCR_RFDIV(4);
    bir = 0xf;
    bmr = UART_REF_CLK / bps - 1;
    regs->bir = bir;
    regs->bmr = bmr;
    regs->fcr = fcr;
}

int
uart_configure(ps_chardevice_t* d, long bps, int char_size, enum serial_parity parity, int stop_bits)
{
    imx6_uart_regs_t* regs = imx6_uart_get_priv(d);
    uint32_t cr2;
    /* Character size */
    cr2 = regs->cr2;
    if (char_size == 8) {
        cr2 |= UART_CR2_WS;
    } else if (char_size == 7) {
        cr2 &= ~UART_CR2_WS;
    } else {
        return -1;
    }
    /* Stop bits */
    if (stop_bits == 2) {
        cr2 |= UART_CR2_STPB;
    } else if (stop_bits == 1) {
        cr2 &= ~UART_CR2_STPB;
    } else {
        return -1;
    }
    /* Parity */
    if (parity == PARITY_NONE) {
        cr2 &= ~UART_CR2_PREN;
    } else if (parity == PARITY_ODD) {
        /* ODD */
        cr2 |= UART_CR2_PREN;
        cr2 |= UART_CR2_PROE;
    } else if (parity == PARITY_EVEN) {
        /* Even */
        cr2 |= UART_CR2_PREN;
        cr2 &= ~UART_CR2_PROE;
    } else {
        return -1;
    }
    /* Apply the changes */
    regs->cr2 = cr2;
    /* Now set the board rate */
    imx6_uart_set_baud(d, bps);
    return 0;
}

int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    imx6_uart_regs_t* regs;

    /* Attempt to map the virtual address, assure this works */
    void* vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    regs = imx6_uart_get_priv(dev);

    /* Software reset */
    regs->cr2 &= ~UART_CR2_SRST;
    while (!(regs->cr2 & UART_CR2_SRST));

    /* Line configuration */
    uart_configure(dev, 115200, 8, PARITY_NONE, 1);

    /* Enable the UART */
    regs->cr1 |= UART_CR1_UARTEN;                /* Enable The uart.                  */
    regs->cr2 |= UART_CR2_RXEN | UART_CR2_TXEN;  /* RX/TX enable                      */
    regs->cr2 |= UART_CR2_IRTS;                  /* Ignore RTS                        */
    regs->cr3 |= UART_CR3_RXDMUXDEL;             /* Configure the RX MUX              */
    /* Initialise the receiver interrupt.                                             */
    regs->cr1 &= ~UART_CR1_RRDYEN;               /* Disable recv interrupt.           */
    regs->fcr &= ~UART_FCR_RXTL_MASK;            /* Clear the rx trigger level value. */
    regs->fcr |= UART_FCR_RXTL(1);               /* Set the rx tigger level to 1.     */
    regs->cr1 |= UART_CR1_RRDYEN;                /* Enable recv interrupt.            */

    return 0;
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdint.h>
#include "mux.h"
#include <platsupport/gpio.h>
#include "../../services.h"

//#define GPIO_DEBUG
#ifdef GPIO_DEBUG
#define DGPIO(...) printf("GPIO: " __VA_ARGS__)
#else
#define DGPIO(...) do{}while(0)
#endif

#define IMX6_GPIO1_PADDR  0x0209C000
#define IMX6_GPIO2_PADDR  0x020A0000
#define IMX6_GPIO3_PADDR  0x020A4000
#define IMX6_GPIO4_PADDR  0x020A8000
#define IMX6_GPIO5_PADDR  0x020AC000
#define IMX6_GPIO6_PADDR  0x020B0000
#define IMX6_GPIO7_PADDR  0x020B4000

#define IMX6_GPIOX_SIZE   0x1000
#define IMX6_GPIO1_SIZE   IMX6_GPIOX_SIZE
#define IMX6_GPIO2_SIZE   IMX6_GPIOX_SIZE
#define IMX6_GPIO3_SIZE   IMX6_GPIOX_SIZE
#define IMX6_GPIO4_SIZE   IMX6_GPIOX_SIZE
#define IMX6_GPIO5_SIZE   IMX6_GPIOX_SIZE
#define IMX6_GPIO6_SIZE   IMX6_GPIOX_SIZE
#define IMX6_GPIO7_SIZE   IMX6_GPIOX_SIZE

#define GPIO_ICFG_LOW     0x0
#define GPIO_ICFG_HIGH    0x1
#define GPIO_ICFG_RISE    0x2
#define GPIO_ICFG_FALL    0x3
#define GPIO_ICFG(f, v)   (((v) & 0x3) << ((f) * 2))
#define GPIO_ICFG_MASK(f) GPIO_ICFG(f, 0x3)


struct imx6_gpio_regs {
    uint32_t data;        /* +0x00 */
    uint32_t direction;   /* +0x04 */
    uint32_t pad_status;  /* +0x08 */
    uint32_t int_cfg;     /* +0x0C */
    uint32_t int_mask;    /* +0x14 */
    uint32_t int_status;  /* +0x18 */
    uint32_t edge;        /* +0x1C */
};


static struct imx6_gpio {
    mux_sys_t* mux;
    volatile struct imx6_gpio_regs* bank[GPIO_NBANKS];
} _gpio;

volatile static struct imx6_gpio_regs*
imx6_gpio_get_bank(gpio_t* gpio) {
    struct imx6_gpio* gpio_priv;
    int port;
    assert(gpio);
    assert(gpio->gpio_sys);
    assert(gpio->gpio_sys->priv);
    gpio_priv = (struct imx6_gpio*)gpio->gpio_sys->priv;
    port = GPIOID_PORT(gpio->id);
    assert(port < GPIO_NBANKS);
    assert(port >= 0);
    return gpio_priv->bank[port];
}

static int
imx6_gpio_init(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio)
{
    volatile struct imx6_gpio_regs* bank;
    struct imx6_gpio* gpio_priv;
    uint32_t v;
    int pin;
    assert(gpio);
    assert(gpio_sys);
    gpio_priv = (struct imx6_gpio*)gpio_sys->priv;
    assert(gpio_priv);
    pin = GPIOID_PIN(id);
    assert(pin < 32);
    assert(pin >= 0);

    gpio->id = id;
    gpio->gpio_sys = gpio_sys;
    gpio->next = NULL;

    bank = imx6_gpio_get_bank(gpio);
    DGPIO("Configuring GPIO on port %d pin %d\n",
          GPIOID_PORT(id), GPIOID_PIN(id));

    /* MUX the GPIO */
    if (imx6_mux_enable_gpio(gpio_priv->mux, id)) {
        DGPIO("Invalid GPIO\n");
        return -1;
    }

    /* Set direction */
    v = bank->direction;
    if (dir == GPIO_DIR_IN) {
        v &= ~BIT(pin);
        DGPIO("configuring {%d,%d} for input %p => 0x%x->0x%x\n",
              GPIOID_PORT(id), GPIOID_PIN(id),
              &bank->direction, bank->direction, v);
    } else {
        v |= BIT(pin);
        DGPIO("configuring {%d,%d} for output %p => 0x%x->0x%x\n",
              GPIOID_PORT(id), GPIOID_PIN(id),
              &bank->direction, bank->direction, v);
    }
    bank->direction = v;

    return 0;
}

static int
imx6_gpio_write(gpio_t* gpio, const char* data, int len)
{
    int count;
    for (count = 0; count < len && gpio; count++) {
        volatile struct imx6_gpio_regs* bank;
        uint32_t v;
        int pin;

        bank = imx6_gpio_get_bank(gpio);
        pin = GPIOID_PIN(gpio->id);
        assert(pin < 32);
        assert(pin >= 0);

        v = bank->data;
        if (*data++) {
            v |= (1U << pin);
            DGPIO("data(%p) => 0x%x->0x%x\n",
                  &bank->data, bank->data, v);
        } else {
            v &= ~(1U << pin);
            DGPIO("data(%p) => 0x%x->0x%x\n",
                  &bank->data, bank->data, v);
        }
        bank->data = v;
        assert(bank->data == v);

        gpio = gpio->next;
    }
    return count;
}

static int
imx6_gpio_read(gpio_t* gpio, char* data, int len)
{
    int count;
    for (count = 0; count < len && gpio; count++) {
        volatile struct imx6_gpio_regs* bank;
        uint32_t v;
        int pin;

        bank = imx6_gpio_get_bank(gpio);
        pin = GPIOID_PIN(gpio->id);
        assert(pin < 32);
        assert(pin >= 0);

        v = bank->data;

        DGPIO("data(%p) <=> 0x%x\n",  &bank->data, v);
        if (v & (1U << pin)) {
            *data++ = 0xff;
        } else {
            *data++ = 0x00;
        }

        gpio = gpio->next;
    }
    return count;
}


int
imx6_gpio_init_common(mux_sys_t* mux, gpio_sys_t* gpio_sys)
{
    _gpio.mux = mux;
    gpio_sys->priv = (void*)&_gpio;
    gpio_sys->read = &imx6_gpio_read;
    gpio_sys->write = &imx6_gpio_write;
    gpio_sys->init = &imx6_gpio_init;
    return 0;
}

int
imx6_gpio_sys_init(void* bank1, void* bank2, void* bank3,
                   void* bank4, void* bank5, void* bank6,
                   void* bank7,
                   mux_sys_t* mux, gpio_sys_t* gpio_sys)
{
    if (bank1 != NULL) {
        _gpio.bank[GPIO_BANK1] = bank1;
    }
    if (bank2 != NULL) {
        _gpio.bank[GPIO_BANK2] = bank2;
    }
    if (bank3 != NULL) {
        _gpio.bank[GPIO_BANK3] = bank3;
    }
    if (bank4 != NULL) {
        _gpio.bank[GPIO_BANK4] = bank4;
    }
    if (bank5 != NULL) {
        _gpio.bank[GPIO_BANK5] = bank5;
    }
    if (bank6 != NULL) {
        _gpio.bank[GPIO_BANK6] = bank6;
    }
    if (bank7 != NULL) {
        _gpio.bank[GPIO_BANK7] = bank7;
    }
    return imx6_gpio_init_common(mux, gpio_sys);
}

int
gpio_sys_init(ps_io_ops_t* io_ops, gpio_sys_t* gpio_sys)
{
    MAP_IF_NULL(io_ops, IMX6_GPIO1, _gpio.bank[0]);
    MAP_IF_NULL(io_ops, IMX6_GPIO2, _gpio.bank[1]);
    MAP_IF_NULL(io_ops, IMX6_GPIO3, _gpio.bank[2]);
    MAP_IF_NULL(io_ops, IMX6_GPIO4, _gpio.bank[3]);
    MAP_IF_NULL(io_ops, IMX6_GPIO5, _gpio.bank[4]);
    MAP_IF_NULL(io_ops, IMX6_GPIO6, _gpio.bank[5]);
    MAP_IF_NULL(io_ops, IMX6_GPIO7, _gpio.bank[6]);
    return imx6_gpio_init_common(&io_ops->mux_sys, gpio_sys);
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_SUPPORT_IMX31_H
#define __PLAT_SUPPORT_IMX31_H

/**
 * Address at which the GPT should be in the initial
 * device space. Map a frame, uncached, to this address
 * to access the gpt. Pass the virtual address of
 * this frame onto timer_init to use the gpt.
 */
#define GPT1_DEVICE_PADDR 0x02098000
#define GPT1_INTERRUPT 87

#define GPT1_CR_EN      (1 << 0)
#define GPT1_CR_ENMOD   (1 << 1)
#define GPT1_CR_DBGEN   (1 << 2)
#define GPT1_CR_WAITEN  (1 << 3)
#define GPT1_CR_DOZEEN  (1 << 4)
#define GPT1_CR_STOPEN  (1 << 5)
#define GPT1_CR_CLKSRC  (1 << 6)
#define GPT1_CR_FRR     (1 << 9)
#define GPT1_CR_SWR     (1 << 15)
#define GPT1_CR_IM1     (1 << 16)
#define GPT1_CR_IM2     (1 << 18)
#define GPT1_CR_OM1     (1 << 20)
#define GPT1_CR_OM2     (1 << 23)
#define GPT1_CR_OM3     (1 << 26)
#define GPT1_CR_FO1     (1 << 29)
#define GPT1_CR_FO2     (1 << 30)
#define GPT1_CR_FO3     (1 << 31)

#define GPT1_CR_CLKSRC_NONE         (0b000 * GPT1_CR_CLKSRC)
#define GPT1_CR_CLKSRC_PERIPHERAL   (0b001 * GPT1_CR_CLKSRC)
#define GPT1_CR_CLKSRC_HIGHFREQREF  (0b010 * GPT1_CR_CLKSRC)
#define GPT1_CR_CLKSRC_EXTERNAL     (0b011 * GPT1_CR_CLKSRC)
#define GPT1_CR_CLKSRC_LOWFREQREF   (0b100 * GPT1_CR_CLKSRC)
#define GPT1_CR_CLKSRC_CRYSTAL8     (0b101 * GPT1_CR_CLKSRC)
#define GPT1_CR_CLKSRC_CRYSTAL      (0b111 * GPT1_CR_CLKSRC)

#define GPT1_SR_OF1     (1 << 0)
#define GPT1_SR_OF2     (1 << 1)
#define GPT1_SR_OF3     (1 << 2)
#define GPT1_SR_IF1     (1 << 3)
#define GPT1_SR_IF2     (1 << 4)
#define GPT1_SR_ROV     (1 << 5)

#define GPT1_IR_OF1IE   (1 << 0)
#define GPT1_IR_OF2IE   (1 << 1)
#define GPT1_IR_OF3IE   (1 << 2)
#define GPT1_IR_IF1IE   (1 << 3)
#define GPT1_IR_IF2IE   (1 << 4)
#define GPT1_IR_ROVIE   (1 << 5)

#endif /* __PLAT_SUPPORT_IMX31_H */

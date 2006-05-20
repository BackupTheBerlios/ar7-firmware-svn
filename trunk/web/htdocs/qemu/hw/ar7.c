/*
 *  QEMU avalanche support
 *
 *  Copyright (c) 2006 Stefan Weil
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* This code emulates Texas Instruments AR7 processor.
 * AR7 is a chip with a MIPS 4KEc core and on-chip peripherals (avalanche).
 *
 * TODO:
 * - bogomips missing
 * - uart0 wrong type (is 16450, should be 16550)
 * - uart1 missing
 * - vlynq0 emulation missing
 * - much more
 */

#include "vl.h"
#include "hw/ar7.h"	/* ar7_init */

#if defined(MIPS32_4KEc)
#endif /* MIPS32_4KEc */

#if 0
struct IoState {
    target_ulong base;
    int it_shift;
};
#endif

#define AVALANCHE_CPMAC0_BASE		0x08610000
#define AVALANCHE_CPMAC06_BASE		0x08610600
#define AVALANCHE_EMIF_BASE		0x08610800
#define AVALANCHE_GPIO_BASE             0x08610900
#define AVALANCHE_CLOCK_BASE		0x08610a00
#define AVALANCHE_WATCHDOG_BASE		0x08610b00
#define AVALANCHE_TIMER0_BASE           0x08610c00 /* Timer 1 */
#define AVALANCHE_TIMER1_BASE           0x08610d00 /* Timer 2 */
#define AVALANCHE_UART0_BASE            0x08610e00 /* UART 0 */
#define AVALANCHE_UART1_BASE            0x08610f00 /* UART 1 */
#define AVALANCHE_RESET_BASE            0x08611600
#define AVALANCHE_VLYNQ0_BASE           0x08611800 /* VLYNQ0 */
#define AVALANCHE_VLYNQ1_BASE           0x08611c00 /* VLYNQ1 */
#define AVALANCHE_MDIO_BASE		0x08611e00
//~ #define AVALANCHE_ 0x08612498       0x08610100 0x086118e0
#define AVALANCHE_INTC_BASE             0x08612400
#define AVALANCHE_CPMAC1_BASE		0x08612800
#define AVALANCHE_ 

typedef struct {
	uint32_t cpmac0[0x180];			// 0x08611000
	uint32_t cpmac06[0x20];			// 0x08611600
	uint32_t emif[64];			// 0x08610800
	uint32_t gpio[8];			// 0x08610900
		// data in, data out, dir, enable, -, cvr, didr1, didr2
	uint32_t clock_control[0x28];		// 0x08610a00
	uint32_t watchdog[7];			// 0x08610b00
	uint32_t timer0[2];
	uint32_t timer1[2];
	uint32_t uart0[8];
	uint32_t reset_control[3];		// 0x08611600
	uint32_t vlynq0[0x40];			// 0x08611800
	uint32_t vlynq1[0x40];			// 0x08611c00
	uint32_t device_config_latch;
	uint32_t interrupt_control[0x68 / 4];	// 0x08612400
	uint32_t exception_control[7];		//   +0x80
	uint32_t pacing[3];			//   +0xa0
	uint32_t channel_control[40];		//   +0x200
	uint32_t mdio[0x21];			// 0x08611e00
	uint32_t intc[0xc0];			// 0x08612400
} avalanche_t;

static avalanche_t av = {
	cpmac0: { 0 },
	emif: { 0 },
	gpio: { 0x800, 0, 0, 0 },
	clock_control: { 0 },
	timer0: { 0 },
	timer1: { 0 },
	uart0: { 0, 0, 0, 0, 0, 0x20, 0 },
	reset_control: { 0x04720043 },
	//~ device_config_latch: 0x025d4297
	device_config_latch: 0x025d4291,
	mdio: { 0 }
};

#if 0
#define AVALANCHE_ADSL_SUB_SYS_MEM_BASE       (KSEG1ADDR(0x01000000)) /* AVALANCHE ADSL Mem Base */
#define BBIF_SPACE1    			      (KSEG1ADDR(0x01800000))
#define AVALANCHE_BROADBAND_INTERFACE__BASE   (KSEG1ADDR(0x02000000)) /* AVALANCHE BBIF */
#define AVALANCHE_ATM_SAR_BASE                (KSEG1ADDR(0x03000000)) /* AVALANCHE ATM SAR */
#define AVALANCHE_USB_SLAVE_BASE              (KSEG1ADDR(0x03400000)) /* AVALANCHE USB SLAVE */
#define AVALANCHE_LOW_VLYNQ_MEM_MAP_BASE      (KSEG1ADDR(0x04000000)) /* AVALANCHE VLYNQ 0 Mem map */
#define AVALANCHE_CLOCK_CONTROL_BASE          (KSEG1ADDR(0x08610A00)) /* AVALANCHE Clock Control */
#define AVALANCHE_WATCHDOG_TIMER_BASE         (KSEG1ADDR(0x08610B00)) /* AVALANCHE Watch Dog Timer */
#define AVALANCHE_I2C_BASE                    (KSEG1ADDR(0x08611000)) /* AVALANCHE I2C */
#define DEV_ID_BASE                           (KSEG1ADDR(0x08611100))
#define AVALANCHE_USB_SLAVE_CONTROL_BASE      (KSEG1ADDR(0x08611200)) /* AVALANCHE USB DMA */
#define PCI_CONFIG_BASE                       (KSEG1ADDR(0x08611300))
#define AVALANCHE_MCDMA0_CTRL_BASE            (KSEG1ADDR(0x08611400)) /* AVALANCHE MC DMA 0 (channels 0-3) */
#define TNETD73xx_VDMAVT_BASE                 PHYS_TO_K1(0x08611500)      /* VDMAVT Control */
#define AVALANCHE_BIST_CONTROL_BASE           (KSEG1ADDR(0x08611700)) /* AVALANCHE BIST Control */
#define AVALANCHE_DEVICE_CONFIG_LATCH_BASE    (KSEG1ADDR(0x08611A00)) /* AVALANCHE Device Config Latch */
#define DSL_IF_BASE                           (KSEG1ADDR(0x08611B00))
#define AVALANCHE_MDIO_BASE                   (KSEG1ADDR(0x08611E00)) /* AVALANCHE MDIO    */
#define AVALANCHE_FSER_BASE                   (KSEG1ADDR(0x08612000)) /* AVALANCHE FSER base */
#define AVALANCHE_HIGH_VLYNQ_MEM_MAP_BASE     (KSEG1ADDR(0x0C000000)) /* AVALANCHE VLYNQ 1 Mem map */
#define PHY_BASE                              (KSEG1ADDR(0x1E000000))
08611600  43 00 72 04 00 00 00 00  00 00 00 00 00 00 00 00  |C.r.............|
08611610  43 00 72 04 00 00 00 00  00 00 00 00 00 00 00 00  |C.r.............|
08611a00  91 42 5d 02 00 00 00 00  00 00 00 00 00 00 00 00  |.B].............|
08611a10  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
08611b00  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
#endif

#define INRANGE(base, var) \
	(((addr) >= (base)) && ((addr) < ((base) + (sizeof(var)) - 1)))

#define VALUE(base, var) var[(addr - (base)) / 4]

static uint32_t mips_io_memread(void *opaque, uint32_t addr)
{
	uint32_t index;
	uint32_t val = 0xffffffff;
	addr |= 0x08610000;
#if 0
	printf("%s: addr 0x%08lx\n", __func__, (unsigned long)addr);
#endif
	if (INRANGE(AVALANCHE_CPMAC0_BASE, av.cpmac0)) {
		val = VALUE(AVALANCHE_CPMAC0_BASE, av.cpmac0);
	} else if (INRANGE(AVALANCHE_CPMAC06_BASE, av.cpmac06)) {
		val = VALUE(AVALANCHE_CPMAC06_BASE, av.cpmac06);
	} else if (INRANGE(AVALANCHE_EMIF_BASE, av.emif)) {
		val = VALUE(AVALANCHE_EMIF_BASE, av.emif);
	} else if (INRANGE(AVALANCHE_GPIO_BASE, av.gpio)) {
#if 0
		printf("%s: addr 0x%08lx (gpio)\n", __func__, (unsigned long)addr);
#endif
		val = VALUE(AVALANCHE_GPIO_BASE, av.gpio);
	} else if (INRANGE(AVALANCHE_CLOCK_BASE, av.clock_control)) {
		index = (addr - AVALANCHE_CLOCK_BASE) / 4;
		val = av.clock_control[index];
		if ((val != 0) && (index == 0x0c || index == 0x14)) {
			av.clock_control[index]--;
		}
	} else if (INRANGE(AVALANCHE_WATCHDOG_BASE, av.watchdog)) {
		val = VALUE(AVALANCHE_WATCHDOG_BASE, av.watchdog);
	} else if (INRANGE(AVALANCHE_TIMER0_BASE, av.timer0)) {
		val = VALUE(AVALANCHE_TIMER0_BASE, av.timer0);
	//~ } else if (INRANGE(AVALANCHE_UART0_BASE, av.uart0)) {
		//~ val = VALUE(AVALANCHE_UART0_BASE, av.uart0);
	} else if (INRANGE(AVALANCHE_RESET_BASE, av.reset_control)) {
#if 0
		printf("%s: addr 0x%08lx (reset control)\n", __func__, (unsigned long)addr);
#endif
		val = VALUE(AVALANCHE_RESET_BASE, av.reset_control);
	} else if (addr == 0x08611a00) {
		val = av.device_config_latch;
	} else if (INRANGE(AVALANCHE_VLYNQ0_BASE, av.vlynq0)) {
		val = VALUE(AVALANCHE_VLYNQ0_BASE, av.vlynq0);
	} else if (INRANGE(AVALANCHE_VLYNQ1_BASE, av.vlynq1)) {
		val = VALUE(AVALANCHE_VLYNQ1_BASE, av.vlynq1);
	} else if (INRANGE(AVALANCHE_MDIO_BASE, av.mdio)) {
		val = VALUE(AVALANCHE_MDIO_BASE, av.mdio);
	} else {
		printf("%s: unknown address 0x%08lx\n", __func__, (unsigned long)addr);
	}
	return val;
}

static void mips_io_memwrite(void *opaque, uint32_t addr, uint32_t val)
{
	addr |= 0x08610000;
#if 0
	printf("%s: addr 0x%08lx val 0x%08lx\n",
		__func__, (unsigned long)addr, (unsigned long)val);
#endif
	if (INRANGE(AVALANCHE_CPMAC0_BASE, av.cpmac0)) {
		VALUE(AVALANCHE_CPMAC0_BASE, av.cpmac0) = val;
	} else if (INRANGE(AVALANCHE_CPMAC06_BASE, av.cpmac06)) {
		VALUE(AVALANCHE_CPMAC06_BASE, av.cpmac06) = val;
	} else if (INRANGE(AVALANCHE_EMIF_BASE, av.emif)) {
		VALUE(AVALANCHE_EMIF_BASE, av.emif) = val;
	} else if (INRANGE(AVALANCHE_GPIO_BASE, av.gpio)) {
#if 0
		printf("%s: addr 0x%08lx (gpio)\n", __func__, (unsigned long)addr);
#endif
		VALUE(AVALANCHE_GPIO_BASE, av.gpio) = val;
	} else if (INRANGE(AVALANCHE_CLOCK_BASE, av.clock_control)) {
		VALUE(AVALANCHE_CLOCK_BASE, av.clock_control) = val;
	} else if (INRANGE(AVALANCHE_WATCHDOG_BASE, av.watchdog)) {
		VALUE(AVALANCHE_WATCHDOG_BASE, av.watchdog) = val;
	} else if (INRANGE(AVALANCHE_TIMER0_BASE, av.timer0)) {
		VALUE(AVALANCHE_TIMER0_BASE, av.timer0) = val;
	//~ } else if (addr >= 0x08610e00 && addr < 0x08610e1f) {
		//~ VALUE(AVALANCHE_UART0_BASE, av.uart0) = val;
	} else if (INRANGE(AVALANCHE_RESET_BASE, av.reset_control)) {
#if 0
		printf("%s: addr 0x%08lx (reset control)\n", __func__, (unsigned long)addr);
#endif
		VALUE(AVALANCHE_RESET_BASE, av.reset_control) = val;
	} else if (addr == 0x08611a00) {
		av.device_config_latch = val;
	} else if (INRANGE(AVALANCHE_VLYNQ0_BASE, av.vlynq0)) {
		VALUE(AVALANCHE_VLYNQ0_BASE, av.vlynq0) = val;
	} else if (INRANGE(AVALANCHE_VLYNQ1_BASE, av.vlynq1)) {
		VALUE(AVALANCHE_VLYNQ1_BASE, av.vlynq1) = val;
	} else if (INRANGE(AVALANCHE_MDIO_BASE, av.mdio)) {
		VALUE(AVALANCHE_MDIO_BASE, av.mdio) = val;
	} else if (INRANGE(AVALANCHE_INTC_BASE, av.intc)) {
		VALUE(AVALANCHE_INTC_BASE, av.intc) = val;		
	} else {
		printf("%s: unknown address 0x%08lx\n", __func__, (unsigned long)addr);
	}
}

#if 0
static uint32_t mips_mm_readb (void *opaque, target_phys_addr_t addr)
{
    SerialState *s = opaque;
    printf("%s: addr 0x%08lx\n", __func__, (unsigned long)addr);
    return mips_ioport_read(s, (addr - s->base) >> s->it_shift) & 0xFF;
}

static void mips_mm_writeb (void *opaque,
                              target_phys_addr_t addr, uint32_t value)
{
    SerialState *s = opaque;
    printf("%s: addr 0x%08lx val 0x%08lx\n",
	   __func__, (unsigned long)addr, (unsigned long)value);

    mips_ioport_write(s, (addr - s->base) >> s->it_shift, value & 0xFF);
}

static uint32_t mips_mm_readw (void *opaque, target_phys_addr_t addr)
{
    SerialState *s = opaque;

    return mips_ioport_read(s, (addr - s->base) >> s->it_shift) & 0xFFFF;
}

static void mips_mm_writew (void *opaque,
                              target_phys_addr_t addr, uint32_t value)
{
    SerialState *s = opaque;

    mips_ioport_write(s, (addr - s->base) >> s->it_shift, value & 0xFFFF);
}

static uint32_t mips_mm_readl (void *opaque, target_phys_addr_t addr)
{
    SerialState *s = opaque;

    return mips_ioport_read(s, (addr - s->base) >> s->it_shift);
}

static void mips_mm_writel (void *opaque,
                              target_phys_addr_t addr, uint32_t value)
{
    SerialState *s = opaque;

    mips_ioport_write(s, (addr - s->base) >> s->it_shift, value);
}

static CPUReadMemoryFunc *mips_mm_read[] = {
    &mips_mm_readb,
    &mips_mm_readw,
    &mips_mm_readl,
};

static CPUWriteMemoryFunc *mips_mm_write[] = {
    &mips_mm_writeb,
    &mips_mm_writew,
    &mips_mm_writel,
};
#endif

static void io_writeb (void *opaque, target_phys_addr_t addr, uint32_t value)
{
#if 0
    if (logfile)
        fprintf(logfile, "%s: addr %08x val %08x\n", __func__, addr, value);
#endif
    cpu_outb(NULL, addr & 0xffff, value);
}

static uint32_t io_readb (void *opaque, target_phys_addr_t addr)
{
    uint32_t ret = cpu_inb(NULL, addr & 0xffff);
#if 0
    if (logfile)
        fprintf(logfile, "%s: addr %08x val %08x\n", __func__, addr, ret);
#endif
    return ret;
}

static void io_writew (void *opaque, target_phys_addr_t addr, uint32_t value)
{
#if 0
    if (logfile)
        fprintf(logfile, "%s: addr %08x val %08x\n", __func__, addr, value);
#endif
#ifdef TARGET_WORDS_BIGENDIAN
    value = bswap16(value);
#endif
    cpu_outw(NULL, addr & 0xffff, value);
}

static uint32_t io_readw (void *opaque, target_phys_addr_t addr)
{
    uint32_t ret = cpu_inw(NULL, addr & 0xffff);
#ifdef TARGET_WORDS_BIGENDIAN
    ret = bswap16(ret);
#endif
#if 0
    if (logfile)
        fprintf(logfile, "%s: addr %08x val %08x\n", __func__, addr, ret);
#endif
    return ret;
}

static void io_writel (void *opaque, target_phys_addr_t addr, uint32_t value)
{
#if 0
    if (logfile)
        fprintf(logfile, "%s: addr %08x val %08x\n", __func__, addr, value);
#endif
#ifdef TARGET_WORDS_BIGENDIAN
    value = bswap32(value);
#endif
    cpu_outl(NULL, addr & 0xffff, value);
}

static uint32_t io_readl (void *opaque, target_phys_addr_t addr)
{
    uint32_t ret = cpu_inl(NULL, addr & 0xffff);

#ifdef TARGET_WORDS_BIGENDIAN
    ret = bswap32(ret);
#endif
#if 0
    if (logfile)
        fprintf(logfile, "%s: addr %08x val %08x\n", __func__, addr, ret);
#endif
    return ret;
}

static CPUWriteMemoryFunc *io_write[] = {
    &io_writeb,
    &io_writew,
    &io_writel,
};

static CPUReadMemoryFunc *io_read[] = {
    &io_readb,
    &io_readw,
    &io_readl,
};

void ar7_init(void)
{
    target_phys_addr_t addr = (0x08610000 & 0xffff);
    unsigned offset;
    int io_memory = cpu_register_io_memory(0, io_read, io_write, NULL);
    cpu_register_physical_memory(0x08610000, 0x0002800, io_memory);
    serial_init(0, 0, 0x08610e00 & 0xffff, 2, 4, serial_hds[0]);
    //~ serial_init(0, 0, 0x08610f00 & 0xffff, 2, 4, serial_hds[1]);
    for (offset = 0; offset < 0x2800; offset += 0x100) {
	if (offset == 0xe00) continue;
	if (offset == 0xf00) continue;
        register_ioport_read(addr + offset, 0x100, 1, mips_io_memread, 0);
	register_ioport_read(addr + offset, 0x100, 2, mips_io_memread, 0);
	register_ioport_read(addr + offset, 0x100, 4, mips_io_memread, 0);
	register_ioport_write(addr + offset, 0x100, 1, mips_io_memwrite, 0);
	register_ioport_write(addr + offset, 0x100, 2, mips_io_memwrite, 0);
	register_ioport_write(addr + offset, 0x100, 4, mips_io_memwrite, 0);
    }
    //~ {
	    //~ struct SerialState state = {
		    //~ base: 0,
		    //~ it_shift: 0
	    //~ };
    //~ s_io_memory = cpu_register_io_memory(&state, mips_mm_read, mips_mm_write, 0);
    //~ cpu_register_physical_memory(0x08610000, 0x2000, s_io_memory);
    //~ }
}


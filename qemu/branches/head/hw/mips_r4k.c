#include "vl.h"

#define BIOS_FILENAME "mips_bios.bin"
//#define BIOS_FILENAME "system.bin"
#define KERNEL_LOAD_ADDR 0x80010000
#define INITRD_LOAD_ADDR 0x80800000

#define VIRT_TO_PHYS_ADDEND (-0x80000000LL)

static const int ide_iobase[2] = { 0x1f0, 0x170 };
static const int ide_iobase2[2] = { 0x3f6, 0x376 };
static const int ide_irq[2] = { 14, 15 };

extern FILE *logfile;

static PITState *pit;

static void pic_irq_request(void *opaque, int level)
{
    CPUState *env = first_cpu;
    if (level) {
        env->CP0_Cause |= 0x00000400;
        cpu_interrupt(env, CPU_INTERRUPT_HARD);
    } else {
	env->CP0_Cause &= ~0x00000400;
        cpu_reset_interrupt(env, CPU_INTERRUPT_HARD);
    }
}

void cpu_mips_irqctrl_init (void)
{
}

/* XXX: do not use a global */
uint32_t cpu_mips_get_random (CPUState *env)
{
    static uint32_t seed = 0;
    uint32_t idx;
    seed = seed * 314159 + 1;
    idx = (seed >> 16) % (MIPS_TLB_NB - env->CP0_Wired) + env->CP0_Wired;
    return idx;
}

/* MIPS R4K timer */
uint32_t cpu_mips_get_count (CPUState *env)
{
    return env->CP0_Count +
        (uint32_t)muldiv64(qemu_get_clock(vm_clock),
                           100 * 1000 * 1000, ticks_per_sec);
}

static void cpu_mips_update_count (CPUState *env, uint32_t count,
                                   uint32_t compare)
{
    uint64_t now, next;
    uint32_t tmp;
    
    tmp = count;
    if (count == compare)
        tmp++;
    now = qemu_get_clock(vm_clock);
    next = now + muldiv64(compare - tmp, ticks_per_sec, 100 * 1000 * 1000);
    if (next == now)
	next++;
#if 0
    if (logfile) {
        fprintf(logfile, "%s: 0x%08" PRIx64 " %08x %08x => 0x%08" PRIx64 "\n",
                __func__, now, count, compare, next - now);
    }
#endif
    /* Store new count and compare registers */
    env->CP0_Compare = compare;
    env->CP0_Count =
        count - (uint32_t)muldiv64(now, 100 * 1000 * 1000, ticks_per_sec);
    /* Adjust timer */
    qemu_mod_timer(env->timer, next);
}

void cpu_mips_store_count (CPUState *env, uint32_t value)
{
    cpu_mips_update_count(env, value, env->CP0_Compare);
}

void cpu_mips_store_compare (CPUState *env, uint32_t value)
{
    cpu_mips_update_count(env, cpu_mips_get_count(env), value);
    env->CP0_Cause &= ~0x00008000;
    cpu_reset_interrupt(env, CPU_INTERRUPT_HARD);
}

static void mips_timer_cb (void *opaque)
{
    CPUState *env;

    env = opaque;
#if 0
    if (logfile) {
        fprintf(logfile, "%s\n", __func__);
    }
#endif
    cpu_mips_update_count(env, cpu_mips_get_count(env), env->CP0_Compare);
    env->CP0_Cause |= 0x00008000;
    cpu_interrupt(env, CPU_INTERRUPT_HARD);
}

void cpu_mips_clock_init (CPUState *env)
{
    env->timer = qemu_new_timer(vm_clock, &mips_timer_cb, env);
    env->CP0_Compare = 0;
    cpu_mips_update_count(env, 1, 0);
}

static void main_cpu_reset(void *opaque)
{
    CPUState *env = opaque;
    cpu_reset(env);
}

void mips_r4k_init (int ram_size, int vga_ram_size, int boot_device,
                    DisplayState *ds, const char **fd_filename, int snapshot,
                    const char *kernel_filename, const char *kernel_cmdline,
                    const char *initrd_filename)
{
    char buf[1024];
    int64_t entry = 0;
    unsigned long bios_offset;
    int ret;
    CPUState *env;
    long kernel_size;
    int i;

    env = cpu_init();
    register_savevm("cpu", 0, 3, cpu_save, cpu_load, env);
    qemu_register_reset(main_cpu_reset, env);

    /* allocate RAM */
    cpu_register_physical_memory(0, ram_size, IO_MEM_RAM);

    /* Try to load a BIOS image. If this fails, we continue regardless,
       but initialize the hardware ourselves. When a kernel gets
       preloaded we also initialize the hardware, since the BIOS wasn't
       run. */
    bios_offset = ram_size + vga_ram_size;
    snprintf(buf, sizeof(buf), "%s/%s", bios_dir, BIOS_FILENAME);
    ret = load_image(buf, phys_ram_base + bios_offset);
    if (ret == BIOS_SIZE) {
	cpu_register_physical_memory((uint32_t)(0x1fc00000),
				     BIOS_SIZE, bios_offset | IO_MEM_ROM);
    } else {
	/* not fatal */
        fprintf(stderr, "qemu: Warning, could not load MIPS bios '%s'\n",
		buf);
    }

    kernel_size = 0;
    if (kernel_filename) {
	kernel_size = load_elf(kernel_filename, VIRT_TO_PHYS_ADDEND, &entry);
	if (kernel_size >= 0)
	    env->PC = entry;
	else {
	    kernel_size = load_image(kernel_filename,
                                     phys_ram_base + KERNEL_LOAD_ADDR + VIRT_TO_PHYS_ADDEND);
            if (kernel_size < 0) {
                fprintf(stderr, "qemu: could not load kernel '%s'\n",
                        kernel_filename);
                exit(1);
            }
            env->PC = KERNEL_LOAD_ADDR;
	}

        /* load initrd */
        if (initrd_filename) {
            if (load_image(initrd_filename,
			   phys_ram_base + INITRD_LOAD_ADDR + VIRT_TO_PHYS_ADDEND)
		== (target_ulong) -1) {
                fprintf(stderr, "qemu: could not load initial ram disk '%s'\n", 
                        initrd_filename);
                exit(1);
            }
        }

	/* Store command line.  */
        strcpy (phys_ram_base + (16 << 20) - 256, kernel_cmdline);
        /* FIXME: little endian support */
        *(int *)(phys_ram_base + (16 << 20) - 260) = tswap32 (0x12345678);
        *(int *)(phys_ram_base + (16 << 20) - 264) = tswap32 (ram_size);
    }

    /* Init internal devices */
    cpu_mips_clock_init(env);
    cpu_mips_irqctrl_init();

    /* Register 64 KB of ISA IO space at 0x14000000 */
    isa_mmio_init(0x14000000, 0x00010000);
    isa_mem_base = 0x10000000;

    isa_pic = pic_init(pic_irq_request, env);
    pit = pit_init(0x40, 0);
    serial_init(&pic_set_irq_new, isa_pic, 0x3f8, 4, serial_hds[0]);
    isa_vga_init(ds, phys_ram_base + ram_size, ram_size, 
                 vga_ram_size);

    if (nd_table[0].vlan) {
        if (nd_table[0].model == NULL
            || strcmp(nd_table[0].model, "ne2k_isa") == 0) {
            isa_ne2000_init(0x300, 9, &nd_table[0]);
        } else {
            fprintf(stderr, "qemu: Unsupported NIC: %s\n", nd_table[0].model);
            exit (1);
        }
    }

    for(i = 0; i < 2; i++)
        isa_ide_init(ide_iobase[i], ide_iobase2[i], ide_irq[i],
                     bs_table[2 * i], bs_table[2 * i + 1]);
}

QEMUMachine mips_machine = {
    "mips",
    "mips r4k platform",
    mips_r4k_init,
};
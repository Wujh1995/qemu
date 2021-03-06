/*
 * ARM gdb server stub
 *
 * Copyright (c) 2003-2005 Fabrice Bellard
 * Copyright (c) 2013 SUSE LINUX Products GmbH
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#include "qemu-common.h"
#include "exec/gdbstub.h"

/* Old gdb always expect FPA registers.  Newer (xml-aware) gdb only expect
   whatever the target description contains.  Due to a historical mishap
   the FPA registers appear in between core integer regs and the CPSR.
   We hack round this by giving the FPA regs zero size when talking to a
   newer gdb.  */

int arm_cpu_gdb_read_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = &cpu->env;

    if (n < 16) {
        /* Core integer register.  */
        return gdb_get_reg32(mem_buf, env->regs[n]);
    }
    if (n < 24) {
        /* FPA registers.  */
        if (gdb_has_xml) {
            return 0;
        }
        memset(mem_buf, 0, 12);
        return 12;
    }
    switch (n) {
    case 24:
        /* FPA status register.  */
        if (gdb_has_xml) {
            return 0;
        }
        return gdb_get_reg32(mem_buf, 0);
    case 25:
        /* CPSR */
        return gdb_get_reg32(mem_buf, cpsr_read(env));
    case 26:
        return gdb_get_reg32(mem_buf, env->cp15.c0_cpuid);
    case 27:
        return gdb_get_reg32(mem_buf, env->cp15.c1_coproc);
    case 28:
        return gdb_get_reg32(mem_buf, env->cp15.c2_mask);
    case 29:
        return gdb_get_reg32(mem_buf, env->cp15.c2_base_mask);
    case 30:
        return gdb_get_reg32(mem_buf, env->cp15.c2_data);
    case 31:
        return gdb_get_reg32(mem_buf, env->cp15.c2_insn);
    case 32:
        return gdb_get_reg32(mem_buf, env->cp15.esr_el[1]);
    case 33:
        return gdb_get_reg32(mem_buf, mpidr_read_val(env));
    case 34:
        return gdb_get_reg32(mem_buf, env->elr_el[1]);
    }
    /* Unknown register.  */
    return 0;
}

int arm_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = &cpu->env;
    uint32_t tmp;

    tmp = ldl_p(mem_buf);

    /* Mask out low bit of PC to workaround gdb bugs.  This will probably
       cause problems if we ever implement the Jazelle DBX extensions.  */
    if (n == 15) {
        tmp &= ~1;
    }

    if (n < 16) {
        /* Core integer register.  */
        env->regs[n] = tmp;
        return 4;
    }
    if (n < 24) { /* 16-23 */
        /* FPA registers (ignored).  */
        if (gdb_has_xml) {
            return 0;
        }
        return 12;
    }
    switch (n) {
    case 24:
        /* FPA status register (ignored).  */
        if (gdb_has_xml) {
            return 0;
        }
        return 4;
    case 25:
        /* CPSR */
        cpsr_write(env, tmp, 0xffffffff);
        return 4;
    case 26:
        env->cp15.c0_cpuid = tmp;
        return 4;
    case 27:
        env->cp15.c1_coproc = tmp;
        return 4;
    case 28:
        env->cp15.c2_mask = tmp;
        return 4;
    case 29:
        env->cp15.c2_base_mask = tmp;
        return 4;
    case 30:
        env->cp15.c2_data = tmp;
        return 4;
    case 31:
        env->cp15.c2_insn = tmp;
        return 4;
    case 32:
        env->cp15.esr_el[1] = tmp;
        return 4;
    case 33:
        /* Writing to the MPIDR is not supported */
        return 0;
    case 34:
        env->elr_el[1] = tmp;
        return 4;
    }
    /* Unknown register.  */
    return 0;
}

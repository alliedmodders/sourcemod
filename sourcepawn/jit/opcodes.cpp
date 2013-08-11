/**
 * vim: set ts=8 sw=2 tw=99 sts=2 et:
 * =============================================================================
 * SourceMod
 * Copyright _(C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License), version 3.0), as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful), but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not), see <http://www.gnu.org/licenses/>.
 *
 * As a special exception), AlliedModders LLC gives you permission to link the
 * code of this program _(as well as its derivative works) to "Half-Life 2)," the
 * "Source Engine)," the "SourcePawn JIT)," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally), AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions), found in LICENSE.txt _(as of this writing), version JULY-31-2007)),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
#include "opcodes.h"
#include "jit_shared.h"

using namespace SourcePawn;

const char *OpcodeNames[] = {
#define _(op, text) text,
  OPCODE_LIST(_)
#undef _
  NULL
};

void
SourcePawn::SpewOpcode(const sp_plugin_t *plugin, cell_t *start, cell_t *cip)
{
  fprintf(stdout, "  [%05d:%04d]", cip - (cell_t *)plugin->pcode, cip - start);

  if (*cip >= OPCODES_TOTAL) {
    fprintf(stdout, " unknown-opcode\n");
    return;
  }

  OPCODE op = (OPCODE)*cip;
  fprintf(stdout, " %s ", OpcodeNames[op]);

  switch (op) {
    case OP_PUSH_C:
    case OP_PUSH_ADR:
    case OP_SHL_C_PRI:
    case OP_SHL_C_ALT:
    case OP_SHR_C_PRI:
    case OP_SHR_C_ALT:
    case OP_ADD_C:
    case OP_SMUL_C:
    case OP_EQ_C_PRI:
    case OP_EQ_C_ALT:
    case OP_TRACKER_PUSH_C:
    case OP_STACK:
    case OP_PUSH_S:
    case OP_HEAP:
    case OP_GENARRAY:
    case OP_GENARRAY_Z:
    case OP_CONST_PRI:
    case OP_CONST_ALT:
      fprintf(stdout, "%d", cip[1]);
      break;

    case OP_JUMP:
    case OP_JZER:
    case OP_JNZ:
    case OP_JEQ:
    case OP_JNEQ:
    case OP_JSLESS:
    case OP_JSGRTR:
    case OP_JSGEQ:
    case OP_JSLEQ:
      fprintf(stdout, "%05d:%04d",
        cip[1] / 4,
        ((cell_t *)plugin->pcode + cip[1] / 4) - start);
      break;

    case OP_SYSREQ_C:
    case OP_SYSREQ_N:
    {
      uint32_t index = cip[1];
      if (index < plugin->num_natives)
        fprintf(stdout, "%s", plugin->natives[index].name); 
      if (op == OP_SYSREQ_N)
        fprintf(stdout, " ; (%d args, index %d)", cip[2], index);
      else
        fprintf(stdout, " ; (index %d)", index);
      break;
    }

    case OP_PUSH2_C:
    case OP_PUSH2:
    case OP_PUSH2_S:
    case OP_PUSH2_ADR:
      fprintf(stdout, "%d, %d", cip[1], cip[2]);
      break;

    case OP_PUSH3_C:
    case OP_PUSH3:
    case OP_PUSH3_S:
    case OP_PUSH3_ADR:
      fprintf(stdout, "%d, %d, %d", cip[1], cip[2], cip[3]);
      break;

    case OP_PUSH4_C:
    case OP_PUSH4:
    case OP_PUSH4_S:
    case OP_PUSH4_ADR:
      fprintf(stdout, "%d, %d, %d, %d", cip[1], cip[2], cip[3], cip[4]);
      break;

    case OP_PUSH5_C:
    case OP_PUSH5:
    case OP_PUSH5_S:
    case OP_PUSH5_ADR:
      fprintf(stdout, "%d, %d, %d, %d, %d", cip[1], cip[2], cip[3], cip[4], cip[5]);
      break;

    default:
      break;
  }

  fprintf(stdout, "\n");
}


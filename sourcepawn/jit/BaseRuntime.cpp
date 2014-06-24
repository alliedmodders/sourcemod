// vim: set ts=8 sts=2 sw=2 tw=99 et:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BaseRuntime.h"
#include "sp_vm_engine.h"
#include "x86/jit_x86.h"
#include "sp_vm_basecontext.h"
#include "engine2.h"

#include "md5/md5.h"

using namespace SourcePawn;

static inline bool
IsPointerCellAligned(void *p)
{
  return uintptr_t(p) % 4 == 0;
}

BaseRuntime::BaseRuntime()
  : m_Debug(&m_plugin),
    m_pCtx(NULL), 
    m_PubFuncs(NULL),
    m_PubJitFuncs(NULL),
    co_(NULL),
    m_CompSerial(0)
{
  memset(&m_plugin, 0, sizeof(m_plugin));

  m_MaxFuncs = 0;
  m_NumFuncs = 0;
  float_table_ = NULL;
  function_map_ = NULL;
  alt_pcode_ = NULL;
  
  memset(m_CodeHash, 0, sizeof(m_CodeHash));
  memset(m_DataHash, 0, sizeof(m_DataHash));

  ke::AutoLock lock(g_Jit.Mutex());
  g_Jit.RegisterRuntime(this);
}

BaseRuntime::~BaseRuntime()
{
  // The watchdog thread takes the global JIT lock while it patches all
  // runtimes. It is not enough to ensure that the unlinking of the runtime is
  // protected; we cannot delete functions or code while the watchdog might be
  // executing. Therefore, the entire destructor is guarded.
  ke::AutoLock lock(g_Jit.Mutex());

  g_Jit.DeregisterRuntime(this);

  for (uint32_t i = 0; i < m_plugin.num_publics; i++)
    delete m_PubFuncs[i];
  delete [] m_PubFuncs;
  delete [] m_PubJitFuncs;
  delete [] float_table_;
  delete [] function_map_;
  delete [] alt_pcode_;

  for (size_t i = 0; i < m_JitFunctions.length(); i++)
    delete m_JitFunctions[i];

  delete m_pCtx;
  if (co_)
    co_->Abort();

  free(m_plugin.base);
  delete [] m_plugin.memory;
  delete [] m_plugin.publics;
  delete [] m_plugin.pubvars;
  delete [] m_plugin.natives;
  free(m_plugin.name);
}

struct NativeMapping {
  const char *name;
  unsigned opcode;
};

static const NativeMapping sNativeMap[] = {
  { "FloatAbs",       OP_FABS },
  { "FloatAdd",       OP_FLOATADD },
  { "FloatSub",       OP_FLOATSUB },
  { "FloatMul",       OP_FLOATMUL },
  { "FloatDiv",       OP_FLOATDIV },
  { "float",          OP_FLOAT },
  { "FloatCompare",   OP_FLOATCMP },
  { "RoundToCeil",    OP_RND_TO_CEIL },
  { "RoundToZero",    OP_RND_TO_ZERO },
  { "RoundToFloor",   OP_RND_TO_FLOOR },
  { "RoundToNearest", OP_RND_TO_NEAREST },
  { "__FLOAT_GT__",   OP_FLOAT_GT },
  { "__FLOAT_GE__",   OP_FLOAT_GE },
  { "__FLOAT_LT__",   OP_FLOAT_LT },
  { "__FLOAT_LE__",   OP_FLOAT_LE },
  { "__FLOAT_EQ__",   OP_FLOAT_EQ },
  { "__FLOAT_NE__",   OP_FLOAT_NE },
  { "__FLOAT_NOT__",  OP_FLOAT_NOT },
  { NULL,             0 },
};

void
BaseRuntime::SetupFloatNativeRemapping()
{
  float_table_ = new floattbl_t[m_plugin.num_natives];
  for (size_t i = 0; i < m_plugin.num_natives; i++) {
    const char *name = m_plugin.natives[i].name;
    const NativeMapping *iter = sNativeMap;
    while (iter->name) {
      if (strcmp(name, iter->name) == 0) {
        float_table_[i].found = true;
        float_table_[i].index = iter->opcode;
        break;
      }
      iter++;
    }
  }
}

unsigned
BaseRuntime::GetNativeReplacement(size_t index)
{
  if (!float_table_[index].found)
    return OP_NOP;
  return float_table_[index].index;
}

void
BaseRuntime::SetName(const char *name)
{
  m_plugin.name = strdup(name);
}

static cell_t InvalidNative(IPluginContext *pCtx, const cell_t *params)
{
  return pCtx->ThrowNativeErrorEx(SP_ERROR_INVALID_NATIVE, "Invalid native");
}

int BaseRuntime::CreateFromMemory(sp_file_hdr_t *hdr, uint8_t *base)
{
  char *nameptr;
  uint8_t sectnum = 0;
  sp_file_section_t *secptr = (sp_file_section_t *)(base + sizeof(sp_file_hdr_t));

  memset(&m_plugin, 0, sizeof(m_plugin));

  m_plugin.base = base;
  m_plugin.base_size = hdr->imagesize;

  if (hdr->version == 0x0101)
    m_plugin.debug.unpacked = true;

  /* We have to read the name section first */
  for (sectnum = 0; sectnum < hdr->sections; sectnum++) {
    nameptr = (char *)(base + hdr->stringtab + secptr[sectnum].nameoffs);
    if (strcmp(nameptr, ".names") == 0) {
      m_plugin.stringbase = (const char *)(base + secptr[sectnum].dataoffs);
      break;
    }
  }

  sectnum = 0;

  /* Now read the rest of the sections */
  while (sectnum < hdr->sections) {
    nameptr = (char *)(base + hdr->stringtab + secptr->nameoffs);

    if (!(m_plugin.pcode) && !strcmp(nameptr, ".code")) {
      sp_file_code_t *cod = (sp_file_code_t *)(base + secptr->dataoffs);

      if (cod->codeversion < SP_CODEVERS_JIT1)
        return SP_ERROR_CODE_TOO_OLD;
      if (cod->codeversion > SP_CODEVERS_JIT2)
        return SP_ERROR_CODE_TOO_NEW;

      m_plugin.pcode = base + secptr->dataoffs + cod->code;
      m_plugin.pcode_size = cod->codesize;
      m_plugin.flags = cod->flags;
      m_plugin.pcode_version = cod->codeversion;
      if (!IsPointerCellAligned(m_plugin.pcode)) {
        // The JIT requires that pcode is cell-aligned, so if it's not, we
        // remap the code segment to a new address.
        alt_pcode_ = new uint8_t[m_plugin.pcode_size];
        memcpy(alt_pcode_, m_plugin.pcode, m_plugin.pcode_size);
        assert(IsPointerCellAligned(alt_pcode_));

        m_plugin.pcode = alt_pcode_;
      }
    } else if (!(m_plugin.data) && !strcmp(nameptr, ".data")) {
      sp_file_data_t *dat = (sp_file_data_t *)(base + secptr->dataoffs);
      m_plugin.data = base + secptr->dataoffs + dat->data;
      m_plugin.data_size = dat->datasize;
      m_plugin.mem_size = dat->memsize;
      m_plugin.memory = new uint8_t[m_plugin.mem_size];
      memcpy(m_plugin.memory, m_plugin.data, m_plugin.data_size);
    } else if ((m_plugin.publics == NULL) && !strcmp(nameptr, ".publics")) {
      sp_file_publics_t *publics;

      publics = (sp_file_publics_t *)(base + secptr->dataoffs);
      m_plugin.num_publics = secptr->size / sizeof(sp_file_publics_t);

      if (m_plugin.num_publics > 0) {
        m_plugin.publics = new sp_public_t[m_plugin.num_publics];

        for (uint32_t i = 0; i < m_plugin.num_publics; i++) {
          m_plugin.publics[i].code_offs = publics[i].address;
          m_plugin.publics[i].funcid = (i << 1) | 1;
          m_plugin.publics[i].name = m_plugin.stringbase + publics[i].name;
        }
      }
    } else if ((m_plugin.pubvars == NULL) && !strcmp(nameptr, ".pubvars")) {
      sp_file_pubvars_t *pubvars;

      pubvars = (sp_file_pubvars_t *)(base + secptr->dataoffs);
      m_plugin.num_pubvars = secptr->size / sizeof(sp_file_pubvars_t);

      if (m_plugin.num_pubvars > 0) {
        m_plugin.pubvars = new sp_pubvar_t[m_plugin.num_pubvars];

        for (uint32_t i = 0; i < m_plugin.num_pubvars; i++) {
          m_plugin.pubvars[i].name = m_plugin.stringbase + pubvars[i].name;
          m_plugin.pubvars[i].offs = (cell_t *)(m_plugin.memory + pubvars[i].address);
        }
      }
    } else if ((m_plugin.natives == NULL) && !strcmp(nameptr, ".natives")) {
      sp_file_natives_t *natives;

      natives = (sp_file_natives_t *)(base + secptr->dataoffs);
      m_plugin.num_natives = secptr->size / sizeof(sp_file_natives_t);

      if (m_plugin.num_natives > 0) {
        m_plugin.natives = new sp_native_t[m_plugin.num_natives];

        for (uint32_t i = 0; i < m_plugin.num_natives; i++) {
          m_plugin.natives[i].flags = 0;
          m_plugin.natives[i].pfn = InvalidNative;
          m_plugin.natives[i].status = SP_NATIVE_UNBOUND;
          m_plugin.natives[i].user = NULL;
          m_plugin.natives[i].name = m_plugin.stringbase + natives[i].name;
        }
      }
    } else if (!(m_plugin.debug.files) && !strcmp(nameptr, ".dbg.files")) {
      m_plugin.debug.files = (sp_fdbg_file_t *)(base + secptr->dataoffs);
    } else if (!(m_plugin.debug.lines) && !strcmp(nameptr, ".dbg.lines")) {
      m_plugin.debug.lines = (sp_fdbg_line_t *)(base + secptr->dataoffs);
    } else if (!(m_plugin.debug.symbols) && !strcmp(nameptr, ".dbg.symbols")) {
      m_plugin.debug.symbols = (sp_fdbg_symbol_t *)(base + secptr->dataoffs);
    } else if (!(m_plugin.debug.lines_num) && !strcmp(nameptr, ".dbg.info")) {
      sp_fdbg_info_t *inf = (sp_fdbg_info_t *)(base + secptr->dataoffs);
      m_plugin.debug.files_num = inf->num_files;
      m_plugin.debug.lines_num = inf->num_lines;
      m_plugin.debug.syms_num = inf->num_syms;
    } else if (!(m_plugin.debug.stringbase) && !strcmp(nameptr, ".dbg.strings")) {
      m_plugin.debug.stringbase = (const char *)(base + secptr->dataoffs);
    } else if (strcmp(nameptr, ".dbg.natives") == 0) {
      m_plugin.debug.unpacked = false;
    }

    secptr++;
    sectnum++;
  }

  if (m_plugin.pcode == NULL || m_plugin.data == NULL)
    return SP_ERROR_FILE_FORMAT;

  if ((m_plugin.flags & SP_FLAG_DEBUG) && (
    !(m_plugin.debug.files) || 
    !(m_plugin.debug.lines) || 
    !(m_plugin.debug.symbols) || 
    !(m_plugin.debug.stringbase) ))
  {
    return SP_ERROR_FILE_FORMAT;
  }

  if (m_plugin.num_publics > 0) {
    m_PubFuncs = new CFunction *[m_plugin.num_publics];
    memset(m_PubFuncs, 0, sizeof(CFunction *) * m_plugin.num_publics);
    m_PubJitFuncs = new JitFunction *[m_plugin.num_publics];
    memset(m_PubJitFuncs, 0, sizeof(JitFunction *) * m_plugin.num_publics);
  }

  MD5 md5_pcode;
  md5_pcode.update(m_plugin.pcode, m_plugin.pcode_size);
  md5_pcode.finalize();
  md5_pcode.raw_digest(m_CodeHash);
  
  MD5 md5_data;
  md5_data.update(m_plugin.data, m_plugin.data_size);
  md5_data.finalize();
  md5_data.raw_digest(m_DataHash);

  m_pCtx = new BaseContext(this);
  co_ = g_Jit.StartCompilation(this);

  SetupFloatNativeRemapping();
  function_map_size_ = m_plugin.pcode_size / sizeof(cell_t) + 1;
  function_map_ = new JitFunction *[function_map_size_];
  memset(function_map_, 0, function_map_size_ * sizeof(JitFunction *));

  return SP_ERROR_NONE;
}

void
BaseRuntime::AddJittedFunction(JitFunction *fn)
{
  m_JitFunctions.append(fn);

  cell_t pcode_offset = fn->GetPCodeAddress();
  assert(pcode_offset % 4 == 0);

  uint32_t pcode_index = pcode_offset / 4;
  assert(pcode_index < function_map_size_);

  function_map_[pcode_index] = fn;
}

JitFunction *
BaseRuntime::GetJittedFunctionByOffset(cell_t pcode_offset)
{
  assert(pcode_offset % 4 == 0);

  uint32_t pcode_index = pcode_offset / 4;
  assert(pcode_index < function_map_size_);

  return function_map_[pcode_index];
}

int
BaseRuntime::FindNativeByName(const char *name, uint32_t *index)
{
  for (uint32_t i=0; i<m_plugin.num_natives; i++) {
    if (strcmp(m_plugin.natives[i].name, name) == 0) {
      if (index)
        *index = i;
      return SP_ERROR_NONE;
    }
  }

  return SP_ERROR_NOT_FOUND;
}

int
BaseRuntime::GetNativeByIndex(uint32_t index, sp_native_t **native)
{
  if (index >= m_plugin.num_natives)
    return SP_ERROR_INDEX;

  if (native)
    *native = &(m_plugin.natives[index]);

  return SP_ERROR_NONE;
}

sp_native_t *
BaseRuntime::GetNativeByIndex(uint32_t index)
{
  assert(index < m_plugin.num_natives);
  return &m_plugin.natives[index];
}

uint32_t
BaseRuntime::GetNativesNum()
{
  return m_plugin.num_natives;
}

int
BaseRuntime::FindPublicByName(const char *name, uint32_t *index)
{
  int diff, high, low;
  uint32_t mid;

  high = m_plugin.num_publics - 1;
  low = 0;

  while (low <= high) {
    mid = (low + high) / 2;
    diff = strcmp(m_plugin.publics[mid].name, name);
    if (diff == 0) {
      if (index)
        *index = mid;
      return SP_ERROR_NONE;
    } else if (diff < 0) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return SP_ERROR_NOT_FOUND;
}

int
BaseRuntime::GetPublicByIndex(uint32_t index, sp_public_t **pblic)
{
  if (index >= m_plugin.num_publics)
    return SP_ERROR_INDEX;

  if (pblic)
    *pblic = &(m_plugin.publics[index]);

  return SP_ERROR_NONE;
}

uint32_t
BaseRuntime::GetPublicsNum()
{
  return m_plugin.num_publics;
}

int
BaseRuntime::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
{
  if (index >= m_plugin.num_pubvars)
    return SP_ERROR_INDEX;

  if (pubvar)
    *pubvar = &(m_plugin.pubvars[index]);

  return SP_ERROR_NONE;
}

int
BaseRuntime::FindPubvarByName(const char *name, uint32_t *index)
{
  int diff, high, low;
  uint32_t mid;

  high = m_plugin.num_pubvars - 1;
  low = 0;

  while (low <= high) {
    mid = (low + high) / 2;
    diff = strcmp(m_plugin.pubvars[mid].name, name);
    if (diff == 0) {
      if (index)
        *index = mid;
      return SP_ERROR_NONE;
    } else if (diff < 0) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return SP_ERROR_NOT_FOUND;
}

int
BaseRuntime::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
  if (index >= m_plugin.num_pubvars)
    return SP_ERROR_INDEX;

  *local_addr = (uint8_t *)m_plugin.pubvars[index].offs - m_plugin.memory;
  *phys_addr = m_plugin.pubvars[index].offs;

  return SP_ERROR_NONE;
}

uint32_t
BaseRuntime::GetPubVarsNum()
{
  return m_plugin.num_pubvars;
}

IPluginContext *
BaseRuntime::GetDefaultContext()
{
  return m_pCtx;
}

IPluginDebugInfo *
BaseRuntime::GetDebugInfo()
{
  return &m_Debug;
}

IPluginFunction *
BaseRuntime::GetFunctionById(funcid_t func_id)
{
  CFunction *pFunc = NULL;

  if (func_id & 1) {
    func_id >>= 1;
    if (func_id >= m_plugin.num_publics)
      return NULL;
    pFunc = m_PubFuncs[func_id];
    if (!pFunc) {
      m_PubFuncs[func_id] = new CFunction(this, (func_id << 1) | 1, func_id);
      pFunc = m_PubFuncs[func_id];
    }
  }

  return pFunc;
}

CFunction *
BaseRuntime::GetPublicFunction(size_t index)
{
  CFunction *pFunc = m_PubFuncs[index];
  if (!pFunc) {
    sp_public_t *pub = NULL;
    GetPublicByIndex(index, &pub);
    if (pub)
      m_PubFuncs[index] = new CFunction(this, (index << 1) | 1, index);
    pFunc = m_PubFuncs[index];
  }

  return pFunc;
}

IPluginFunction *
BaseRuntime::GetFunctionByName(const char *public_name)
{
  uint32_t index;

  if (FindPublicByName(public_name, &index) != SP_ERROR_NONE)
    return NULL;

  return GetPublicFunction(index);
}

bool BaseRuntime::IsDebugging()
{
  return true;
}

void BaseRuntime::SetPauseState(bool paused)
{
  if (paused)
  {
    m_plugin.run_flags |= SPFLAG_PLUGIN_PAUSED;
  }
  else
  {
    m_plugin.run_flags &= ~SPFLAG_PLUGIN_PAUSED;
  }
}

bool BaseRuntime::IsPaused()
{
  return ((m_plugin.run_flags & SPFLAG_PLUGIN_PAUSED) == SPFLAG_PLUGIN_PAUSED);
}

size_t BaseRuntime::GetMemUsage()
{
  size_t mem = 0;

  mem += sizeof(this);
  mem += sizeof(sp_plugin_t);
  mem += sizeof(BaseContext);
  mem += m_plugin.base_size;

  return mem;
}

unsigned char *BaseRuntime::GetCodeHash()
{
  return m_CodeHash;
}

unsigned char *BaseRuntime::GetDataHash()
{
  return m_DataHash;
}

BaseContext *BaseRuntime::GetBaseContext()
{
  return m_pCtx;
}

int
BaseRuntime::ApplyCompilationOptions(ICompilation *co)
{
  if (co == NULL)
    return SP_ERROR_NONE;

  co_ = g_Jit.ApplyOptions(co_, co);
  m_plugin.prof_flags = ((CompData *)co_)->profile;

  return SP_ERROR_NONE;
}

int
BaseRuntime::CreateBlank(uint32_t heastk)
{
  memset(&m_plugin, 0, sizeof(m_plugin));

  /* Align to cell_t bytes */
  heastk += sizeof(cell_t);
  heastk -= heastk % sizeof(cell_t);

  m_plugin.mem_size = heastk;
  m_plugin.memory = new uint8_t[heastk];

  m_pCtx = new BaseContext(this);
  co_ = g_Jit.StartCompilation(this);

  return SP_ERROR_NONE;
}

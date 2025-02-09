/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter_internal.h                                *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef M64P_DEVICE_R4300_VR4300_JITTER_INTERNAL_H
#define M64P_DEVICE_R4300_VR4300_JITTER_INTERNAL_H

#include "Common/x64Emitter.h"
#include "JitCache.h"
#include "Common/JitRegister.h"
#include "rangeset/include/rangeset/rangesizeset.h"

#include "RegCache/GPRRegCache.h"
#include "RegCache/FPURegCache.h"

#include "vr4300_jitter_instruction_decoder.h"

#include <ucontext.h>
typedef mcontext_t SContext;

#include "ConstantPool.h"

struct BackPatchInfo {
    int read;
    uint8_t *code_before;
    const uint8_t *code_after;
    const uint8_t *farcode;
    int fault_count;
};

class VR4300_Jitter;
extern VR4300_Jitter *gJitterInstance;

class VR4300_Jitter : public Gen::X64CodeBlock {
    private:
        bool m_initialized = false;

        const u8 *m_dispatcher_start = NULL;
        const u8 *m_recompiler_start = NULL;
        const u8 *m_jitcache_dispatcher_start = NULL;
        const u8 *m_dispatcher_slow = NULL;
        const u8 *m_dispatcher_checkstop = NULL;
        const u8 *m_dispatcher_mispredicted_ret_1 = NULL;
        const u8 *m_dispatcher_mispredicted_ret_2 = NULL;
        const u8 *m_dispatcher_mispredicted_ret_3 = NULL;
        const u8 *m_dispatcher_exit = NULL;
        const u8 *m_core_compare = NULL;

        u8 *m_free_code_start = NULL;
        const u8 *m_enter_code = NULL;

        Common::MemArena m_arena;
        u8 *m_fastmem_arena = nullptr;
        uint64_t m_fastmem_arena_size = 0;

        void *m_rdram_ptr = nullptr;
        void *m_rspmem_ptr = nullptr;
        void *m_dd_ptr = nullptr;
        void *m_cart_rom_ptr = nullptr;
        void *m_pif_ptr = nullptr;

        uint64_t m_rdram_position = 0;
        uint64_t m_rspmem_position = 0;
        uint64_t m_dd_position = 0;
        uint64_t m_cart_rom_position = 0;
        uint64_t m_pif_position = 0;

        Gen::X64CodeBlock m_far_code;
        ConstantPool m_const_pool;

        std::unordered_map<u8*, BackPatchInfo> m_back_patch_info;

        GPRRegCache m_gpr;
        FPURegCache m_fpr;

        bool m_using_tlb = false;

        HyoutaUtilities::RangeSizeSet<uint8_t*> m_free_ranges_near;
        HyoutaUtilities::RangeSizeSet<uint8_t*> m_free_ranges_far;

        std::vector<std::pair<u8*, u8*>> m_ranges_to_free_on_next_codegen_near;
        std::vector<std::pair<u8*, u8*>> m_ranges_to_free_on_next_codegen_far;

        u8 *m_physical_base = nullptr;
        JitCache m_block_cache;
        static constexpr u64 VALID_BLOCK_ARENA_SIZE = 0x1'0000'0000;
        Common::LazyMemoryRegion m_valid_block_arena;
        u8 *m_valid_block_ptr = nullptr;

        u8 *m_near_code = nullptr;
        u8 *m_near_code_end = nullptr;
        bool m_near_code_write_failed = false;

        bool m_in_far_code = false;

        struct r4300_core *m_r4300 = &g_dev.r4300;

        u32 m_tlb_mappings[0x100000];
        u8 m_valid_virtual_block[0x100000];

public:
        static inline VR4300_Jitter *GetInstance() {
            return gJitterInstance;
        }

        inline struct r4300_core *GetR4300Core() {
            return m_r4300;
        }

        inline JitCache *GetBlockCache() {
            return &m_block_cache;
        }

        inline const void *GetEnterCode() {
            return m_enter_code;
        }

        inline const u8 *GetDispatcherStart() {
            return m_dispatcher_start;
        }

        inline const u8 *GetRecompilerStart() {
            return m_recompiler_start;
        }

        inline const u8 *GetJitCacheDispatcherStart() {
            return m_jitcache_dispatcher_start;
        }

        Common::MemArena *GetFastmemArena() {
            return &m_arena;
        }

        inline u8 *GetPhysicalBase() {
            return m_physical_base;
        }

        inline uint64_t GetRdramPosition() {
            return m_rdram_position;
        }

        bool AnalyzeInstruction(jit_instr *instr, uint32_t instruction, unsigned int instr_address);
        unsigned int Analyze(unsigned int addr, struct prepared_code_block *code_block, int max_instr);

        void InvalidateCachedCodeJustErase(uint32_t address, size_t length);
        void InvalidateCachedCode(uint32_t address, size_t length);

        void Init();
        void Cleanup();

        bool HandleFault(uintptr_t access_address, SContext* ctx);

        void *InitializeFastmem();

        uint8_t *GetLogicalMemory(int type);

        void *RecompileBlock(unsigned int addr);

        void ValidBlockSet(uint32_t address);
        void ValidBlockUnset(uint32_t address);
        void ValidBlockUnsetRange(uint32_t address, u32 length);
        bool ValidBlockIsSet(uint32_t address);

        void ValidBlockSetVirtual(uint32_t address);
        void ValidBlockUnsetVirtual(uint32_t address);

        bool ValidBlockIsSetVirtual(uint32_t address);
        bool ValidBlockIsSetVirtualIdx(uint32_t address_idx);

        void ReleaseCodeSpace(uint8_t *near_begin, uint8_t *near_end, uint8_t *far_begin, uint8_t *far_end);

        void WriteDestroyBlock(u8 *ptr);
        void TLBWrite(unsigned int idx);

        void FixHotCycles(void);
        void MapCorruptRdram(bool corrupt);

        void ClearCache();

private:
        template <typename T>
        const void* GetConstantFromPool(const T& value)
        {
          return m_const_pool.GetConstant(&value, sizeof(T), 1, 0);
        }

        template <typename T>
        Gen::OpArg MConst(const T& value)
        {
          return Gen::M(GetConstantFromPool(value));
        }

        template <typename T, size_t N>
        Gen::OpArg MConst(const T (&value)[N], size_t index = 0)
        {
          return Gen::M(m_const_pool.GetConstant(&value, sizeof(T), N, index));
        }

        void compile_cycle_count_checks(struct jit_instr *op, uint32_t pc, bool no_compare, bool no_add, u32 compare_pc);
        void compile_tlb_exception_check(struct jit_instr *op, bool update_cycles, uint32_t expected_pc);

        // Set the emitter state to far/near code
        void switch_to_near_code();
        void switch_to_far_code();
        bool is_in_far_code();

        // Jump to far/near code and set the emitter state to far/near code
        void enter_farcode();
        void leave_farcode();

        u32 get_address_of_nth_instruction_after(struct jit_instr *op, int n);

        // Move 2 arguments to 2 locations, using shortcuts if possible
        void mov_2(int bits1, const Gen::X64Reg &dest1, int bits2, const Gen::X64Reg &dest2, const RCOpArg &source1, const RCOpArg &source2);
        // Similar as above but for 3 arguments
        void mov_3(int bits, const Gen::X64Reg &dest1, const Gen::X64Reg &dest2, const Gen::X64Reg &dest3, const RCOpArg &source1, const RCOpArg &source2, const RCOpArg &source3);
        BitSet32 caller_saved_registers_in_use();
        void emit_core_compare();

        void generate_asm();
        void set_instruction_stats(struct jit_instr *instr);
        void set_cp0_register_modification_stats(struct prepared_code_block *code_block, int i);
        void get_source_and_pagelimit(uint32_t addr, uint32_t **source, unsigned int *pagelimit, unsigned int *hard_pagelimit);

        void zero_hot_cycles(void);
        void update_hot_cycles(struct jit_instr *op, bool store_pc = true);

        void do_core_compare(struct jit_instr *op, u32 pc);
        void update_count_reg(void);

        void core_compare_copy_registers_to_tmp_now();
        void core_compare_copy_registers_to_tmp();

        void save_discarded_registers_for_core_compare(struct jit_instr *op);
        void save_discarded_registers_for_core_compare_float(struct jit_instr *op);

        void update_cycle_count(struct jit_instr *op, bool no_add = false);

        void compile_invalidate_code_constaddress(uint32_t address, uint32_t length = 4);
        void compile_invalidate_code(const RCOpArg &addr, uint32_t addr_offset, const RCX64Reg &scratch);
        void compile_goto_dispatcher_destinhotstate_ret();
        void compile_goto_dispatcher_ret(struct jit_instr *op, u32 address);
        void compile_goto_dispatcher_destinhotstate(struct jit_instr *op, bool call, bool checkstop = false, bool flush = true, u32 after = 0);
        void compile_goto_dispatcher(struct jit_instr *op, u32 address, bool call, bool flush = true);

        Gen::FixupBranch check_pc_differs(uint32_t expected_pc);
        Gen::FixupBranch check_pc_equals(uint32_t expected_pc);

        void compile_INTERPRETER_FALLBACK(struct jit_instr *op, u32 address);

        bool is_linking_branch(struct jit_instr *op);
        bool is_nop(struct jit_instr *op);
        bool is_likely_branch(struct jit_instr *op);
        bool is_jump(struct jit_instr *op);
        bool will_branch(struct jit_instr *op);

        void recompile_delay_slot(struct jit_instr *op, bool skip_instruction);
        void load_cop1_register_to_host_register(struct jit_instr *op, int bits, const RCOpArg &Rt, const RCX64Reg &target);
        void load_cop1_register_to_host_register(struct jit_instr *op, int bits, int reg, const RCX64Reg &target);
        void store_host_register_to_cop1_register(struct jit_instr *op, int bits, const RCOpArg &cpu_val, const RCX64Reg &Rt);
        void store_host_register_to_cop1_register(struct jit_instr *op, int bits, const RCOpArg &cpu_val, int reg);

        void recompile_LW(struct jit_instr *op, bool unsigned_lw = false);
        void recompile_LWU(struct jit_instr *op);
        void recompile_LL(struct jit_instr *op);
        void recompile_LDL(struct jit_instr *op);
        void recompile_LWL(struct jit_instr *op);
        void recompile_LWR(struct jit_instr *op);
        void recompile_LDR(struct jit_instr *op);
        void recompile_LH(struct jit_instr *op, bool unsigned_lh = false);
        void recompile_LHU(struct jit_instr *op);
        void recompile_LD(struct jit_instr *op);
        void recompile_LB(struct jit_instr *op, bool unsigned_lb = false);
        void recompile_LBU(struct jit_instr *op);
        void recompile_SW(struct jit_instr *op);
        void recompile_SWL(struct jit_instr *op);
        void recompile_SDR(struct jit_instr *op);
        void recompile_SWR(struct jit_instr *op);
        void recompile_SB(struct jit_instr *op);
        void recompile_SD(struct jit_instr *op);
        void recompile_SC(struct jit_instr *op);
        void recompile_SDL(struct jit_instr *op);
        void recompile_SH(struct jit_instr *op);

        void recompile_TLBWI(struct jit_instr *op);
        void recompile_TLBWR(struct jit_instr *op);
        void recompile_TLBP(struct jit_instr *op);
        void recompile_TLBR(struct jit_instr *op);

        void recompile_MFC0(struct jit_instr *op);
        void recompile_DMFC0(struct jit_instr *op);

        void recompile_MTC0(struct jit_instr *op);
        void recompile_DMTC0(struct jit_instr *op);

        void recompile_MTC1(struct jit_instr *op);
        void recompile_DMTC1(struct jit_instr *op);
        void recompile_MFC1(struct jit_instr *op);
        void recompile_DMFC1(struct jit_instr *op);
        void recompile_CTC1(struct jit_instr *op);
        void recompile_CFC1(struct jit_instr *op);
        void recompile_DCFC1(struct jit_instr *op);
        void recompile_DCTC1(struct jit_instr *op);

        void recompile_SYNC(struct jit_instr *op);

        void recompile_CTC2(struct jit_instr *op);
        void recompile_MTC2(struct jit_instr *op);

        void recompile_DMTC2(struct jit_instr *op);
        void recompile_CFC2(struct jit_instr *op);
        void recompile_MFC2(struct jit_instr *op);
        void recompile_DMFC2(struct jit_instr *op);

        void recompile_SYSCALL(struct jit_instr *op);
        void recompile_ERET(struct jit_instr *op);
        void recompile_BREAK(struct jit_instr *op);

        void recompile_TRUNC_W_S(struct jit_instr *op);
        void recompile_FLOOR_W_S(struct jit_instr *op);
        void recompile_FLOOR_W_D(struct jit_instr *op);
        void recompile_FLOOR_L_D(struct jit_instr *op);
        void recompile_FLOOR_L_S(struct jit_instr *op);
        void recompile_TRUNC_W_D(struct jit_instr *op);
        void recompile_TRUNC_L_S(struct jit_instr *op);
        void recompile_TRUNC_L_D(struct jit_instr *op);
        void recompile_CVT_S_W(struct jit_instr *op);
        void recompile_CVT_S_D(struct jit_instr *op);
        void recompile_CVT_W_D(struct jit_instr *op);
        void recompile_CVT_W_S(struct jit_instr *op);
        void recompile_CVT_D_S(struct jit_instr *op);
        void recompile_CVT_D_L(struct jit_instr *op);
        void recompile_CVT_D_W(struct jit_instr *op);
        void recompile_CVT_L_S(struct jit_instr *op);
        void recompile_CVT_L_D(struct jit_instr *op);
        void recompile_CVT_S_L(struct jit_instr *op);
        void recompile_C_cond_fmt(struct jit_instr *op, int fmt);
        void recompile_C_cond_S(struct jit_instr *op);
        void recompile_C_cond_D(struct jit_instr *op);
        void recompile_NEG_S(struct jit_instr *op);
        void recompile_NEG_D(struct jit_instr *op);
        void recompile_ROUND_W_S(struct jit_instr *op);
        void recompile_ROUND_L_D(struct jit_instr *op);
        void recompile_ROUND_L_S(struct jit_instr *op);
        void recompile_ROUND_W_D(struct jit_instr *op);
        void recompile_CEIL_W_D(struct jit_instr *op);
        void recompile_CEIL_W_S(struct jit_instr *op);
        void recompile_CEIL_L_D(struct jit_instr *op);
        void recompile_CEIL_L_S(struct jit_instr *op);
        void recompile_MUL_S(struct jit_instr *op);
        void recompile_MUL_D(struct jit_instr *op);
        void recompile_DIV_S(struct jit_instr *op);
        void recompile_DIV_D(struct jit_instr *op);
        void recompile_ADD_S(struct jit_instr *op);
        void recompile_ABS_S(struct jit_instr *op);
        void recompile_ABS_D(struct jit_instr *op);
        void recompile_SUB_S(struct jit_instr *op);
        void recompile_SUB_D(struct jit_instr *op);
        void recompile_SQRT_S(struct jit_instr *op);
        void recompile_SQRT_D(struct jit_instr *op);
        void recompile_MOV_S(struct jit_instr *op);
        void recompile_MOV_D(struct jit_instr *op);
        void recompile_ADD_D(struct jit_instr *op);
        void recompile_SWC1(struct jit_instr *op);
        void recompile_LWC1(struct jit_instr *op);
        void recompile_LDC1(struct jit_instr *op);
        void recompile_SDC1(struct jit_instr *op);

        void recompile_ADD(struct jit_instr *op);
        void recompile_DADD(struct jit_instr *op);
        void recompile_DADDU(struct jit_instr *op);
        void recompile_OR(struct jit_instr *op);
        void recompile_NOR(struct jit_instr *op);
        void recompile_AND(struct jit_instr *op);
        void recompile_DADDI(struct jit_instr *op);
        void recompile_DADDIU(struct jit_instr *op);
        void recompile_ADDI(struct jit_instr *op);
        void recompile_ADDU(struct jit_instr *op);
        void recompile_SUB(struct jit_instr *op);
        void recompile_DSUB(struct jit_instr *op);
        void recompile_DSUBU(struct jit_instr *op);
        void recompile_SUBU(struct jit_instr *op);
        void recompile_ORI(struct jit_instr *op);
        void recompile_ADDIU(struct jit_instr *op);
        void recompile_ANDI(struct jit_instr *op);
        void recompile_XOR(struct jit_instr *op);
        void recompile_XORI(struct jit_instr *op);
        void recompile_LUI(struct jit_instr *op);
        void recompile_SLTI(struct jit_instr *op);
        void recompile_CACHE(struct jit_instr *op);
        void recompile_SLTIU(struct jit_instr *op);
        void recompile_SLT(struct jit_instr *op);
        void recompile_SLTU(struct jit_instr *op);
        void recompile_SLL(struct jit_instr *op);
        void recompile_SRL(struct jit_instr *op);
        void recompile_SRA(struct jit_instr *op);
        void recompile_SRLV(struct jit_instr *op);
        void recompile_SRAV(struct jit_instr *op);
        void recompile_DSRAV(struct jit_instr *op);
        void recompile_SLLV(struct jit_instr *op);
        void recompile_DSLL32(struct jit_instr *op);
        void recompile_DSLL(struct jit_instr *op);
        void recompile_DSRL(struct jit_instr *op);
        void recompile_DSRLV(struct jit_instr *op);
        void recompile_DSLLV(struct jit_instr *op);
        void recompile_DSRL32(struct jit_instr *op);
        void recompile_DSRA32(struct jit_instr *op);
        void recompile_DSRA(struct jit_instr *op);
        void recompile_MULT(struct jit_instr *op, bool unsigned_multiply = false);
        void recompile_MULTU(struct jit_instr *op);
        void recompile_DMULTU(struct jit_instr *op);
        void recompile_DMULT(struct jit_instr *op);
        void recompile_DDIV(struct jit_instr *op, bool unsigned_div = false);
        void recompile_DDIVU(struct jit_instr *op);
        void recompile_DIV(struct jit_instr *op, bool un_signed = false);
        void recompile_DIVU(struct jit_instr *op);
        void recompile_MFLO(struct jit_instr *op);
        void recompile_MFHI(struct jit_instr *op);
        void recompile_MTLO(struct jit_instr *op);
        void recompile_MTHI(struct jit_instr *op);

        void recompile_BNE(struct jit_instr *op, bool likely = false);
        void recompile_BNEL(struct jit_instr *op);
        void recompile_BEQ(struct jit_instr *op, bool likely = false);
        void recompile_BEQL(struct jit_instr *op);
        void recompile_BC1T(struct jit_instr *op, bool likely = false);
        void recompile_BC1TL(struct jit_instr *op);
        void recompile_BC1F(struct jit_instr *op, bool likely = false);
        void recompile_BC1FL(struct jit_instr *op);
        void recompile_BLEZ(struct jit_instr *op, bool likely = false);
        void recompile_BLEZL(struct jit_instr *op);
        void recompile_BGTZ(struct jit_instr *op, bool likely = false);
        void recompile_BGTZL(struct jit_instr *op);
        void recompile_JALR(struct jit_instr *op);
        void recompile_JAL(struct jit_instr *op);
        void recompile_JR(struct jit_instr *op);
        void recompile_J(struct jit_instr *op);
        void recompile_BGEZ(struct jit_instr *op, bool likely = false, bool link = false);
        void recompile_BGEZAL(struct jit_instr *op);
        void recompile_BGEZALL(struct jit_instr *op);
        void recompile_BGEZL(struct jit_instr *op);
        void recompile_BLTZ(struct jit_instr *op, bool likely = false, bool link = false);
        void recompile_BLTZALL(struct jit_instr *op);
        void recompile_BLTZAL(struct jit_instr *op);
        void recompile_BLTZL(struct jit_instr *op);

        void recompile_TGE(struct jit_instr *op);
        void recompile_TGEU(struct jit_instr *op);
        void recompile_TGEI(struct jit_instr *op);
        void recompile_TGEIU(struct jit_instr *op);
        void recompile_TLT(struct jit_instr *op);
        void recompile_TLTU(struct jit_instr *op);
        void recompile_TLTI(struct jit_instr *op);
        void recompile_TLTIU(struct jit_instr *op);
        void recompile_TEQ(struct jit_instr *op);
        void recompile_TEQI(struct jit_instr *op);
        void recompile_TNE(struct jit_instr *op);
        void recompile_TNEI(struct jit_instr *op);

        void recompile_RESERVED_COP2(struct jit_instr *op);
        void recompile_RESERVED(struct jit_instr *op);

        void div_core(struct jit_instr *op, const RCOpArg &edx, const RCOpArg &eax, const RCOpArg &Rs, const RCOpArg &Rt, bool un_signed);

        void compile_fpu_reset_cause(struct jit_instr *op);
        void compile_fpu_reset_exceptions(struct jit_instr *op);
        void compile_fpu_check_exceptions(struct jit_instr *op);
        void compile_fpu_check_input_float(struct jit_instr *op, const RCX64Reg &reg);
        void compile_fpu_check_input_double(struct jit_instr *op, const RCX64Reg &reg);
        void compile_fpu_check_input_float(struct jit_instr *op, const RCOpArg &input);
        void compile_fpu_check_input_double(struct jit_instr *op, const RCOpArg &input);
        void compile_fpu_check_output_float(struct jit_instr *op);
        void compile_fpu_check_output_double(struct jit_instr *op);
        void compile_fpu_store_output_float_for_check(struct jit_instr *op, const RCX64Reg &reg);
        void compile_fpu_store_output_double_for_check(struct jit_instr *op, const RCX64Reg &reg);

        void recompile_instruction(struct jit_instr *op);
        void embed_valid_block_check(u32 address, bool force_check, bool delay_slot = false, bool update_cc = false);
        void embed_valid_block_check_pc();

        void clear_ranges_to_free();

        void mtc0_helper(int t, int d, unsigned int mask);

        void update_random_reg();

        void compile_exception_general(struct jit_instr *op);
        void compile_cop1_usable_check(struct jit_instr *op);
        void compile_cop2_usable_check(struct jit_instr *op);

        void compile_check_cycle_count(struct jit_instr *op, uint32_t in_pc);

        void install_exception_handler(void);

        bool is_address_in_fastmem_arena(const u8* address);
};

#endif


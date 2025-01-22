// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bitset>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common/CommonTypes.h"
#include "Common/MemArena.h"
#include "vr4300_jitter.h"

struct JitBlock {
    u8 *host_entry;

    u8 *code_begin;
    u8 *code_end;
    u8 *farcode_begin;
    u8 *farcode_end;

    uint32_t physical_address;
    uint32_t virtual_address;
    bool fr_is_set;

    struct LinkData
    {
        u8* exitPtrs;  // to be able to rewrite the exit jump
        u32 exitAddress;
        u32 exitAddressPhysical;
        bool linkStatus;  // is it already linked?
        bool call;
    };

    std::vector<LinkData> linkData;
    std::set<uint32_t> virtual_addresses;
    std::set<uint32_t> physical_addresses;
    bool OverlapsPhysicalRange(u32 address, u32 length) const;
};

class JitCache {
private:
#if !HUGE_MAP_FOR_ENTRY_POINTS
    // (fr_is_set + virtual / 4-byte alignment) * sizeof(void*)
    static constexpr u64 FAST_BLOCK_MAP_SIZE = 0x4'0000'0000;
#else
    // ((fr_is_set + virtual + (physical & 0x7FF000)) / 4-byte alignment) * sizeof(void*)
    static constexpr u64 FAST_BLOCK_MAP_SIZE = 0x4000'0000'0000;
#endif
    Common::LazyMemoryRegion m_entry_points_arena;
    u8** m_entry_points_ptr = 0;
#if !HUGE_MAP_FOR_ENTRY_POINTS
    u32
#else
    u64
#endif
    AddressToLookupIndex(u32 virtual_address, u32 physical_address, bool fr_is_set);

    std::multimap<u32, JitBlock> m_block_map;

#if !HUGE_MAP_FOR_ENTRY_POINTS
    std::unordered_map<u32, std::unordered_set<JitBlock*>> m_links_to;
#else
    std::unordered_map<u64, std::unordered_set<JitBlock*>> m_links_to;
#endif
    static constexpr u32 BLOCK_RANGE_MAP_ELEMENTS = 0x100;
    std::map<u32, std::unordered_set<JitBlock*>> m_block_range_map;
    // This is indexed by the virtual block index (addr >> 12).
    std::map<u32, std::unordered_set<JitBlock*>> m_block_range_map_virtual;

    void LinkBlockExits(JitBlock &block);

    JitBlock *GetBlockFromStartAddress(u32 virtual_address, u32 physical_address, bool fr_is_set);
    void UnlinkBlock(JitBlock& block);
    void WriteLinkBlock(const JitBlock::LinkData& source, const JitBlock* dest);

public:
    u8 **GetEntryPoints() { return m_entry_points_ptr; }
    JitBlock *AllocateBlock(uint32_t virtual_address, uint32_t physical_address);
    void FinalizeBlock(JitBlock &block, const std::set<u32>& virtual_addresses, const std::set<u32>& physical_addresses);
    const void *Dispatch(uint32_t address);
    void Clear();
    void Init();

    void ErasePhysicalRange(u32 address, u32 length);
    void EraseVirtualRange(u32 virtual_address, u32 physical_address, u32 length);
    void EraseSingleBlock(const JitBlock& block);


    void DestroyBlock(JitBlock &block);
    void LinkBlock(JitBlock &block);
};


// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "JitCache.h"
#include "main/main.h"
#include "api/callbacks.h"

#include "vr4300_jitter_internal.h"

JitBlock *JitCache::GetBlockFromStartAddress(u32 virtual_address, u32 physical_address, bool fr_is_set)
{
    auto iter = m_block_map.equal_range(physical_address);
    for (; iter.first != iter.second; iter.first++) {
        JitBlock& b = iter.first->second;
        if ((b.physical_address == physical_address)
            && (b.virtual_address == virtual_address)
            && (b.fr_is_set == fr_is_set)) {
            return &b;
        }
    }

    return nullptr;
}

void JitCache::LinkBlockExits(JitBlock& block)
{
    for (auto& e : block.linkData)
    {
        if (!e.linkStatus)
        {
            JitBlock* destinationBlock = GetBlockFromStartAddress(e.exitAddress, e.exitAddressPhysical, block.fr_is_set);
            if (destinationBlock)
            {
                WriteLinkBlock(e, destinationBlock);
                e.linkStatus = true;
            }
        }
    }
}

void JitCache::LinkBlock(JitBlock& block)
{
    LinkBlockExits(block);

    const auto it = m_links_to.find(block.physical_address);
    if (it == m_links_to.end())
        return;

    for (JitBlock* b2 : it->second) {
        LinkBlockExits(*b2);
    }
}

JitBlock *JitCache::AllocateBlock(uint32_t virtual_address, uint32_t physical_address)
{
    JitBlock &block = m_block_map.emplace(physical_address, JitBlock())->second;

    block.physical_address = physical_address;
    block.virtual_address = virtual_address;
    block.fr_is_set = HOT_STATE->fr_is_set;

#ifndef NDEBUG
    if (block.linkData.size() > 0) {
        DebugMessage(M64MSG_VERBOSE, "AllocateBlock, nonempty linkData\n");
    }
#endif

    block.linkData.clear();

    return &block;
}

void JitCache::Clear()
{
#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "JitCache::Clear()\n");
#endif
    m_block_map.clear();
    m_block_range_map.clear();
    m_block_range_map_virtual.clear();

    m_entry_points_arena.Clear();

    m_links_to.clear();
}

void JitCache::FinalizeBlock(JitBlock &block, const std::set<u32>& virtual_addresses, const std::set<u32>& physical_addresses)
{
    u32 range_mask = ~(BLOCK_RANGE_MAP_ELEMENTS - 1);

    VR4300_Jitter *jitter = VR4300_Jitter::GetInstance();
    for (u32 addr : physical_addresses)
    {
        jitter->ValidBlockSet(addr);
        m_block_range_map[addr & range_mask].insert(&block);
    }

    for (u32 addr : virtual_addresses)
    {
        // NOTE: We set the virtual addresses to valid too in order
        // to speed up the checks in compile_invalidate_code in the
        // non-immediate case.
        // NOTE: We can only unset them when we are sure there are no
        // other blocks using the same virtual addresses (e.g.
        // EraseVirtualRange)!
        jitter->ValidBlockSet(addr);
        m_block_range_map_virtual[addr >> 12].insert(&block);
    }

    block.physical_addresses = physical_addresses;
    block.virtual_addresses = virtual_addresses;

    if (m_entry_points_ptr) {
        m_entry_points_ptr[AddressToLookupIndex(block.virtual_address, block.physical_address, block.fr_is_set)] = block.host_entry;
    }

    for (const auto& e : block.linkData) {
        m_links_to[e.exitAddressPhysical].insert(&block);
    }

    LinkBlock(block);

#ifndef NDEBUG
    fprintf(stderr, "FIN BLOCK: host_entry: %x %p\n", block.virtual_address, block.host_entry);
#endif
}

extern void vr4300_jitter_core_compare();
extern void vr4300_jitter_core_compare_copy_registers_to_tmp_now(void);

const void *JitCache::Dispatch(uint32_t address)
{
    VR4300_Jitter *jitter = VR4300_Jitter::GetInstance();
    uint32_t physical_address = address;

    if (vr4300_jitter_address_needs_translation(address) && !jitter->ValidBlockIsSetVirtual(address)) {
        if (HOT_STATE->delay_slot) {
            TLB_refill_exception(jitter->GetR4300Core(), address, 2);
            address = HOT_STATE->pc;
            vr4300_jitter_core_compare_copy_registers_to_tmp_now();
            vr4300_jitter_core_compare();

            HOT_STATE->delay_slot = 0;
        }
    }

    if (vr4300_jitter_address_needs_translation(address)) {
        HOT_STATE->pc = address;
#if COMPARE_CORE
        HOT_STATE->dbg_pc = address;
#endif

        if (vr4300_jitter_translate_pc_no_exception(address) == (u32)-1) {
            if (!HOT_STATE->delay_slot) {
                vr4300_jitter_core_compare_copy_registers_to_tmp_now();
                vr4300_jitter_core_compare();
            }
        }

        physical_address = vr4300_jitter_translate_address(address, 2) & MEMORY_MASK;

        if (HOT_STATE->pc != address) {
#if COMPARE_CORE
            HOT_STATE->dbg_pc = HOT_STATE->pc;
#endif
            HOT_STATE->pc_changed = 1;
            address = HOT_STATE->pc;
            if (vr4300_jitter_address_needs_translation(address)) {
                physical_address = vr4300_jitter_translate_address_no_exception(address, 2) & MEMORY_MASK;
            } else {
                physical_address = address & MEMORY_MASK;
            }
        }
    } else {
        physical_address = address & MEMORY_MASK;
    }

    if (HOT_STATE->last_block_broken && !HOT_STATE->pc_changed) {
        return nullptr;
    }

    JitBlock *block = GetBlockFromStartAddress(address, physical_address, HOT_STATE->fr_is_set);

    if (block) {
        return block->host_entry;
    }

    return NULL;
}

void JitCache::DestroyBlock(JitBlock &block)
{
#ifndef NDEBUG
    fprintf(stderr, "DESTROYING BLOCK: %p %x %x 0x%x\n", &block, block.virtual_address, block.physical_address, block.host_entry);
#endif

    if (m_entry_points_ptr) {
        m_entry_points_ptr[AddressToLookupIndex(block.virtual_address, block.physical_address, block.fr_is_set)] = 0;
    }

    VR4300_Jitter::GetInstance()->ReleaseCodeSpace(block.code_begin, block.code_end, block.farcode_begin, block.farcode_end);

    UnlinkBlock(block);

    // Delete linking addresses
    for (const auto& e : block.linkData)
    {
        auto it = m_links_to.find(e.exitAddressPhysical);
        if (it == m_links_to.end())
            continue;
        it->second.erase(&block);
        if (it->second.empty())
            m_links_to.erase(it);
    }

    block.linkData.clear();

    VR4300_Jitter::GetInstance()->WriteDestroyBlock(block.host_entry);

    block.host_entry = 0;
}

void JitCache::UnlinkBlock(JitBlock& block)
{
    // Unlink all exits of this block.
    for (auto& e : block.linkData)
    {
        WriteLinkBlock(e, nullptr);
    }

    // Unlink all exits of other blocks which points to this block
    const auto it = m_links_to.find(block.physical_address);
    if (it == m_links_to.end())
        return;

    for (JitBlock* sourceBlock : it->second)
    {
        for (auto& e : sourceBlock->linkData)
        {
            if ((e.exitAddress == block.virtual_address && e.exitAddressPhysical == block.physical_address))
            {
                WriteLinkBlock(e, nullptr);
                e.linkStatus = false;
            }
        }
    }
}

bool JitBlock::OverlapsPhysicalRange(u32 address, u32 length) const
{
  return physical_addresses.lower_bound(address) !=
         physical_addresses.lower_bound(address + length);
}

void JitCache::ErasePhysicalRange(u32 address, u32 length)
{
  // Iterate over all macro blocks which overlap the given range.
  u32 range_mask = ~(BLOCK_RANGE_MAP_ELEMENTS - 1);
  auto start = m_block_range_map.lower_bound(address & range_mask);
  auto end = m_block_range_map.lower_bound(address + length);
  while (start != end)
  {
    // Iterate over all blocks in the macro block.
    auto iter = start->second.begin();
    while (iter != start->second.end())
    {
      JitBlock* block = *iter;
      if (block->OverlapsPhysicalRange(address, length))
      {
        // If the block overlaps, also remove all other occupied slots in the other macro blocks.
        // This will leak empty macro blocks, but they may be reused or cleared later on.
        for (u32 addr : block->physical_addresses)
          if ((addr & range_mask) != start->first)
            m_block_range_map[addr & range_mask].erase(block);

        for (u32 addr : block->virtual_addresses)
          m_block_range_map_virtual[addr >> 12].erase(block);

        // And remove the block.
        DestroyBlock(*block);
        auto block_map_iter = m_block_map.equal_range(block->physical_address);
        while (block_map_iter.first != block_map_iter.second)
        {
          if (&block_map_iter.first->second == block)
          {
            m_block_map.erase(block_map_iter.first);
            break;
          }
          block_map_iter.first++;
        }
        iter = start->second.erase(iter);
      }
      else
      {
        iter++;
      }
    }

    // If the macro block is empty, drop it.
    if (start->second.empty())
      start = m_block_range_map.erase(start);
    else
      start++;
  }
}

#if !HUGE_MAP_FOR_ENTRY_POINTS
u32
#else
u64
#endif
JitCache::AddressToLookupIndex(u32 virtual_address, u32 physical_address, bool fr_is_set)
{
    ASSERT((0xFF800000 & physical_address) == 0);

#if !HUGE_MAP_FOR_ENTRY_POINTS
    return (virtual_address >> 2) | ((fr_is_set ? 1 : 0) << 30);
#else
    return ((((u64)((u64)(((fr_is_set ? 1 : 0) << (12 + 11)) | ((u64)physical_address & 0x7FF000))) << 20) | (u64)virtual_address) >> 2);
#endif
}

void JitCache::Init()
{
    m_entry_points_ptr = reinterpret_cast<u8**>(m_entry_points_arena.Create(FAST_BLOCK_MAP_SIZE));
}

void JitCache::EraseVirtualRange(u32 virtual_address, u32 physical_address, u32 length)
{
  VR4300_Jitter *jitter = VR4300_Jitter::GetInstance();
  jitter->ValidBlockUnsetRange(virtual_address, length);

  auto bmvit = m_block_range_map_virtual.find(virtual_address >> 12);

  if (bmvit != m_block_range_map_virtual.end()) {
    for (auto it = bmvit->second.begin(); it != bmvit->second.end(); ) {
      JitBlock *b = *it;
      for (const u32 addr : b->virtual_addresses) {
          if ((addr >> 12) != (virtual_address >> 12)) {
              m_block_range_map_virtual[addr >> 12].erase(b);
          }
      }
      it = bmvit->second.erase(it);
      EraseSingleBlock(*b);
    }

    m_block_range_map_virtual.erase(bmvit);
  }
}

void JitCache::EraseSingleBlock(const JitBlock& block)
{
  const auto equal_range = m_block_map.equal_range(block.physical_address);
  const auto block_map_iter = std::ranges::find(equal_range.first, equal_range.second, &block,
                                                [](const auto& kv) { return &kv.second; });
  if (block_map_iter == equal_range.second) [[unlikely]]
    return;

  JitBlock& mutable_block = block_map_iter->second;

  u32 range_mask = ~(BLOCK_RANGE_MAP_ELEMENTS - 1);
  for (const u32 addr : mutable_block.physical_addresses)
    m_block_range_map[addr & range_mask].erase(&mutable_block);

  DestroyBlock(mutable_block);
  m_block_map.erase(block_map_iter);  // The original JitBlock reference is now dangling.
}


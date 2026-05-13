Status: Active
Source Idea Path: ideas/open/206_prepared_memory_volatility_address_space_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Existing BIR And Prepared Memory Surfaces

# Current Packet

## Just Finished

Completed Step 1, `Audit Existing BIR And Prepared Memory Surfaces`.

BIR source facts:
- `src/backend/bir/bir.hpp` defines `bir::AddressSpace::{Default, Fs, Gs, Tls}` and
  `bir::MemoryAddress` carries `address_space = AddressSpace::Default` plus
  `is_volatile = false`.
- The memory instruction carriers with optional addresses are
  `bir::LoadLocalInst`, `bir::LoadGlobalInst`, `bir::StoreGlobalInst`, and
  `bir::StoreLocalInst`.
- Existing LIR load/store ops do not expose volatility or address space, and
  `rg` found no current assignment to `MemoryAddress::address_space` outside
  the default field initializer. `LirMemcpyOp`/`LirMemsetOp` have
  `is_volatile`, but current BIR intrinsic lowering rejects volatile forms
  instead of producing volatile `MemoryAddress` facts.

Prepared surfaces:
- `src/backend/prealloc/prealloc.hpp` defines `PreparedAddressBaseKind`,
  `PreparedAddress`, `PreparedMemoryAccess`, `PreparedAddressingFunction`, and
  lookup helpers `find_prepared_addressing_function`,
  `find_prepared_memory_access`, `find_prepared_memory_access_by_result_name`,
  and `find_prepared_addressing`.
- `PreparedAddress` currently publishes base kind, frame slot, symbol,
  pointer-value name, byte offset, size, alignment, and base-plus-offset
  usability.
- `PreparedMemoryAccess` currently publishes function name, block label,
  instruction index, optional result/stored value name, and `PreparedAddress`;
  it has no volatility or address-space field/helper.

Construction-site inventory:
- `src/backend/prealloc/stack_layout/coordinator.cpp` has ten
  `PreparedMemoryAccess{...}` builders:
  `build_direct_frame_slot_access(LoadLocalInst)` at line 143,
  `build_direct_frame_slot_access(StoreLocalInst)` at line 195,
  `build_direct_symbol_backed_access(LoadLocalInst)` at line 327,
  `build_direct_symbol_backed_access(StoreLocalInst)` at line 365,
  `build_direct_symbol_backed_access(LoadGlobalInst)` at line 403,
  `build_direct_symbol_backed_access(StoreGlobalInst)` at line 445,
  `build_pointer_indirect_access(LoadLocalInst)` at line 507,
  `build_pointer_indirect_access(StoreLocalInst)` at line 537,
  `build_pointer_indirect_access(LoadGlobalInst)` at line 567, and
  `build_pointer_indirect_access(StoreGlobalInst)` at line 597.
- The shared address builders that need fact threading are
  `build_direct_symbol_backed_address(...)` and
  `build_pointer_indirect_address(...)`; frame-slot builders currently build
  `PreparedAddress` inline.
- `tests/backend/backend_prepare_stack_layout_test.cpp` has two manual
  `prepare::PreparedMemoryAccess{...}` literals in
  `check_prepared_addressing_contract_activation`; these will need field
  updates if the struct receives designated-initializer fields.

Affected tests/helpers:
- `tests/backend/backend_prepare_stack_layout_test.cpp` is the focused test
  target for prepared memory carrier preservation. It already has direct BIR
  fixture helpers for frame-slot, global-symbol, string-constant, and
  pointer-value prepared accesses, plus lookup assertions through
  `find_prepared_memory_access`.
- Existing stack-layout fixtures set `bir::MemoryAddress` manually, so they
  can directly inject `is_volatile = true` and
  `address_space = bir::AddressSpace::{Fs,Gs,Tls}` for preservation tests.
- Frontend/LIR-produced fixture coverage is limited: current LIR load/store
  ops have no volatile/address-space fields, no production path sets
  non-default `MemoryAddress::address_space`, and volatile memcpy/memset are
  rejected before BIR address publication. Use direct BIR fixtures for Step 3
  unless a separate producer contract is added.

## Suggested Next

Execute Step 2 from `plan.md`: add typed volatility and address-space facts to
the shared prepared carrier, then thread them through all ten stack-layout
`PreparedMemoryAccess` builders and the affected designated-initializer tests.

## Watchouts

- Do not implement target-local volatility or address-space handling as a
  substitute for the shared prepared carrier.
- Do not parse printed BIR, rendered names, debug dumps, or testcase-specific
  strings to recover memory semantics.
- Keep target MIR memory lowering, assembler/object/linker work, atomics,
  inline asm, and alias analysis out of this packet.
- Preserve existing `PreparedAddress` identity, base, offset, size, alignment,
  and `can_use_base_plus_offset` behavior while adding the missing facts.
- Direct BIR fixtures can prove preservation; do not claim frontend/LIR
  production of volatile or non-default address-space accesses from this audit.

## Proof

Command: `git diff --check -- todo.md`

Result: pass.

Proof log: none required for this docs-only audit packet; no `test_after.log`
produced.

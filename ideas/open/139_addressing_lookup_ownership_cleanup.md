# 139 Addressing lookup ownership cleanup

## Goal

Move memory-access, address-materialization, and frame-address-offset lookup
ownership toward the addressing/frame domain instead of the generic prepared
lookup facade.

## Why This Exists

The Step 3 classification in the BIR/prealloc prepared query surface audit
found that addressing lookup facts are reusable target-neutral semantics, but
their public owner is mismatched. `addressing.hpp` already owns direct prepared
address and memory-access records, while `prepared_lookups.*` owns the indexed
caches and frame-address-offset helpers.

Concrete query groups:

- `PreparedAddressMaterializationLookups`
- `PreparedMemoryAccessLookups`
- `make_prepared_address_materialization_lookups`
- `make_prepared_memory_access_lookups`
- `find_indexed_prepared_memory_access*`
- `collect_prepared_address_materializations_for_block`
- `find_indexed_prepared_frame_address_offset_for_value`
- `find_indexed_prepared_frame_address_offset_for_value_id`
- `find_prepared_global_load_access`
- `find_prepared_same_block_global_load_access`

Future x86/riscv work should be able to discover addressing facts from
addressing/frame ownership without depending on the broad lookup facade.

## In Scope

- Address lookup declarations and definitions in:
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/prepared_lookups.cpp`
  - `src/backend/prealloc/addressing.hpp`
  - `src/backend/prealloc/stack_layout/stack_layout.hpp`
  - `src/backend/prealloc/stack_layout/coordinator.cpp`
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/publication_plans.cpp`
- Aggregate construction wiring for
  `PreparedFunctionLookups::address_materializations` and
  `PreparedFunctionLookups::memory_accesses`.
- Known AArch64 consumers:
  - `src/backend/mir/aarch64/module/module.hpp`
  - `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
  - `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
  - `src/backend/mir/aarch64/codegen/globals.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/memory_store_retargeting.*`

## Out Of Scope

- Changing address lowering, AArch64 addressing modes, TLS/global relocation
  behavior, or stack-frame layout.
- Rewriting store-source publication semantics in
  `publication_plans.*`; it may consume addressing lookups but should not be
  redesigned by this slice.
- Replacing prepared address facts with target-local scans or name matching.
- Broad frame-plan or stack-layout redesign beyond the lookup owner boundary.

## Acceptance Criteria

- Addressing lookup types and query helpers have an addressing/frame-domain
  declaration boundary instead of living only under `prepared_lookups.hpp`.
- Existing direct helpers in `addressing.hpp` and indexed helpers have a clear
  relationship, with no duplicate public names that differ only by owner.
- AArch64 memory/global consumers keep using prepared address facts and do not
  rediscover frame slots, address materializations, or global-load accesses
  locally.
- `PreparedFunctionLookups` or an equivalent aggregate still provides cached
  lookup access to consumers that need it.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Escalate to full `ctest --test-dir build -j --output-on-failure` if the
  implementation changes address/materialization semantics, global access
  matching, or frame slot resolution.

## Reviewer Reject Signals

- Reject if AArch64 memory/global code starts matching frame/global names or
  scanning BIR instructions to replace prepared addressing queries.
- Reject if TLS, global relocation, stack offset, or store-source publication
  behavior changes without a separately justified semantic bug.
- Reject if backend expectations are weakened, unsupported labels are added, or
  printer-only changes are presented as capability proof.
- Reject if the cleanup moves all addressing facts into another broad facade
  rather than a domain owner.
- Reject if cached lookup construction is removed and hot consumers rebuild the
  same memory/access maps per query.

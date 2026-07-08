Status: Active
Source Idea Path: ideas/open/600_pointer_value_memory_use_freshness_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Pointer-Value Memory Producers And Consumers

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: audited pointer-value memory producers,
shared prepared/prealloc support facts, diagnostics/printer carriers, and the
narrow RV64/AArch64 target consumers.

Producer surfaces audited:

- BIR/lowering producers in `lir_to_bir` create
  `MemoryAddress::BaseKind::PointerValue` from pointer provenance,
  dynamic pointer-value array materialization, intrinsics, local-slot pointer
  aliases, aggregate/calling/module paths, and address/provenance helpers.
- `stack_layout/coordinator.cpp` maps those BIR addresses into
  `PreparedAddressBaseKind::PointerValue` with `pointer_value_name`,
  byte offset, size/alignment, `can_use_base_plus_offset`, and memory
  provenance.
- `addressing.hpp` exposes
  `prepared_pointer_value_memory_has_proven_authority(...)`, which is an
  address-legality/range/layout helper, not selected pointer freshness.

Shared prepared/prealloc support surfaces audited:

- `prepared_lookups.cpp` copies pointer-value memory facts into edge
  publication records and treats `pointer_value_name` as a complete address
  base identity.
- `publication_plans.cpp/.hpp` carries source/destination pointer-value memory
  facts for edge/store-source plans, including store-local destination support,
  but those facts are not the pointer-value memory-use freshness owner.
- Prepared printer surfaces print pointer-value address facts, range verdicts,
  layout authority, offsets, and atomic pointer operands as diagnostics/dumps.
- Regalloc/liveness/pointer-carrier surfaces track pointer memory base values,
  dense-value liveness, and `PreparedPointerValueAccess` carrier state as
  placement/carry support only.

Consumer surfaces audited:

- Proposed representative route for Step 2/3: shared `PreparedMemoryAccess`
  records for `PreparedAddressBaseKind::PointerValue` load/store memory uses,
  with the initial target-side consumer limited to AArch64 prepared memory
  operand formation (`prepared_memory_operand_from_access(...)` /
  `make_prepared_pointer_value_base_register(...)`) consuming the shared
  selected result.
- RV64 `prepared_local_memory_emit.cpp` pointer-value load/store helpers and
  edge-publication source-memory uses are target-consume-only/deferred for this
  runbook.
- AArch64 dispatch, memory-store retargeting, intrinsic, inline-asm, call,
  atomic, and operand paths consume pointer-value address facts or target
  operand shape; only the prepared memory operand route is a representative
  candidate.
- Broad RV64/AArch64 target migration remains out of scope.

## Suggested Next

Execute Step 2 from `plan.md`: define the selected pointer-value memory-use
freshness contract for the shared `PreparedMemoryAccess` pointer-value route,
including use/source/proof/rank vocabulary and exact query dimensions for
pointer value identity, load/store use, program point, offset/range support,
provenance/layout support, and target-shape rejection.

## Watchouts

- Keep pointer base plus offset selected authority closed under idea 599.
- Do not treat `prepared_pointer_value_memory_has_proven_authority(...)`,
  object extent, offset/range proof, local layout, target offset encodability,
  target memory operand shape, diagnostics, dumps, or final assembly as
  selected pointer-value freshness.
- Loaded-value freshness, store-source freshness, semantic GEP target
  consumption, global symbol memory freshness, and broad target migration are
  separate routes.
- The representative migration should use a shared prepared/prealloc freshness
  helper; AArch64/RV64 target code should consume that result rather than own
  pointer-value freshness semantics.
- Step 2 should not reuse pointer-arithmetic, move-bundle, edge-publication,
  branch, select-carrier, loaded-value, or store-source freshness vocabulary
  unless it can prove the exact pointer-value memory-use boundary is already
  owned.

## Proof

Audit-only/todo-only packet. Build/tests were not run and `test_after.log` was
not updated. Proof command: `git diff --check` passed.

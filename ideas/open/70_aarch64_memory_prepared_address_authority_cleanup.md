# AArch64 Memory Prepared Address Authority Cleanup

## Goal

Make AArch64 memory lowering consume prepared addressing, value-home, storage,
frame-offset, and stack-source authority before selecting AArch64 load/store
forms and address materialization sequences.

## Why This Exists

The large-owner residue audit classified memory addressing, address
materialization, value homes, storage plans, frame offsets, aggregate
stack-source authority, and typed stack-source publication as `consume-shared`.
AArch64 should use those existing prepared facts directly while retaining
target-local decisions about encodable offsets, scratch registers, opcodes, and
address spelling.

## Owned Files

- `src/backend/mir/aarch64/codegen/memory.cpp`
- Relevant local consumers when memory-backed transfers cross owner boundaries:
  - `src/backend/mir/aarch64/codegen/f128.cpp`
  - `src/backend/mir/aarch64/codegen/variadic.cpp`
- Shared prepared authority call sites only where needed to consume existing
  facts:
  - `src/backend/prealloc/addressing.*`
  - `src/backend/prealloc/storage_plans.*`
  - `src/backend/prealloc/value_locations.hpp`
  - `src/backend/prealloc/prepared_lookups.*`

## In Scope

- Consume `PreparedAddressingFunction`, `PreparedMemoryAccess`,
  `PreparedAddressMaterialization`, frame-address offset queries,
  `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans,
  decoded home storage, `PreparedAggregateStackSourceAuthority`, and
  `PreparedTypedStackSourcePublication`.
- Remove duplicated source-authority and value-home decisions from AArch64
  memory helpers.
- Keep AArch64 encodable offset checks, base/scratch register selection, memory
  opcode selection, address spelling, and va_list field addressing local.
- Prove local instruction choice still handles local, global, frame, pointer,
  stack-source, and memory-backed transfer cases.

## Out Of Scope

- Inventing a new aggregate transport planner; that belongs to the separate
  aggregate-transport follow-up.
- Moving AArch64 offset legality, scratch-register selection, or load/store
  opcode selection into shared prealloc.
- Mechanical file splitting that does not reduce duplicated authority.
- Rewriting f128 or variadic lowering except where memory-backed prepared facts
  are directly consumed.

## Proof Expectations

- Focused AArch64 memory-lowering tests or CTest subset covering local/global
  addressing, frame offsets, value homes, typed stack sources, aggregate stack
  sources, and memory-backed f128 or variadic consumers when touched.
- Evidence that prepared address/value-home/storage facts are the source of
  truth before AArch64 instruction selection.
- Regression guard logs for acceptance-sized slices.

## Reviewer Reject Signals

- The route locally re-derives value homes, storage, stack-source authority, or
  frame offsets under a new helper name.
- The route claims shared-authority progress by only moving code within
  `memory.cpp`.
- AArch64-specific offset legality or opcode spelling is moved into shared
  prealloc without a target-neutral fact boundary.
- Tests are weakened, unsupported expectations are downgraded, or only one
  handpicked memory testcase is proven while nearby addressing forms are left
  unexamined.

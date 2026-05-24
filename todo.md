Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-candidate-audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Inspect Candidate Destination Layers

# Current Packet

## Just Finished

Step 3 - Inspect Candidate Destination Layers completed as a destination-layer
ownership audit for the Step 2 out-of-reference-model AArch64 codegen groups.

Inspected destination surfaces:
- `src/backend/prealloc/README.md`, `module.hpp`, `regalloc.hpp`,
  `value_locations.hpp`, `storage.hpp`, `calls.hpp`,
  `regalloc/value_homes.hpp`, `regalloc/call_moves.hpp`,
  `regalloc/move_records.hpp`, and `storage_plans.hpp`.
- `src/backend/bir/bir.hpp`, `lir_to_bir/lowering.hpp`,
  `lir_to_bir/call_abi.cpp`, and `lir_to_bir/memory/value_materialization.cpp`.
- `src/backend/mir/mir.hpp`, `printer.hpp`, and `printer.cpp`.
- x86 reuse surfaces: `src/backend/mir/x86/x86.hpp`, `api/api.hpp`,
  `core/core.*`, `module/module.hpp`, `prepared/prepared.hpp`,
  `prepared/dispatch.cpp`, `prepared/operands.cpp`, and
  `lowering/lowering.hpp`.
- RISC-V comparison surfaces: `src/backend/mir/riscv/codegen/README.md`,
  `riscv_codegen.hpp`, `mod.cpp`, and `emit.hpp`.
- AArch64 context surfaces: `src/backend/mir/aarch64/module/module.hpp`,
  `module/module.cpp`, `codegen/codegen.hpp`, `module_compile.hpp`,
  `traversal.hpp`, `dispatch.hpp`, `operands.hpp`, and
  `compatibility_projection.hpp`.

Destination classification:
- Prepared-module public handoff and top-level compile coordination
  (`codegen.hpp`, `module_compile.*`, `traversal.*`): plausible destination is
  a target API/module boundary, likely `src/backend/mir/aarch64/api/` plus
  `src/backend/mir/aarch64/module/`, backed by shared `src/backend/mir` for the
  `MachineModule`/`MachineFunction` stream contract. Evidence: shared MIR owns
  hierarchical machine module/function/block/instruction shape and printer
  traversal, while x86 exposes public prepared entrypoints in `api/api.hpp` and
  target module emission in `module/module.hpp`. Keep only AArch64-specific
  target handoff and BuildResult wiring target-local.
- Prepared fact indexing inside AArch64 lowering
  (`PreparedCallPlanIndexes`, `PreparedAddressMaterializationIndexes`,
  `PreparedMoveBundleIndexes`, `PreparedValueHomeIndexes` currently in
  `aarch64/module/module.hpp`): plausible destination is `src/backend/prealloc`
  as reusable prepared lookup/index helpers, or a shared prepared-consumer helper
  adjacent to prealloc if ownership should remain read-only. Evidence:
  `prealloc/module.hpp` publishes `PreparedBirModule` and find helpers for
  frame, dynamic stack, call plans, variadic entry plans, addressing, value
  locations, and storage; x86 already has `ConsumedPlans` in `x86.hpp` that
  performs similar plan lookup. Defer exact file split, but not target-local
  AArch64 codegen.
- Prepared value-home and storage decoding (`operands.*` generic portions):
  plausible destination is `src/backend/prealloc` shared helper APIs for
  resolving `PreparedStoragePlanValue`, `PreparedValueHome`, regalloc
  assignment, frame-slot, immediate, symbol, and label authority into a
  target-neutral operand decision. Evidence: `prealloc/value_locations.hpp` and
  `storage.hpp` define the home/storage schema and lookup helpers; x86
  prepared/lowering comments state targets should consume prepared plans and
  only map banks/registers to final target spellings. AArch64 register view,
  condition, immediate encoding, and final `mir::Operand` spelling remain
  target-local.
- Prepared move/publication, edge-copy, entry-formal, and store-source
  bookkeeping (`dispatch_publication.cpp`, `dispatch_edge_copies.cpp`,
  `dispatch_entry_formals.cpp`, `dispatch_store_sources.cpp`,
  `dispatch_value_materialization.cpp`, plus generic pieces of
  `dispatch_lookup.cpp`): plausible destination is `src/backend/prealloc`
  value-location/regalloc move-bundle producers or reusable prepared-consumer
  helpers. Evidence: `PreparedValueLocationFunction` already owns value homes
  and `PreparedMoveBundle` records by block/instruction phase; `regalloc` helper
  headers own move-record emission and call/return move resolution. x86 reuses
  this model by consuming prepared plans rather than rebuilding policy locally.
  Target-local AArch64 emission remains responsible for materializing the
  published moves as AArch64 instructions.
- Call-boundary prepared-plan interpretation (`dispatch_calls.cpp` and generic
  portions of `calls_common.cpp`, `calls_argument_sources.cpp`,
  `calls_moves.cpp`, `calls_preservation.cpp`): plausible destination is
  `src/backend/prealloc/calls.*`, `regalloc/call_moves.*`, and
  `regalloc/call_return_abi.*` for call argument/result placement,
  preservation, clobber, and move-plan publication. Evidence:
  `prealloc/README.md` explicitly assigns call argument/result placement,
  caller/callee-save policy, and target-facing move-resolution plans around
  calls and returns to prealloc; x86 `ConsumedPlans` and module comments consume
  those plans. AAPCS64 register names, sret details, variadic FPR/GP behavior,
  and printed call records remain AArch64 target-local.
- Generic diagnostics for missing prepared facts or unsupported prepared
  encodings (`dispatch_diagnostics.cpp` and diagnostics enum/message patterns
  in `aarch64/module/module.hpp`): plausible destination is a shared prepared
  consumer diagnostic utility under `src/backend/prealloc` or shared
  `src/backend/mir`. Evidence: the diagnostic kinds describe missing function
  context, value authority, typed register authority, prepared call plan,
  storage plan, and instruction/block mappings rather than AArch64 mnemonics.
  Unknown / defer for exact bucket because `src/backend/mir` owns machine
  context and `prealloc` owns the prepared facts being diagnosed.
- Compatibility projection (`compatibility_projection.*`): plausible
  destination is `src/backend/mir` as a generic legacy flat-view adapter, or
  eventual retirement if all callers consume hierarchical `MachineModule`.
  Evidence: shared MIR already provides `flatten_compatibility_instructions`
  helpers and comments that flat-vector printer paths are compatibility-only;
  AArch64 `FunctionRecord` separately labels `machine_nodes` as
  compatibility-only. Unknown / defer on whether this should be shared or
  deleted until callers/tests are audited.
- BIR lowering and LIR-to-BIR compatibility tables are not a direct destination
  for AArch64 prepared-consumer logic. Evidence: `src/backend/bir` owns semantic
  BIR shape, name tables, type/call ABI metadata, and LIR-boundary
  compatibility lowering; it does not own final prepared placement, machine
  value publication, target MIR traversal, or AArch64 instruction selection.
  Use BIR only for earlier semantic fact production when an audited
  responsibility is actually LIR-to-BIR lowering rather than prepared-to-MIR
  consumption.
- RISC-V does not provide a current reusable destination for the audited
  AArch64 prepared-consumer responsibilities. Evidence: RISC-V codegen mirrors
  the Rust reference stack-slot/text-emitter model and exposes
  `emit_prepared_lir_module` over LIR, not a shared `PreparedBirModule`
  consumer. RISC-V is useful as a reference-model contrast, not a destination
  bucket for prepared indexing/publication logic.

## Suggested Next

Start Step 4 by turning the destination audit into concrete migration seams:
separate target-local AArch64 instruction/ABI emission from generic
PreparedBirModule lookup, prepared index construction, move-bundle consumption,
call-plan consumption, and compatibility projection. Prefer the first narrow
slice that moves only reusable read-only helpers or records, leaving AArch64
emission behavior unchanged.

## Watchouts

Do not move AArch64 mnemonic spelling, register spelling, instruction-record
construction, AAPCS64 policy, target immediate/addressing constraints, or
target assembly printing out of the target.

`prealloc` is the strongest destination for authority-producing or
authority-indexing prepared facts, but target consumers should stay read-only:
they should not move policy back into x86 or AArch64 lowering. Shared `mir` is
the stronger destination only for machine stream shape, traversal, printing
context, and compatibility flattening.

x86 is the main reuse precedent for consuming prepared plans through
`ConsumedPlans`, `prepared/*`, `lowering/*`, and `module/*`. RISC-V currently
does not share the PreparedBirModule route and should not drive the destination
design beyond reference-model contrast.

## Proof

Documentation/audit-only packet. No build, ctest, or `test_after.log` was
required by the delegated proof because the owned change was limited to
canonical `todo.md` destination-layer audit notes. Inspected destination
files/directories are listed above; no implementation, plan, source-idea, or
reference files were touched.

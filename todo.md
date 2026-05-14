Status: Active
Source Idea Path: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Reconcile Active Compiled Surface Shards

# Current Packet

## Just Finished

Completed `plan.md` Step 2 ledger reconciliation for the active compiled
surface shard group: `alu.md`, `comparison.md`, `returns.md`, and `emit.md`.
Each entry below was checked against the current compiled owner before
classification.

Shard: `src/backend/mir/aarch64/codegen/alu.md`
Classification: already-converted/reconcile-ledger
Current Owner: `src/backend/mir/aarch64/codegen/alu.cpp`,
`src/backend/mir/aarch64/codegen/alu.hpp`, and the scalar record helpers in
`src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
Review Result: The current owner is the post-228 structured scalar ALU route,
not the archived accumulator/direct-assembly helper surface. It lowers
prepared BIR binary instructions through `lower_scalar_instruction`, consumes
prepared value/storage authority, records emitted scalar registers for return
chaining, and selects machine-node records for the accepted scalar ALU subset.
The legacy helper catalog's wider arithmetic, float-negation, byte-swap, popcount,
i128-copy, and direct assembly details are not current Step 2 owner gaps; they
remain archival evidence for later family-specific shard review rather than a
reason to recreate the old monolith.
Proof or Evidence: `alu.cpp:295` defines `lower_scalar_instruction`;
`instruction.cpp:624` maps current scalar ALU opcodes; `instruction.cpp:1349`
maps selected scalar ALU records to machine opcodes; `instruction.cpp:2141`
builds prepared scalar ALU records from value-location and storage-plan facts.
Focused evidence exists in
`tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp:147`
for prepared record conversion, `:391` for module build selection, and `:452`
for fail-closed unsupported opcodes such as `Mul` and compare predicates.
Follow-Up: none for Step 2; do not create a new missing-feature idea from the
legacy ALU helper catalog during this active-surface ledger pass.

Shard: `src/backend/mir/aarch64/codegen/comparison.md`
Classification: already-converted/reconcile-ledger
Current Owner: `src/backend/mir/aarch64/codegen/comparison.cpp`,
`src/backend/mir/aarch64/codegen/comparison.hpp`, and branch instruction record
helpers in `src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
Review Result: The current owner covers prepared branch-control lowering rather
than the old combined compare/select/text emitter hooks. It lowers prepared
unconditional branches, materialized-bool conditional branches, and fusable
compare branches into selected branch machine nodes, while block successors
preserve true/false and unconditional control-flow facts. Old materialized
boolean compare, float compare, soft-float `F128`, and select helper details
are not reintroduced here and are not classified as current-owner missing
without a later focused semantic route.
Proof or Evidence: `comparison.cpp:117` lowers prepared unconditional branch
terminators; `comparison.cpp:161` lowers prepared conditional branch
terminators; `comparison.cpp:237` enforces selected `ConditionalBranch` or
`CompareBranch` opcodes; `instruction.cpp:1391` maps branch records to machine
opcodes. Focused evidence exists in
`tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp:353` for
materialized-bool conditional branches and `:388` for fusable compare-branch
dispatch with recorded MIR successors.
Follow-Up: none for Step 2; any future materialized-compare, floating-compare,
or select enablement should be routed through a later focused feature idea only
after current prepared-owner review.

Shard: `src/backend/mir/aarch64/codegen/returns.md`
Classification: already-converted/reconcile-ledger
Current Owner: `src/backend/mir/aarch64/codegen/returns.cpp`,
`src/backend/mir/aarch64/codegen/returns.hpp`, scalar-state support from
`src/backend/mir/aarch64/codegen/alu.cpp` / `.hpp`, and return record helpers
in `src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
Review Result: The current owner covers prepared return terminator lowering
into a structured `ReturnInstructionRecord`, including immediate returns,
named/rematerialized returns, and scalar-result returns that reuse emitted
scalar register state. The archived ABI register move hooks for scalar FP,
binary128, second return components, epilogue text, and direct helper calls are
historical evidence, not current Step 2 active-surface gaps.
Proof or Evidence: `returns.cpp:120` builds typed return records from the BIR
terminator value; `returns.cpp:184` exposes `lower_prepared_return_terminator`;
`instruction.cpp:1912` creates selected return machine nodes with return and
control-flow side effects. Focused evidence exists in
`tests/backend/mir/backend_aarch64_return_lowering_test.cpp:360` for module
MIR return lowering, `:384` for immediate returns, `:417` for named
rematerialized returns, and `:441` / `:471` for scalar-result and scalar-chain
returns.
Follow-Up: none for Step 2; deeper ABI return-resource work belongs to a
focused ABI/call/frame route if later shard review proves a current missing
feature.

Shard: `src/backend/mir/aarch64/codegen/emit.md`
Classification: already-converted/reconcile-ledger
Current Owner: `src/backend/mir/aarch64/codegen/emit.cpp`,
`src/backend/mir/aarch64/codegen/emit.hpp`,
`src/backend/mir/aarch64/codegen/traversal.cpp` / `.hpp`,
`src/backend/mir/aarch64/codegen/compatibility_projection.cpp` / `.hpp`, and
the public facade in `src/backend/mir/aarch64/module/module.cpp` / `.hpp`
Review Result: The current owner is the thin prepared-module build entry point,
not the archived direct BIR/LIR text renderer, recognizer pile, or assembler
handoff surface. `build_module` validates the prepared AArch64 handoff, creates
the structured module/MIR containers, lowers prepared functions, and derives
compatibility projections for migration callers. The old direct assembly,
shape recognizer, constant-fold fallback, type-string parser, and object
assembly responsibilities are stale with respect to this active compiled
surface and must not be restored through Step 2.
Proof or Evidence: `emit.cpp:11` defines `build_module`; `emit.cpp:13` resolves
the target profile; `emit.cpp:14` validates the prepared handoff; `emit.cpp:27`
lowers prepared functions; `emit.cpp:29` / `:31` derive compatibility
projections. Focused evidence exists in
`tests/backend/mir/backend_aarch64_prepared_handoff_gate_test.cpp:53` and `:71`
for fail-closed target/ABI handoff validation, plus the return and scalar
module-build tests cited above for prepared-module integration.
Follow-Up: none for Step 2; route-boundary and stale monolith retirement should
continue in Step 3.

## Suggested Next

Execute `plan.md` Step 3 by reconciling only the route-boundary and stale
structural shard group: `mod.md`, `records.md`, and `asm_emitter.md`.

## Watchouts

- Do not mechanically convert markdown shards into same-named C++ files.
- Do not reopen the shared MIR printer boundary closed by idea 224.
- Do not treat `records.md` as a mandate to recreate legacy record ownership.
- Do not classify a missing feature before checking current compiled owners.
- Keep compatibility projection out of terminal assembly printing; terminal
  assembly must walk shared `module::MachineModule` through
  `mir::print_machine_module` plus the AArch64 `MachineInstructionPrinter`.
- The Step 2 ledger deliberately does not promote every archived helper detail
  into missing-feature work. Later shard groups should still check current
  owners before deciding whether any broader arithmetic, comparison, ABI, or
  emit behavior is a real missing feature.

## Proof

Ledger-only classification; no build or test proof required, and no broad
validation was run. No `test_after.log` was produced because the delegated
proof explicitly required ledger-only classification.

Evidence inspected with focused `rg`, `sed`, and `nl` reads of:

- `ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md`
- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/emit.md`
- `src/backend/mir/aarch64/codegen/alu.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/returns.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/emit.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
- focused tests under `tests/backend/mir/` covering scalar ALU records,
  branch-control lowering, return lowering, and prepared handoff gating

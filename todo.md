Status: Active
Source Idea Path: ideas/open/aarch64-codegen-reference-layout-consolidation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Layout Review

# Current Packet

## Just Finished

Completed Step 5 - Final Layout Review.

Final direct inventory under `src/backend/mir/aarch64/codegen/`: 77 files,
split into 41 `.cpp` and 36 `.hpp`.

Final family comparison against the reference README inventory:

| Family | Direct files | Review note |
| --- | ---: | --- |
| `codegen/module` | 5 | Covers the current `codegen.hpp` public entry plus module compile/traversal; this is the live replacement for reference `mod.rs`/`emit.rs` routing. |
| `prologue` | 3 | Prologue/frame lowering plus entry-formal publication are family-named and intentionally split. |
| `calls` | 10 | Calls remain split by ABI planning, argument sources, moves, preservation, printing, byval, and dispatch bridge responsibilities. |
| `memory` | 6 | Memory lowering owns direct load/store plus dynamic stack and store-source publication helpers. |
| `alu` | 2 | Reference scalar ALU family is represented by one narrow header and one implementation. |
| `comparison` | 4 | Comparison/select/branch spelling is represented; branch fusion stays a cohesive comparison shard with dispatch hooks. |
| `float_ops` | 2 | Reference floating-point operation family is represented. |
| `cast_ops` | 2 | Reference cast operation family is represented. |
| `f128` | 2 | Reference binary128 family is represented. |
| `i128_ops` | 2 | Reference 128-bit integer family is represented. |
| `atomics` | 2 | Reference atomic family is represented. |
| `intrinsics` | 2 | Reference intrinsics family is represented. |
| `globals` | 2 | Reference globals family is represented. |
| `returns` | 2 | Reference returns family is represented. |
| `variadic` | 2 | Reference variadic family is represented. |
| `inline_asm` | 2 | Reference inline asm family is represented. |
| `asm_emitter` | 2 | Reference asm emission surface is represented as the live assembly text consumer. |
| `peephole` | 2 | Reference peephole family is represented as the current deferred optimizer boundary. |
| `machine_printer` | 2 | Current C++ structured MIR printer support has no direct reference Rust shard but is a durable live output family. |
| `operands/instruction` | 4 | Current C++ instruction and operand record support replaces the old centralized record/emit surface. |
| `adapter/internal` | 17 | Remaining dispatch/projection glue is private boundary work, not a missing reference family. |

Remaining intentionally separate adapter/internal files:

- `compatibility_projection.{cpp,hpp}` stays separate as legacy flat-view
  projection for migration callers and tests; terminal assembly printing now
  walks structured MIR instead.
- `dispatch.{cpp,hpp}` stays separate as the block-dispatch entry surface and
  dispatcher implementation. `dispatch.hpp` now exposes only
  `InstructionDispatchResult`, `make_block_lowering_context`, and
  `dispatch_prepared_block`.
- `dispatch_diagnostics.{cpp,hpp}` stays separate as narrow diagnostic helper
  glue shared by dispatcher, calls, and dynamic-stack paths.
- `dispatch_edge_copies.{cpp,hpp}` stays separate as cross-block join/edge copy
  publication logic; forcing it into memory, comparison, or prologue would
  blur the current authority boundary.
- `dispatch_lookup.{cpp,hpp}` stays separate as prepared value/home lookup glue
  shared by multiple lowering shards.
- `dispatch_producers.{cpp,hpp}` stays separate as same-block producer
  discovery shared by fusion, publication, and materialization paths.
- `dispatch_publication.{cpp,hpp}` and
  `dispatch_publication_common.hpp` stay separate as current-block prepared
  value publication helpers and shared scalar/frame-slot vocabulary.
- `dispatch_value_materialization.{cpp,hpp}` stays separate as prepared
  value-home materialization glue across stack, register, global, pointer, and
  FP cases.

Other intentionally separate family/internal bridge files:

- `calls_dispatch_bridge.{cpp,hpp}` stays separate because it bridges
  dispatch-time lowering context into prepared call-plan handling and remains
  large/cohesive enough to keep out of `calls.cpp`.
- `comparison_branch_fusion.{cpp,hpp}` stays as a comparison-family shard while
  fused branch lowering still needs dispatch publication hooks.
- `memory_dynamic_stack.{cpp,hpp}` stays as a memory-family dynamic alloca
  helper-call boundary.
- `memory_store_sources.{cpp,hpp}` stays as a memory-family store-source and
  store-publication boundary still coupled to dispatch publication helpers.
- `prologue_entry_formals.cpp` stays as a prologue-family implementation shard
  for ABI entry-formal publication.

Target-neutral migration candidate check: none discovered during this final
layout review. The remaining non-reference files are AArch64 codegen
adapter/internal glue or durable live C++ support families, not hidden prealloc,
BIR, or target-neutral ownership moves.

Closure recommendation: ask the plan owner to close
`ideas/open/aarch64-codegen-reference-layout-consolidation.md`. The visible
AArch64 codegen layout now maps to the reference family inventory, the broad
`dispatch.hpp` surface has been reduced to block dispatch, remaining
adapter/internal files are explicitly justified, and no separate initiative was
found that must be opened before closure.

## Suggested Next

Supervisor should hand this closure recommendation to the plan owner for the
single-plan lifecycle decision. No executor implementation packet is
recommended from this Step 5 audit.

## Watchouts

- The retained Step 1 inventory below is historical packet output. The final
  Step 5 count is the current authoritative count: 77 total, 41 `.cpp`, 36
  `.hpp`.
- Direct implementation shards may still include both `dispatch.hpp` and narrow
  private dispatch headers when they need the block-dispatch context plus
  private adapter helpers.
- `dispatch_publication.hpp`, `dispatch_publication_common.hpp`,
  `dispatch_edge_copies.hpp`, and `dispatch_value_materialization.hpp`
  intentionally depend on `alu.hpp` for `BlockScalarLoweringState` and register
  operand vocabulary.
- There is no target-neutral migration follow-up to create from this audit.

## Proof

Ran delegated proof command:

`git diff --check`

Result: passed.

# Retained Step 1 Output

Completed Step 1 - Map AArch64 Codegen Files. The direct inventory under
`src/backend/mir/aarch64/codegen/` is 66 files: 41 `.cpp` and 25 `.hpp`.

File-to-family map:

| File | Family | Audit note |
| --- | --- | --- |
| `alu.cpp` | `alu` | Durable large scalar lowering and printing implementation; includes `dispatch.hpp` only for cross-dispatch publication helpers. |
| `alu.hpp` | `alu` | Durable scalar lowering surface; also owns `BlockScalarLoweringState`, which makes it a dependency of dispatch internals. |
| `asm_emitter.cpp` | `asm_emitter` | Durable prepared-module assembly text emission entry point. |
| `asm_emitter.hpp` | `asm_emitter` | Narrow public compile-to-assembly interface. |
| `atomics.cpp` | `atomics` | Durable atomic lowering implementation. |
| `atomics.hpp` | `atomics` | Narrow atomic lowering declaration surface. |
| `calls.cpp` | `calls` | Durable call-plan entry point and RAII call-preservation toggle; thin at 177 lines but central, not a merge target yet. |
| `calls.hpp` | `calls` | Broad family header with declarations grouped by `calls_*` shard; acceptable family surface but large. |
| `calls_argument_sources.cpp` | `calls` | Durable call argument source/materialization helpers; large enough to stay split. |
| `calls_byval_aggregates.cpp` | `calls` | Durable byval aggregate lane/stack logic; includes `dispatch.hpp` for frame/value publication helpers and is a possible later header-narrowing beneficiary. |
| `calls_common.cpp` | `calls` | Small shared calls helpers; durable but thin. Could stay as calls-common support or merge only if a later calls-only packet needs it. |
| `calls_dispatch_bridge.cpp` | `calls` | Durable calls-family dispatch bridge: scalar call producers, call lowering, call boundary materialization, indirect callee materialization, missing frame-slot args, stack-preserved values. Keep separate from `calls.cpp` while it bridges dispatcher context and prepared call plans. |
| `calls_dispatch_bridge.hpp` | `calls` | Narrow bridge declaration surface used by direct lowering callers. |
| `calls_moves.cpp` | `calls` | Durable large call boundary move lowering implementation; should stay split. |
| `calls_preservation.cpp` | `calls` | Durable preserved-value analysis/republication helpers; includes `dispatch.hpp` for diagnostic/context helpers and is a possible later header-narrowing beneficiary. |
| `calls_printing.cpp` | `calls` | Durable call instruction construction, effect modeling, and printer support; should stay split. |
| `cast_ops.cpp` | `cast_ops` | Durable cast lowering and printing implementation. |
| `cast_ops.hpp` | `cast_ops` | Narrow cast lowering/printing declarations. |
| `codegen.hpp` | `codegen/module` | Public codegen typedefs and `compile` entry declaration. |
| `comparison.cpp` | `comparison` | Durable comparison, select, and branch spelling implementation. |
| `comparison.hpp` | `comparison` | Durable comparison lowering and branch spelling declarations. |
| `compatibility_projection.cpp` | `adapter/internal` | Compatibility projection from prepared/module records; historical boundary adapter, not a reference family. |
| `compatibility_projection.hpp` | `adapter/internal` | Narrow adapter header for compatibility projection. |
| `dispatch.cpp` | `adapter/internal` | Main block dispatcher and retargeting glue; should shrink but not be merged into one giant dispatch file. |
| `dispatch.hpp` | `adapter/internal` | Current catch-all dispatch/internal header. It exposes block dispatch plus producer lookup, publication, edge copies, store sources, entry formals, and lookup declarations. Step 2 and Step 3 have narrowed some shard declarations, but this header still needs continued shrinkage. |
| `comparison_branch_fusion.cpp` | `comparison` | Durable fused compare/branch lowering, now using a comparison-family filename while temporarily depending on dispatch hooks. Keep separate until hook dependencies are narrow enough to justify moving declarations toward `comparison.hpp` or a private branch-fusion header. |
| `comparison_branch_fusion.hpp` | `comparison` | Narrow branch-fusion declaration surface for the comparison-family shard; still exposes dispatch hook plumbing by necessity. |
| `dispatch_diagnostics.cpp` | `adapter/internal` | Thin diagnostics helper; first low-risk header extraction candidate because the declarations are self-contained and used by dispatch/calls/dynamic-stack paths. |
| `dispatch_edge_copies.cpp` | `adapter/internal` | Join/edge copy publication glue across predecessor/successor contexts; temporarily justified as dispatch-internal. |
| `dispatch_lookup.cpp` | `adapter/internal` | Thin prepared-value/home lookup helpers; temporarily justified as dispatch-internal and a possible private-header split after diagnostics. |
| `dispatch_producers.cpp` | `adapter/internal` | Same-block producer analysis used by fusion/publication; temporarily justified as dispatch-internal. |
| `dispatch_publication.cpp` | `adapter/internal` | Current-block prepared-value publication helpers; cross-cuts memory/alu/comparison, so keep private adapter/internal until a cleaner family boundary emerges. |
| `dispatch_value_materialization.cpp` | `adapter/internal` | Prepared value materialization to registers/stacks; cross-cutting glue, temporarily justified as adapter/internal. |
| `f128.cpp` | `f128` | Durable binary128 lowering/printing/runtime-helper support. |
| `f128.hpp` | `f128` | Durable f128 declarations. |
| `float_ops.cpp` | `float_ops` | Durable floating-point scalar operation lowering/printing. |
| `float_ops.hpp` | `float_ops` | Narrow float operation declarations. |
| `globals.cpp` | `globals` | Durable global symbol/address materialization lowering and printing. |
| `globals.hpp` | `globals` | Durable globals declarations. |
| `i128_ops.cpp` | `i128_ops` | Durable 128-bit integer lowering/printing/runtime-helper support. |
| `i128_ops.hpp` | `i128_ops` | Durable i128 declarations, with module forward declarations. |
| `inline_asm.cpp` | `inline_asm` | Durable inline asm substitution/lowering/printing support. |
| `inline_asm.hpp` | `inline_asm` | Durable inline asm declarations. |
| `instruction.cpp` | `operands/instruction` | Durable instruction-record construction/inspection/printing support. |
| `instruction.hpp` | `operands/instruction` | Large durable instruction and operand record type surface. |
| `intrinsics.cpp` | `intrinsics` | Durable intrinsic lowering/printing support. |
| `intrinsics.hpp` | `intrinsics` | Durable intrinsic declarations. |
| `machine_printer.cpp` | `machine_printer` | Durable central instruction printer, dispatching to family printers. |
| `machine_printer.hpp` | `machine_printer` | Narrow printer class and helper declarations. |
| `memory.cpp` | `memory` | Durable load/store/address lowering and printing implementation. |
| `memory_dynamic_stack.cpp` | `memory` | Durable dynamic alloca helper-call lowering, now using a memory-family filename. Keep as a separate implementation shard because it is cohesive and still bridges prepared dynamic-stack plans with dispatch call handling. |
| `memory_dynamic_stack.hpp` | `memory` | Narrow dynamic-stack helper-call declaration surface used by direct lowering callers. |
| `memory.hpp` | `memory` | Durable memory lowering declarations. |
| `memory_store_sources.cpp` | `memory` | Store-source publication and pointer/global store helpers, now using a memory-family filename. Keep as a separate implementation shard because it is cohesive and still coupled to dispatch publication. |
| `memory_store_sources.hpp` | `memory` | Narrow store-source declaration surface used by direct lowering callers. |
| `module_compile.cpp` | `codegen/module` | Public compile entry point wrapping traversal/projection. |
| `module_compile.hpp` | `codegen/module` | Narrow module compile declaration. |
| `operands.cpp` | `operands/instruction` | Durable operand resolution helpers. |
| `operands.hpp` | `operands/instruction` | Narrow operand resolution declarations. |
| `peephole.cpp` | `peephole` | Stub/no-op optimizer implementation; durable family placeholder. |
| `peephole.hpp` | `peephole` | Narrow peephole declaration surface. |
| `prologue.cpp` | `prologue` | Durable frame-boundary/prologue insertion implementation. |
| `prologue_entry_formals.cpp` | `prologue` | Entry formal publication complements frame/prologue setup; now uses a prologue-family filename while staying separate as cohesive ABI/home-publication bridge logic. |
| `prologue.hpp` | `prologue` | Narrow prologue declaration surface. |
| `returns.cpp` | `returns` | Durable return instruction printing support. |
| `returns.hpp` | `returns` | Durable return printing declarations. |
| `traversal.cpp` | `codegen/module` | Prepared-module/function/block traversal that calls dispatch. |
| `traversal.hpp` | `codegen/module` | Narrow traversal declarations. |
| `variadic.cpp` | `variadic` | Durable variadic helper lowering/printing support. |
| `variadic.hpp` | `variadic` | Durable variadic declarations. |

Specific dispatch surface notes:

- `dispatch.hpp`: adapter/internal catch-all. It currently includes `alu.hpp`,
  ABI/module/query headers, and still declares several dispatch shard
  interfaces in one public-looking header. The safest shrink is extracting
  remaining self-contained declarations into narrow private headers used only by
  implementation shards.
- `comparison_branch_fusion.cpp`: maps to `comparison`; now has a
  durable-family filename but still needs dispatch publication callbacks. Keep
  separate while hook dependencies remain, then consider whether declarations
  should move further toward `comparison.hpp`.
- `dispatch_diagnostics.cpp`: maps to `adapter/internal`; declarations are thin,
  cohesive, and low-risk to extract first.
- `memory_dynamic_stack.cpp`: maps to `memory`; dynamic alloca helper-call
  lowering now has a memory-family filename and a narrow direct header while
  remaining separate from `memory.cpp`.
- `dispatch_edge_copies.cpp`: maps to `adapter/internal`; cross-block join copy
  publication is genuine dispatch glue and should stay private.
- `prologue_entry_formals.cpp`: maps to `prologue`; entry-formal publication
  now has a durable-family filename but remains a separate shard because it
  depends on dispatch context, prepared homes, and scalar state.
- `dispatch_lookup.cpp`: maps to `adapter/internal`; thin lookup helpers are a
  later private-header candidate after diagnostics.
- `dispatch_producers.cpp`: maps to `adapter/internal`; same-block producer
  helpers serve multiple dispatch shards and should stay private for now.
- `dispatch_publication.cpp`: maps to `adapter/internal`; current-block value
  publication is broad internal glue and should not be forced into a durable
  family yet.
- `memory_store_sources.cpp`: maps to `memory`; store-source publication now
  has a memory-family filename while remaining separate from `memory.cpp`
  because the implementation is cohesive and still coupled to dispatch
  publication.
- `dispatch_value_materialization.cpp`: maps to `adapter/internal`; value-home
  materialization is shared internal glue and should remain private until its
  callers are narrowed.

Specific calls shard notes:

- `calls.cpp`: central calls entry point, thin but durable because it owns
  `lower_prepared_call_instruction` and the scoped preservation toggle.
- `calls_argument_sources.cpp`: durable split for call argument source operands;
  keep split.
- `calls_byval_aggregates.cpp`: durable byval aggregate support; keep split,
  but later remove `dispatch.hpp` dependency if frame-slot helpers move.
- `calls_common.cpp`: thin shared helper file; low priority merge candidate only
  inside a calls-only cleanup packet.
- `calls_dispatch_bridge.cpp`: durable calls-family bridge from dispatch-time
  lowering context into prepared call-plan handling; keep split from `calls.cpp`
  because the implementation remains large and cohesive.
- `calls_moves.cpp`: large durable call boundary move lowering; keep split.
- `calls_preservation.cpp`: durable preservation analysis/republication; keep
  split, but later remove `dispatch.hpp` dependency if diagnostics/helpers move.
- `calls_printing.cpp`: durable calls printer/effect model; keep split.

Prioritized consolidation list:

1. Step 2 low-risk header packet: extract `dispatch_diagnostics` declarations
   from `dispatch.hpp` into a narrow private header, update only implementation
   includes that need diagnostics, and keep behavior unchanged.
2. Grouped Step 3 route-boundary decision: justify the remaining
   adapter/internal `dispatch_*` shard names, or mechanically rename one more
   clearly durable-family shard if the family boundary is already obvious.
3. Split `dispatch_lookup` declarations into a private lookup header used by
   dispatch internals.
4. Revisit whether `comparison_branch_fusion.hpp` should move closer to
   `comparison.hpp` once the hook dependency can remain private.
5. Keep `prologue_entry_formals.cpp`, `memory_dynamic_stack.cpp`, and
   `memory_store_sources.cpp` as separate family-named shards unless a later
   family-only cleanup finds a real merge benefit.
6. Leave large cohesive family files (`alu`, `memory`, `calls_moves`, `f128`,
   `i128_ops`, `machine_printer`, `instruction`) split as durable families; do
   not merge them just to reduce file count.

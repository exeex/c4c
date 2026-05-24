Status: Active
Source Idea Path: ideas/open/aarch64-codegen-reference-layout-consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Reduce Catch-All Header Surface

# Current Packet

## Just Finished

Completed Step 2 - Reduce Catch-All Header Surface. Added
`src/backend/mir/aarch64/codegen/dispatch_branch_fusion.hpp` as the narrow
private declaration surface for branch-fusion helpers and
`DispatchBranchFusionHooks`, removed those declarations from `dispatch.hpp`,
and updated direct implementation users to include the new header.

Direct callers updated:

- `dispatch_branch_fusion.cpp` now includes its own narrow header so its
  definitions are checked against the extracted declaration surface.
- `dispatch.cpp` includes `dispatch_branch_fusion.hpp` directly for the hook
  construction and branch-fusion lowering calls.
- `dispatch_calls.cpp`, `dispatch_edge_copies.cpp`, and
  `dispatch_value_materialization.cpp` include `dispatch_branch_fusion.hpp`
  directly for shared branch condition/immediate helpers.

## Suggested Next

Recommended next packet: extract the dispatch producer declarations from
`dispatch.hpp` into a narrow private producer header, then update only direct
same-block producer users.

## Watchouts

- `calls.hpp` still has a pre-existing `append_call_diagnostic` declaration even
  though direct implementation users now include `dispatch_diagnostics.hpp`;
  removing that declaration would cross this packet's owned-files boundary.
- `dispatch_branch_fusion.cpp` needs `../../query.hpp` directly for MIR
  same-block producer/constant query helpers that were previously available
  through `dispatch.hpp`.
- `dispatch_calls.cpp` still includes `dispatch.hpp` for frame/value
  publication, producer lookup, select-chain, and stack/frame helper
  declarations; those are separate header-narrowing candidates and were not
  widened into this packet.
- `prepared_value_home_for_value` remains in `dispatch.hpp` because it is
  implemented in `dispatch_publication.cpp`; moving publication or
  value-materialization declarations was intentionally outside this packet.
- Several files still include both `dispatch.hpp` and `dispatch_lookup.hpp`
  because they also use non-lookup dispatch internals; this packet only removed
  the lookup helper declarations from the catch-all header.

## Proof

Initial compile check passed with `cmake --build --preset default`.

Ran delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed. `cmake --build --preset default` completed, and CTest reported
162/162 `^backend_` tests passed. Proof log: `test_after.log`.

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
| `dispatch.hpp` | `adapter/internal` | Current catch-all dispatch/internal header. It exposes block dispatch plus branch fusion, calls glue, diagnostics, dynamic stack, producer lookup, publication, edge copies, store sources, entry formals, and lookup declarations. Step 2 should narrow this. |
| `dispatch_branch_fusion.cpp` | `comparison` | Durable fused compare/branch lowering, but temporarily depends on dispatch hooks. Candidate to move declarations toward `comparison.hpp` or a private branch-fusion header after hook boundary is narrowed. |
| `dispatch_calls.cpp` | `calls` | Durable dispatch-to-calls bridge: scalar call producers, call lowering, call boundary materialization, indirect callee materialization, missing frame-slot args, stack-preserved values. Should move toward `calls.hpp`/calls-private surface instead of `dispatch.hpp`. |
| `dispatch_diagnostics.cpp` | `adapter/internal` | Thin diagnostics helper; first low-risk header extraction candidate because the declarations are self-contained and used by dispatch/calls/dynamic-stack paths. |
| `dispatch_dynamic_stack.cpp` | `memory` | Durable dynamic alloca helper-call lowering; should move toward `memory` or a narrow dynamic-stack private header. |
| `dispatch_edge_copies.cpp` | `adapter/internal` | Join/edge copy publication glue across predecessor/successor contexts; temporarily justified as dispatch-internal. |
| `dispatch_entry_formals.cpp` | `prologue` | Entry formal publication complements frame/prologue setup; durable responsibility but currently dispatch-internal. |
| `dispatch_lookup.cpp` | `adapter/internal` | Thin prepared-value/home lookup helpers; temporarily justified as dispatch-internal and a possible private-header split after diagnostics. |
| `dispatch_producers.cpp` | `adapter/internal` | Same-block producer analysis used by fusion/publication; temporarily justified as dispatch-internal. |
| `dispatch_publication.cpp` | `adapter/internal` | Current-block prepared-value publication helpers; cross-cuts memory/alu/comparison, so keep private adapter/internal until a cleaner family boundary emerges. |
| `dispatch_store_sources.cpp` | `memory` | Store-source publication and pointer/global store helpers; durable memory/store responsibility but still coupled to dispatch publication. |
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
| `memory.hpp` | `memory` | Durable memory lowering declarations. |
| `module_compile.cpp` | `codegen/module` | Public compile entry point wrapping traversal/projection. |
| `module_compile.hpp` | `codegen/module` | Narrow module compile declaration. |
| `operands.cpp` | `operands/instruction` | Durable operand resolution helpers. |
| `operands.hpp` | `operands/instruction` | Narrow operand resolution declarations. |
| `peephole.cpp` | `peephole` | Stub/no-op optimizer implementation; durable family placeholder. |
| `peephole.hpp` | `peephole` | Narrow peephole declaration surface. |
| `prologue.cpp` | `prologue` | Durable frame-boundary/prologue insertion implementation. |
| `prologue.hpp` | `prologue` | Narrow prologue declaration surface. |
| `returns.cpp` | `returns` | Durable return instruction printing support. |
| `returns.hpp` | `returns` | Durable return printing declarations. |
| `traversal.cpp` | `codegen/module` | Prepared-module/function/block traversal that calls dispatch. |
| `traversal.hpp` | `codegen/module` | Narrow traversal declarations. |
| `variadic.cpp` | `variadic` | Durable variadic helper lowering/printing support. |
| `variadic.hpp` | `variadic` | Durable variadic declarations. |

Specific dispatch surface notes:

- `dispatch.hpp`: adapter/internal catch-all. It currently includes `alu.hpp`,
  ABI/module/query headers, and declares every dispatch shard interface in one
  public-looking header. The safest shrink is extracting self-contained
  declarations into narrow private headers used only by implementation shards,
  starting with diagnostics.
- `dispatch_branch_fusion.cpp`: maps to `comparison`; keep temporarily while
  hooks still need dispatch publication callbacks, then move branch-fusion
  declarations out of `dispatch.hpp`.
- `dispatch_calls.cpp`: maps to `calls`; should stop being declared from
  `dispatch.hpp` once the calls bridge has a calls-owned or private dispatch-call
  surface.
- `dispatch_diagnostics.cpp`: maps to `adapter/internal`; declarations are thin,
  cohesive, and low-risk to extract first.
- `dispatch_dynamic_stack.cpp`: maps to `memory`; dynamic alloca helper-call
  lowering should eventually have a memory/dynamic-stack boundary.
- `dispatch_edge_copies.cpp`: maps to `adapter/internal`; cross-block join copy
  publication is genuine dispatch glue and should stay private.
- `dispatch_entry_formals.cpp`: maps to `prologue`; entry-formal publication is
  prologue-adjacent but currently depends on dispatch context and scalar state.
- `dispatch_lookup.cpp`: maps to `adapter/internal`; thin lookup helpers are a
  later private-header candidate after diagnostics.
- `dispatch_producers.cpp`: maps to `adapter/internal`; same-block producer
  helpers serve multiple dispatch shards and should stay private for now.
- `dispatch_publication.cpp`: maps to `adapter/internal`; current-block value
  publication is broad internal glue and should not be forced into a durable
  family yet.
- `dispatch_store_sources.cpp`: maps to `memory`; store-source publication is
  memory/store responsibility but too coupled for the first packet.
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
- `calls_moves.cpp`: large durable call boundary move lowering; keep split.
- `calls_preservation.cpp`: durable preservation analysis/republication; keep
  split, but later remove `dispatch.hpp` dependency if diagnostics/helpers move.
- `calls_printing.cpp`: durable calls printer/effect model; keep split.

Prioritized consolidation list:

1. Step 2 low-risk header packet: extract `dispatch_diagnostics` declarations
   from `dispatch.hpp` into a narrow private header, update only implementation
   includes that need diagnostics, and keep behavior unchanged.
2. Move `dispatch_calls.cpp` declarations toward `calls.hpp` or a narrow
   calls-private bridge so calls logic is not advertised by the catch-all
   dispatch header.
3. Split `dispatch_lookup` declarations into a private lookup header used by
   dispatch internals.
4. Move branch-fusion declarations toward `comparison` once the hook dependency
   can remain private.
5. Revisit memory/prologue-adjacent dispatch shards:
   `dispatch_dynamic_stack.cpp`, `dispatch_store_sources.cpp`, and
   `dispatch_entry_formals.cpp`.
6. Leave large cohesive family files (`alu`, `memory`, `calls_moves`, `f128`,
   `i128_ops`, `machine_printer`, `instruction`) split as durable families; do
   not merge them just to reduce file count.

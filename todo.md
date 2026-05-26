Status: Active
Source Idea Path: ideas/open/20_aarch64_codegen_layout_classification.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Build The Durable Classification Table

# Current Packet

## Just Finished

Completed `Step 4: Build The Durable Classification Table` by merging the
Step 1 inventory, Step 2/3 family classifications, and the Step 4
evidence-resolution audit into one durable classification table.

Bucket meanings:

- `move-forward`: semantic authority belongs in BIR, MIR, or shared prepare.
- `fold-back`: target-local helper logic should merge into a matching
  reference-style owner.
- `keep-local`: AArch64-specific emission, ABI, representation, or orchestration
  justifies the owner boundary.
- `keep-local (reference-owner)`: the AArch64 file/family already maps to a
  reference ARM owner family and is not extra layout debt for this plan.

| File/family | Bucket | Proposed owner | Rationale | Follow-up idea target |
| --- | --- | --- | --- | --- |
| `alu.cpp`, `alu.hpp` | keep-local (reference-owner) | `alu` | Direct counterpart to reference ARM `alu`; AArch64 instruction spelling and operand selection stay target-local. | None. |
| `asm_emitter.cpp`, `asm_emitter.hpp` | keep-local (reference-owner) | `asm_emitter` | Direct counterpart to reference ARM `asm_emitter`; final assembly formatting is target-local. | None. |
| `atomics.cpp`, `atomics.hpp` | keep-local (reference-owner) | `atomics` | Direct counterpart to reference ARM `atomics`; atomic instruction emission is target-local. | None. |
| `calls.cpp`, `calls.hpp` | keep-local (reference-owner) | `calls` | Direct counterpart to reference ARM `calls`; owns the AArch64 calls/ABI lowering entry points. | None, except as the destination for calls fold-back cleanup below. |
| `calls_byval_aggregates.cpp` | fold-back | `calls.cpp` byval/call-argument section | Consumes prepared byval lane source selections and emits AArch64 memory/register operands; idea 17 says byval source choice is prepared authority. | Mechanical AArch64 calls fold-back idea. |
| `calls_common.cpp` | fold-back | `calls.cpp` calls-private helper section | Stack-argument sizing, fixed-frame/`va_start` offsets, prepared register conversion, F128 carrier checks, scalar views, and diagnostics are calls/ABI utilities around prepared facts. | Mechanical AArch64 calls fold-back idea. |
| `calls_dispatch_bridge.cpp`, `calls_dispatch_bridge.hpp` | fold-back | `calls.cpp` call-lowering entry points | Dispatch invokes the bridge, but the bridge consumes `PreparedCallPlan`, lowers call-boundary moves/materialization, handles dynamic-stack helper calls, and records call results as calls/ABI work. | Mechanical AArch64 calls fold-back idea. |
| `calls_moves.cpp` | fold-back | `calls.cpp` calls-private move section | Emits prepared call-boundary move bundles, ABI register/stack placements, preservation republication, before/after-call moves, before-return moves, and target register spelling. | Mechanical AArch64 calls fold-back idea. |
| `cast_ops.cpp`, `cast_ops.hpp` | keep-local (reference-owner) | `cast_ops` | Direct counterpart to reference ARM `cast_ops`; AArch64 cast instruction selection stays target-local. | None. |
| `codegen.hpp` | keep-local (reference-owner) | `mod` / target codegen interface | C++ target codegen interface header corresponding to the reference `mod` owner. | None. |
| `comparison.cpp`, `comparison.hpp` | keep-local (reference-owner) | `comparison` | Direct counterpart to reference ARM `comparison`; target compare lowering remains local. | None, except as the destination for branch-fusion fold-back below. |
| `comparison_branch_fusion.cpp`, `comparison_branch_fusion.hpp` | fold-back | `comparison.cpp` comparison/branch lowering section | Consumes prepared branch-condition facts and emits AArch64 compare/branch spellings, scratch materialization, and same-block shortcuts. | Mechanical AArch64 comparison fold-back idea. |
| `compatibility_projection.cpp`, `compatibility_projection.hpp` | fold-back | `module_compile.cpp` or module-private compatibility section | Caller audit found legacy flat projection construction at the module build boundary; terminal assembly authority is the MIR stream, not `FunctionRecord::machine_nodes`. | Mechanical module compatibility fold-back idea, if supervisor wants this tracked separately. |
| `constant_materialization.cpp`, `constant_materialization.hpp` | keep-local | AArch64 constant materialization owner | MOV/MOVK-style integer materialization, register widths, and immediate spelling are AArch64 emission details. | None. |
| `dispatch.cpp`, `dispatch.hpp` | keep-local | AArch64 dispatch orchestrator | Coordinates prepared-block traversal, instruction lowering, diagnostics, call bridging, publication hooks, and terminator emission; closure evidence does not make dispatch shared semantic authority debt. | None; later slimming only after destination owners are classified. |
| `dispatch_diagnostics.cpp`, `dispatch_diagnostics.hpp` | fold-back | `dispatch.cpp` diagnostics section | Builds target-local lowering diagnostics and unsupported-operation messages; no separate semantic or reference owner. | Mechanical AArch64 dispatch fold-back idea. |
| `dispatch_edge_copies.cpp`, `dispatch_edge_copies.hpp` | keep-local | AArch64 edge-copy emission and hazard helper | Idea 16 moved edge/block-entry value-flow authority to prepared facts; remaining copy emission and clobber avoidance are target-local. | None unless a later audit finds semantic producer rediscovery outside prepared facts. |
| `dispatch_lookup.cpp`, `dispatch_lookup.hpp` | keep-local | AArch64 prepared-home lookup adapter | Reads prepared ids/homes and local scalar state for emission decisions; ideas 16 and 19 validate shared prepared lookup authority. | None. |
| `dispatch_producers.cpp`, `dispatch_producers.hpp` | keep-local | AArch64 same-block producer/emission adapter | Supports local materialization and hazard choices for constants, select chains, globals, and current-block join copies; not the prepared edge-publication authority path. | None unless fresh evidence identifies missing shared prepared facts. |
| `dispatch_publication.cpp`, `dispatch_publication.hpp` | keep-local | AArch64 publication emission owner | Emits publication, retargeting, and clobber/hazard handling around shared prepared records; scratch/register spelling and stack operands stay target-local. | None. |
| `dispatch_publication_common.hpp` | fold-back | `dispatch_publication` private declarations | Publication helper declarations for register aliases, frame-slot addressing, scalar widths, and register views do not justify a standalone owner. | Mechanical AArch64 publication fold-back idea. |
| `dispatch_value_materialization.cpp`, `dispatch_value_materialization.hpp` | keep-local | AArch64 value materialization owner | Emits AArch64 register materialization sequences using target registers and scratch policy; idea 18 treats clobber-sensitive materialization as local emission responsibility. | None, or later local merge into publication if desired. |
| `effects.cpp`, `effects.hpp` | keep-local | AArch64 effects owner | AArch64-only effect modeling/printing support from the inventory; no Step 2/3 evidence marks it as semantic migration debt. | None. |
| `f128.cpp`, `f128.hpp` | keep-local (reference-owner) | `f128` | Direct counterpart to reference ARM `f128`; target handling of F128 carrier details remains local. | None. |
| `float_ops.cpp`, `float_ops.hpp` | keep-local (reference-owner) | `float_ops` | Direct counterpart to reference ARM `float_ops`; FP operation emission is target-local. | None. |
| `fp_value_materialization.cpp`, `fp_value_materialization.hpp` | keep-local | AArch64 FP value materialization owner | Chooses FP/GP scratch behavior and AArch64 instruction lines for FP immediate/value materialization and publication. | None. |
| `globals.cpp`, `globals.hpp` | keep-local (reference-owner) | `globals` | Direct counterpart to reference ARM `globals`; symbol/global emission spelling stays target-local. | None. |
| `i128_ops.cpp`, `i128_ops.hpp` | keep-local (reference-owner) | `i128_ops` | Direct counterpart to reference ARM `i128_ops`; I128 operation emission is target-local. | None. |
| `inline_asm.cpp`, `inline_asm.hpp` | keep-local (reference-owner) | `inline_asm` | Direct counterpart to reference ARM `inline_asm`; target constraint and assembly spelling stay local. | None. |
| `instruction.cpp`, `instruction.hpp` | keep-local | AArch64 instruction representation owner | AArch64-only machine instruction representation support from the inventory; no classification evidence marks it as shared semantic authority debt. | None. |
| `intrinsics.cpp`, `intrinsics.hpp` | keep-local (reference-owner) | `intrinsics` | Direct counterpart to reference ARM `intrinsics`; target intrinsic emission stays local. | None. |
| `machine_printer.cpp`, `machine_printer.hpp` | keep-local | AArch64 machine printer owner | AArch64-only machine-module printing support; idea 19 keeps final assembly formatting and target spelling local. | None. |
| `memory.cpp`, `memory.hpp` | keep-local (reference-owner) | `memory` | Direct counterpart to reference ARM `memory`; owns AArch64 memory lowering entry points. | None, except as the destination for memory fold-back cleanup below. |
| `memory_dynamic_stack.cpp`, `memory_dynamic_stack.hpp` | keep-local | AArch64 memory/dynamic-stack helper | Recognizes prepared dynamic-stack operations and emits AArch64 `sp` manipulation with x16/x17 scratch policy and stable-frame diagnostics. | None, or optional mechanical memory fold-back if local owner count is later reduced. |
| `memory_store_sources.cpp`, `memory_store_sources.hpp` | move-forward | Shared prepare or BIR/MIR store-source publication planning first; target-local residue later owned by `memory.cpp` | Audit found target-neutral producer/source facts for narrow-store recovery, byval frame-slot loads, cast/select/scalar-FP producer publication, direct global-load select chains, and pending store-global stack publications mixed with AArch64 emission. | Semantic store-source publication planning idea, followed by mechanical AArch64 memory fold-back idea for remaining emission helpers. |
| `module_compile.cpp`, `module_compile.hpp` | keep-local (reference-owner) | `emit` / module compilation entry family | C++ module compilation entry family corresponding to reference ARM `emit`; builds AArch64 machine modules. | None, except as possible destination for compatibility projection fold-back. |
| `operands.cpp`, `operands.hpp` | keep-local | AArch64 operands owner | AArch64-only operand representation/spelling support from the inventory; no Step 2/3 evidence marks it as migration debt. | None. |
| `peephole.cpp`, `peephole.hpp` | keep-local (reference-owner) | `peephole` | Direct counterpart to reference ARM `peephole`; target peephole decisions remain local. | None. |
| `prepared_value_home_materialization.cpp`, `prepared_value_home_materialization.hpp` | keep-local | AArch64 prepared-home materialization owner | Consumes shared prepared value-home facts and emits AArch64 loads/moves using physical registers and stack addresses. | None. |
| `prologue.cpp`, `prologue.hpp` | keep-local (reference-owner) | `prologue` | Direct counterpart to reference ARM `prologue`; owns AArch64 function entry/prologue setup. | None, except as the destination for entry-formals fold-back below. |
| `prologue_entry_formals.cpp` | fold-back | `prologue.cpp` entry-formal publication/prologue setup section | Lowers prepared formal publication plans into AArch64 stores, loads, register moves, byval entry copies, and F128 handling under prologue/ABI ownership. | Mechanical AArch64 prologue fold-back idea. |
| `returns.cpp`, `returns.hpp` | keep-local (reference-owner) | `returns` | Direct counterpart to reference ARM `returns`; consumes prepared value homes and emits/diagnoses AArch64 return terminator behavior. | None. |
| `traversal.cpp`, `traversal.hpp` | keep-local | AArch64 traversal owner | AArch64-only traversal support from the inventory; no Step 2/3 evidence marks it as shared semantic authority debt. | None. |
| `variadic.cpp`, `variadic.hpp` | keep-local (reference-owner) | `variadic` | Direct counterpart to reference ARM `variadic`; AArch64 variadic ABI behavior stays target-local. | None. |

Coverage check:

- Direct reference-owner files are covered by the `keep-local
  (reference-owner)` rows.
- Extra dispatch, publication, producer, lookup, diagnostic, materialization,
  bridge, compatibility projection, calls, memory, comparison, prologue, and
  returns families are explicitly covered.
- No family remains `needs-more-evidence`: `compatibility_projection.*` is
  resolved as `fold-back`, and `memory_store_sources.*` is resolved as
  `move-forward` for semantic source-publication planning with later
  target-local memory fold-back.

## Suggested Next

Proceed to `Step 5: Create Numbered Follow-Up Ideas` by creating separate,
coherent ideas for:

- semantic store-source publication planning from `memory_store_sources.*`;
- mechanical AArch64 calls fold-back into `calls.cpp`;
- mechanical AArch64 dispatch/publication fold-back for diagnostics/common
  declarations;
- mechanical AArch64 comparison/prologue/module-compatibility fold-back where
  the supervisor wants tracked cleanup.

## Watchouts

Keep semantic migration separate from mechanical file consolidation. In
particular, `memory_store_sources.*` should not be treated as pure mechanical
memory cleanup until the target-neutral producer/source-publication facts are
moved into shared prepare or BIR/MIR publication planning.

The `keep-local (reference-owner)` rows are not cleanup requests. They are
included only so the durable table covers every current AArch64 codegen
`.cpp`/`.hpp` family from the Step 1 inventory.

## Proof

Not run. This was a classification-table-only packet, and the delegated proof
explicitly said not to run build/tests and not to modify `test_after.log`
because this step normalizes classification artifacts and the plan forbids
root proof-log changes.

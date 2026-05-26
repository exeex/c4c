Status: Active
Source Idea Path: ideas/open/20_aarch64_codegen_layout_classification.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Extra AArch64 Codegen Files

# Current Packet

## Just Finished

Completed `Step 1: Inventory Extra AArch64 Codegen Files` by comparing
`src/backend/mir/aarch64/codegen/` against the reference ARM owner layout in
`ref/claudes-c-compiler/src/backend/arm/codegen/`.

Reference ARM owner files are Rust modules, not `.cpp`/`.hpp` files. The
reference owner families used for this inventory are:

`alu`, `asm_emitter`, `atomics`, `calls`, `cast_ops`, `comparison`, `emit`,
`f128`, `float_ops`, `globals`, `i128_ops`, `inline_asm`, `intrinsics`,
`memory`, `mod`, `peephole`, `prologue`, `returns`, and `variadic`.

| AArch64 file(s) | Step 1 owner-family mapping |
| --- | --- |
| `alu.cpp`, `alu.hpp` | Reference owner: `alu` |
| `asm_emitter.cpp`, `asm_emitter.hpp` | Reference owner: `asm_emitter` |
| `atomics.cpp`, `atomics.hpp` | Reference owner: `atomics` |
| `calls.cpp`, `calls.hpp` | Reference owner: `calls` |
| `calls_byval_aggregates.cpp` | Reference owner: `calls`; extra AArch64 split helper |
| `calls_common.cpp` | Reference owner: `calls`; extra AArch64 split helper |
| `calls_dispatch_bridge.cpp`, `calls_dispatch_bridge.hpp` | Reference owner: `calls`; extra AArch64 dispatch bridge helper |
| `calls_moves.cpp` | Reference owner: `calls`; extra AArch64 split helper |
| `cast_ops.cpp`, `cast_ops.hpp` | Reference owner: `cast_ops` |
| `codegen.hpp` | Reference owner: `mod`; C++ target codegen interface header |
| `comparison.cpp`, `comparison.hpp` | Reference owner: `comparison` |
| `comparison_branch_fusion.cpp`, `comparison_branch_fusion.hpp` | Reference owner: `comparison`; extra AArch64 branch-fusion helper |
| `compatibility_projection.cpp`, `compatibility_projection.hpp` | Extra/AArch64-only family: compatibility projection |
| `constant_materialization.cpp`, `constant_materialization.hpp` | Extra/AArch64-only family: constant materialization |
| `dispatch.cpp`, `dispatch.hpp` | Extra/AArch64-only family: dispatch |
| `dispatch_diagnostics.cpp`, `dispatch_diagnostics.hpp` | Extra/AArch64-only family: dispatch diagnostics |
| `dispatch_edge_copies.cpp`, `dispatch_edge_copies.hpp` | Extra/AArch64-only family: dispatch edge copies |
| `dispatch_lookup.cpp`, `dispatch_lookup.hpp` | Extra/AArch64-only family: dispatch lookup |
| `dispatch_producers.cpp`, `dispatch_producers.hpp` | Extra/AArch64-only family: dispatch producers |
| `dispatch_publication.cpp`, `dispatch_publication.hpp`, `dispatch_publication_common.hpp` | Extra/AArch64-only family: dispatch publication |
| `dispatch_value_materialization.cpp`, `dispatch_value_materialization.hpp` | Extra/AArch64-only family: dispatch value materialization |
| `effects.cpp`, `effects.hpp` | Extra/AArch64-only family: effects |
| `f128.cpp`, `f128.hpp` | Reference owner: `f128` |
| `float_ops.cpp`, `float_ops.hpp` | Reference owner: `float_ops` |
| `fp_value_materialization.cpp`, `fp_value_materialization.hpp` | Extra/AArch64-only family: floating-point value materialization |
| `globals.cpp`, `globals.hpp` | Reference owner: `globals` |
| `i128_ops.cpp`, `i128_ops.hpp` | Reference owner: `i128_ops` |
| `inline_asm.cpp`, `inline_asm.hpp` | Reference owner: `inline_asm` |
| `instruction.cpp`, `instruction.hpp` | Extra/AArch64-only family: instruction representation |
| `intrinsics.cpp`, `intrinsics.hpp` | Reference owner: `intrinsics` |
| `machine_printer.cpp`, `machine_printer.hpp` | Extra/AArch64-only family: machine printer |
| `memory.cpp`, `memory.hpp` | Reference owner: `memory` |
| `memory_dynamic_stack.cpp`, `memory_dynamic_stack.hpp` | Reference owner: `memory`; extra AArch64 dynamic-stack helper |
| `memory_store_sources.cpp`, `memory_store_sources.hpp` | Reference owner: `memory`; extra AArch64 store-source helper |
| `module_compile.cpp`, `module_compile.hpp` | Reference owner: `emit`; C++ module compilation entry family |
| `operands.cpp`, `operands.hpp` | Extra/AArch64-only family: operands |
| `peephole.cpp`, `peephole.hpp` | Reference owner: `peephole` |
| `prepared_value_home_materialization.cpp`, `prepared_value_home_materialization.hpp` | Extra/AArch64-only family: prepared value home materialization |
| `prologue.cpp`, `prologue.hpp` | Reference owner: `prologue` |
| `prologue_entry_formals.cpp` | Reference owner: `prologue`; extra AArch64 entry-formals helper |
| `returns.cpp`, `returns.hpp` | Reference owner: `returns` |
| `traversal.cpp`, `traversal.hpp` | Extra/AArch64-only family: traversal |
| `variadic.cpp`, `variadic.hpp` | Reference owner: `variadic` |

## Suggested Next

Start `Step 2: Classify Dispatch And Publication Families` using the
extra/AArch64-only dispatch, publication, compatibility-projection, and
materialization families from the Step 1 inventory.

## Watchouts

The reference ARM path has no `.cpp`/`.hpp` files; its owner families are
derived from `*.rs` module names. The dispatch/publication/materialization
families have no direct ARM owner counterpart and should be classified before
any follow-up idea is proposed.

## Proof

Not run. This was a classification/inventory-only packet, and the delegated
proof explicitly said not to run build/tests or modify `test_after.log` because
the step only inventories file paths and the plan forbids root proof-log
changes.

Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-candidate-audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory AArch64 Codegen Responsibilities

# Current Packet

## Just Finished

Step 1 - Inventory AArch64 Codegen Responsibilities completed for
`src/backend/mir/aarch64/codegen`.

Inspected evidence:
- Directory inventory covered all 65 files under `src/backend/mir/aarch64/codegen`:
  `README.md`, `alu.cpp`, `alu.hpp`, `asm_emitter.cpp`, `asm_emitter.hpp`,
  `atomics.cpp`, `atomics.hpp`, `calls.cpp`, `calls.hpp`,
  `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
  `calls_common.cpp`, `calls_moves.cpp`, `calls_preservation.cpp`,
  `calls_printing.cpp`, `cast_ops.cpp`, `cast_ops.hpp`, `codegen.hpp`,
  `comparison.cpp`, `comparison.hpp`, `compatibility_projection.cpp`,
  `compatibility_projection.hpp`, `dispatch.cpp`, `dispatch.hpp`,
  `dispatch_branch_fusion.cpp`, `dispatch_calls.cpp`,
  `dispatch_diagnostics.cpp`, `dispatch_dynamic_stack.cpp`,
  `dispatch_edge_copies.cpp`, `dispatch_entry_formals.cpp`,
  `dispatch_lookup.cpp`, `dispatch_producers.cpp`,
  `dispatch_publication.cpp`, `dispatch_store_sources.cpp`,
  `dispatch_value_materialization.cpp`, `f128.cpp`, `f128.hpp`,
  `float_ops.cpp`, `float_ops.hpp`, `globals.cpp`, `globals.hpp`,
  `i128_ops.cpp`, `i128_ops.hpp`, `inline_asm.cpp`, `inline_asm.hpp`,
  `instruction.cpp`, `instruction.hpp`, `intrinsics.cpp`, `intrinsics.hpp`,
  `machine_printer.cpp`, `machine_printer.hpp`, `memory.cpp`, `memory.hpp`,
  `module_compile.cpp`, `module_compile.hpp`, `operands.cpp`,
  `operands.hpp`, `peephole.cpp`, `peephole.hpp`, `prologue.cpp`,
  `prologue.hpp`, `returns.cpp`, `returns.hpp`, `traversal.cpp`,
  `traversal.hpp`, `variadic.cpp`, and `variadic.hpp`.
- `README.md` names the live route as `backend.cpp` driver -> prepared BIR ->
  `compile_prepared_module(...)` -> `module_compile` -> traversal/dispatch/family
  lowerers -> compiled AArch64 target module/nodes -> `asm_emitter` plus shared
  MIR printer.
- `codegen.hpp` exposes the public prepared-module entry and aliases
  `CompiledModule` / `CompileResult` to module-level MIR build products.
- `module_compile.cpp` validates the prepared-module handoff, builds the module
  shell, runs `lower_prepared_functions`, and derives compatibility records.
- `traversal.*` builds function lowering context and walks prepared functions.
- `dispatch.*` and `dispatch_*.cpp` own block/instruction bridge work:
  prepared block contexts, diagnostics, producer lookup, value materialization,
  value publication, store sources, entry formals, edge copies, dynamic-stack
  helpers, call routing, and fused branch handling.

Responsibility grouping:
- Prepared/MIR bridge and orchestration: `codegen.hpp`, `module_compile.*`,
  `traversal.*`, `dispatch.*`, `dispatch_branch_fusion.cpp`,
  `dispatch_calls.cpp`, `dispatch_diagnostics.cpp`,
  `dispatch_dynamic_stack.cpp`, `dispatch_edge_copies.cpp`,
  `dispatch_entry_formals.cpp`, `dispatch_lookup.cpp`,
  `dispatch_producers.cpp`, `dispatch_publication.cpp`,
  `dispatch_store_sources.cpp`, `dispatch_value_materialization.cpp`,
  `compatibility_projection.*`, and `operands.*`.
- Call/ABI bridge helpers: `calls.hpp`, `calls.cpp`, `calls_common.cpp`,
  `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
  `calls_moves.cpp`, `calls_preservation.cpp`, and `calls_printing.cpp`.
  These translate prepared call plans, move bundles, clobbers, byval aggregates,
  preserved values, and ABI bindings into call-boundary machine nodes and
  printable records.
- Target-local instruction records and printer surface: `instruction.*`,
  `machine_printer.*`, `asm_emitter.*`, and `peephole.*`. These own AArch64
  record enums/payloads, name/status helpers, target instruction printing,
  prepared-node assembly text output, and the currently deferred peephole
  boundary.
- Mixed family lowerers with both prepared-bridge inputs and target-local
  emission/printing: `alu.*`, `memory.*`, `comparison.*`, `cast_ops.*`,
  `float_ops.*`, `globals.*`, `atomics.*`, `intrinsics.*`, `inline_asm.*`,
  `f128.*`, `i128_ops.*`, `returns.*`, `prologue.*`, and `variadic.*`.
  Headers show `make_prepared_*`, `lower_*`, `make_*_instruction`, and/or
  `print_*` APIs, so these files are not pure emitters; they straddle prepared
  fact consumption, machine-node construction, and AArch64 spelling.

## Suggested Next

Start Step 2 by comparing this inventory against the forward-migration
candidate criteria in `plan.md`, using the bridge/orchestration groups as the
first likely candidates for extraction or clearer ownership boundaries.

## Watchouts

Most family files are mixed surfaces, not pure target-local emission. Treating
them as movable solely because they print AArch64 text would blur their
prepared-value, storage-plan, and MIR-node responsibilities.

`compatibility_projection.*` is explicitly compatibility-only for legacy MIR
tests and migration callers; terminal assembly output should continue through
the machine module, shared MIR printer, and target instruction printer.

## Proof

Documentation/audit-only packet. No build, ctest, or `test_after.log` was
required because the owned change was limited to canonical `todo.md` inventory
evidence and no implementation files were touched.

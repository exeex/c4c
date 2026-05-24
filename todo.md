Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-candidate-audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Compare Against the Reference Codegen Model

# Current Packet

## Just Finished

Step 2 - Compare Against the Reference Codegen Model completed for the Step 1
inventory of `src/backend/mir/aarch64/codegen`.

Reference README sections used:
- `Architecture Overview`: reference codegen is an `ArchCodegen` text-emission
  implementation centered on per-function `ArmCodegen` state, accumulator-style
  value flow, target register allocation, and final assembly-line cleanup.
- `File Inventory`: reference codegen responsibilities are target modules such
  as `emit.rs`, `prologue.rs`, `calls.rs`, `memory.rs`, `alu.rs`,
  `comparison.rs`, `float_ops.rs`, `cast_ops.rs`, `f128.rs`, `i128_ops.rs`,
  `atomics.rs`, `intrinsics.rs`, `globals.rs`, `returns.rs`, `variadic.rs`,
  `inline_asm.rs`, `asm_emitter.rs`, and `peephole.rs`.
- ABI and target-behavior sections used as target-local anchors:
  `Calling Convention (AAPCS64)`, `Register Allocation`, `Stack Frame Layout`,
  `Addressing Modes`, `F128 Quad-Precision Handling`,
  `128-bit Integer Operations`, `Atomic Operations`, `NEON/SIMD Intrinsics`,
  `Inline Assembly Support`, `Peephole Optimizer`, `Codegen Options`, and
  `Key Design Decisions`.

Reference-model comparison result:
- Clean target-local matches: `alu.*`, `memory.*`, `comparison.*`,
  `cast_ops.*`, `float_ops.*`, `globals.*`, `atomics.*`, `intrinsics.*`,
  `inline_asm.*`, `f128.*`, `i128_ops.*`, `returns.*`, `prologue.*`,
  `variadic.*`, and `peephole.*` map to named reference inventory families when
  they are doing AArch64 instruction selection, AAPCS64 frame/call/return
  emission, register spelling, target address-form spelling, target feature
  emission, or target-local cleanup.
- Preserve target ABI emission as target-local: AAPCS64 lane placement, sret
  register behavior, variadic save-area emission, prologue/epilogue register
  saves, call clobber/preservation effects, return-value placement, and
  AArch64-specific stack/addressing constraints should stay in the AArch64
  codegen bucket even if adjacent classification or move-planning helpers later
  migrate.
- Preserve instruction printing as target-local: `instruction.*`,
  `machine_printer.*`, `asm_emitter.*`, and family `print_*` helpers are the
  current structured-node-to-AArch64 spelling surface. This differs from the
  reference text-first emitter shape, but still belongs to the target because it
  owns mnemonic, operand, condition-code, register-name, and assembly syntax
  spelling.
- Out-of-reference-model responsibilities: `codegen.hpp`,
  `module_compile.*`, and `traversal.*` expose and coordinate a prepared-module
  to compiled-MIR route. The reference README has no equivalent prepared BIR
  handoff, reusable compiled target module product, shared MIR printer consumer,
  or Prepared function traversal utility.
- Out-of-reference-model responsibilities: `dispatch.*`,
  `dispatch_branch_fusion.cpp`, `dispatch_calls.cpp`,
  `dispatch_diagnostics.cpp`, `dispatch_dynamic_stack.cpp`,
  `dispatch_edge_copies.cpp`, `dispatch_entry_formals.cpp`,
  `dispatch_lookup.cpp`, `dispatch_producers.cpp`,
  `dispatch_publication.cpp`, `dispatch_store_sources.cpp`, and
  `dispatch_value_materialization.cpp` mostly interpret Prepared facts,
  construct block/function indexes, materialize value homes, publish machine
  values, classify edge copies, route Prepared call plans, and produce generic
  diagnostics. The reference model expects codegen to emit target operations
  from existing IR/state; it does not describe this Prepared-to-machine-record
  bookkeeping layer.
- Out-of-reference-model responsibilities: `operands.*` decodes Prepared value
  homes and storage encodings into lowering operands. AArch64 register and
  immediate spelling remains target-local, but the value-home decoding contract
  itself is not present in the reference ARM codegen responsibility model.
- Out-of-reference-model responsibilities: `compatibility_projection.*` builds
  compatibility records for migration callers and legacy MIR tests. The
  reference README describes final assembly text generation and target helpers,
  not a compatibility projection from compiled target MIR back into older test
  contracts.
- Mixed responsibilities requiring later classification: `calls_common.cpp`,
  `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
  `calls_moves.cpp`, and `calls_preservation.cpp` combine target ABI effects
  with prepared call-plan interpretation, move-bundle expansion, byval aggregate
  staging, and preserved-value bookkeeping. The AAPCS64 register/stack effects
  map to the reference `calls.rs`; generic call-boundary planning and
  prepared-fact interpretation do not. `calls_printing.cpp` remains target-local
  when it only spells printable AArch64 call records.
- Mixed responsibilities requiring later classification: family files with
  `make_prepared_*`, `lower_*`, and `make_*_instruction` APIs often bridge
  Prepared operands into machine nodes before target spelling. Their actual
  AArch64 instruction selection maps to the reference inventory; any generic
  Prepared operand recovery, publication, diagnostic, or storage-plan decoding
  should be considered outside the reference ARM codegen model.

## Suggested Next

Start Step 3 by inspecting destination layers for the out-of-reference-model
groups: `src/backend/bir`, `src/backend/prealloc`, shared MIR/module code under
`src/backend/mir`, and the x86/RISC-V MIR codegen surfaces. Focus first on
Prepared move/publication indexing, value-home/storage interpretation,
entry-formal publication, call-boundary move classification, edge-copy
bookkeeping, and generic diagnostics.

## Watchouts

The reference README is text-first, while the current route is structured
target MIR plus printer. Do not treat that architectural difference as a reason
to move AArch64 instruction records, mnemonic spelling, register spelling,
target ABI emission, or assembly printing out of the target.

Most migration-worthy areas are mixed with target effects. Step 3 should split
generic Prepared/MIR bookkeeping from AAPCS64 policy hooks before proposing a
destination layer.

## Proof

Documentation/audit-only packet. No build, ctest, or `test_after.log` was
required by the delegated proof because the owned change was limited to
canonical `todo.md` reference-model comparison notes and no implementation,
plan, source-idea, or reference files were touched.

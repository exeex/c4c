Status: Active
Source Idea Path: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory HIR-to-LIR Legacy Layout Authority

# Current Packet

## Just Finished

Step 1 inventory completed for HIR-to-LIR `StructuredLayoutLookup` consumers.
Targeted scans/readbacks covered `legacy_decl`, `StructuredLayoutLookup`,
`structured_decl`, and `structured_name_id` references under
`src/codegen/lir/hir_to_lir`.

Classification:
- Observation-only:
  - `src/codegen/lir/hir_to_lir/core.cpp:82` records
    `legacy_found`, legacy size/align, and legacy field types for structured
    layout observations. This should stay available for mismatch reporting.
  - `src/codegen/lir/hir_to_lir/core.cpp:106` populates `legacy_decl` and uses
    it for structured-vs-legacy parity observation. This is lookup construction
    and parity reporting, not a demotion target by itself.
  - `src/codegen/lir/hir_to_lir/lvalue.cpp:662` uses `structured_decl` /
    `structured_name_id` directly for indexed aggregate GEP type refs and does
    not read `legacy_decl`.
  - `src/codegen/lir/hir_to_lir/types.cpp:43` / `:56` / `:124` use
    `structured_decl` only to attach `structured_name_id` to field-chain steps;
    field discovery still comes from the HIR `HirStructDef` already in scope.
- Fallback-only:
  - No direct HIR-to-LIR call-site read is purely fallback-only today. The
    fallback role lives in `lookup_structured_layout` itself and in call sites
    that still need legacy coverage when structured coverage is absent.
- Active semantic authority:
  - `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:98` returns
    `layout.legacy_decl` as the selected const-init aggregate declaration before
    falling back to the same `mod_.struct_defs` lookup.
  - `src/codegen/lir/hir_to_lir/core.cpp:468` and
    `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:144` use
    `legacy_decl->align_bytes` as object alignment authority.
  - `src/codegen/lir/hir_to_lir/call/args.cpp:169` uses
    `legacy_decl->size_bytes` as variadic aggregate payload-size authority.
  - `src/codegen/lir/hir_to_lir/call/vaarg.cpp:151` derives aggregate
    zero/nonzero payload behavior from legacy union field sizes or legacy struct
    size, and `:265` uses `legacy_decl->size_bytes` for the AArch64 aggregate
    `va_arg` payload path.

## Suggested Next

First narrow code-changing demotion target: make
`ConstInitEmitter::lookup_const_init_struct_def` treat
`lookup_structured_layout(..., "const-init-aggregate")` as observation-only and
stop selecting semantic aggregate authority through `layout.legacy_decl`.
Preserve the existing `mod_.struct_defs` fallback return because the const
initializer still consumes `HirStructDef` fields.

Relevant owned files for that next packet:
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`
- `todo.md`

Supervisor proof command proposed for that next packet:
`cmake --build build --target c4cll && ctest --test-dir build -R 'frontend_lir_global_type_ref|cpp_hir_record_field_array_layout|cpp_hir_record_packed_aligned_layout|cpp_positive_sema_inherited_base_aggregate_init_runtime_cpp' --output-on-failure`

## Watchouts

- Keep `legacy_decl` fallback for incomplete structured coverage; do not remove
  `StructuredLayoutLookup::legacy_decl`.
- Do not edit `src/backend/mir/` as part of this route.
- Do not weaken tests or add testcase-shaped shortcuts.
- Size/alignment demotions are not the first safe packet: `LirStructDecl`
  currently carries field type refs but not explicit size/align metadata, so
  `object_align_bytes`, variadic aggregate payload sizing, and `va_arg`
  aggregate payload sizing need a parity-gated helper/data decision before
  their `legacy_decl` reads can be safely demoted.
- Const-init demotion should preserve output drift exactly; it only removes the
  call-site dependence on `layout.legacy_decl` as authority.

## Proof

Inventory-only proof; no build required and no `test_after.log` generated.
Commands run:
- `rg -n "legacy_decl|StructuredLayoutLookup|structured_decl|structured_name_id" src/codegen/lir/hir_to_lir`
- `rg -n "lookup_structured_layout|record_structured|legacy_decl" src/codegen/lir/hir_to_lir`
- Focused `sed`/`nl` reads of the relevant HIR-to-LIR call sites listed above.

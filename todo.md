Status: Active
Source Idea Path: ideas/open/355_rv64_prepared_object_shape_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Rejection Paths

# Current Packet

## Just Finished

Step 1 audit traced the RV64 prepared-object rejection path:

- `src/backend/backend.cpp::emit_rv64_prepared_object_module` calls
  `write_rv64_prepared_relocatable_elf_object` and emits the generic
  `RISC-V backend object route unsupported prepared module shape` whenever the
  returned image is `std::nullopt`.
- `src/backend/mir/riscv/codegen/object_emission.cpp` already has
  `RiscvPreparedObjectModuleResult` and
  `build_rv64_prepared_text_object_module_with_diagnostics`, but
  `write_rv64_prepared_relocatable_elf_object` calls the older
  `build_rv64_prepared_text_object_module` wrapper and drops
  `prepared_consumer_category` plus `diagnostic`.
- Shared consumer-contract diagnostics are already consumed for unplaced
  parallel-copy obligations, move-bundle traversal events, and select-carrier
  classification via `diagnose_unplaced_parallel_copy_obligations`,
  `classify_prepared_object_move_bundle_consumer`, and
  `classify_prepared_object_select_consumer`.
- The shared taxonomy lives in
  `src/backend/prealloc/prepared_object_traversal.{hpp,cpp}` and covers select
  carriers, value homes, move bundles, frame slots, and unplaced parallel-copy
  obligations. Category names are available through
  `prepared_object_consumer_diagnostic_category_name`.
- RV64 target evidence is still lost at bare `{}` / `std::nullopt` returns for
  string constants, missing/unsupported function admission, unsupported
  parameter homes, unsupported stack/call-frame immediates, unsupported move
  bundle shapes after the shared classifier says the bundle is structurally
  available, unsupported instruction/terminator fragments, unsupported local
  memory widths/addressing, unsupported globals/constant-object layout, and
  object-module/ELF construction failure.
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp` already throws
  detailed text-route diagnostics for unsupported external calls and global
  storage layout, but that path is separate from the object route and should not
  be treated as Step 2's insertion point.

## Suggested Next

Step 2 should add a small diagnostic-preserving RV64 object-result handoff:
call `build_rv64_prepared_text_object_module_with_diagnostics` from the RV64
object route, surface existing shared consumer diagnostics through
`backend.cpp`, and add RV64-local rejection reasons only at the top-level
admission/helper gates that currently return an empty
`RiscvPreparedObjectModuleResult` after target evidence is known.

## Watchouts

- Do not implement RV64 capability repairs under this diagnostics idea.
- Do not define shared BIR/prepared semantics inside RV64 object emission.
- Do not use testcase-name matching or expectation downgrades as progress.
- Minimal diagnostic insertion points are the object-route handoff in
  `backend.cpp`, the diagnostic-capable wrapper in `object_emission.hpp/cpp`,
  and the existing empty-result gates in
  `build_rv64_prepared_text_object_module_with_diagnostics` /
  `prepared_function_to_object_function`; avoid adding classifiers inside
  low-level encoders unless the caller cannot name the failing target bucket.
- Prefer reusing shared category names/messages when
  `prepared_consumer_category` is present. Add RV64-specific buckets only for
  target evidence such as `module_string_constants`, `unsupported_global_data`,
  `unsupported_function_admission`, `unsupported_param_home`,
  `unsupported_stack_frame`, `unsupported_move_bundle_target_shape`,
  `unsupported_instruction_fragment`, `unsupported_terminator_fragment`,
  `unsupported_local_memory_width_or_addressing`, and
  `object_module_or_elf_build_failed`.

## Proof

Audit-only packet; no build required by delegation. Read-only inspection used
`rg`, targeted `sed` reads, and `c4c-clang-tool-ccdb function-callees` for
`emit_rv64_prepared_object_module`,
`build_rv64_prepared_text_object_module_with_diagnostics`, and
`prepared_function_to_object_function`. No `test_after.log` was produced
because the delegated proof explicitly required no build.

# RV64 Object Emission Staged Follow-Ups

Source idea: `ideas/open/519_rv64_object_emission_cleanup_umbrella.md`
Plan step: Step 3 - Draft Staged Cleanup Follow-Ups

This is an analysis-only staged cleanup list. No implementation files, tests,
expectations, allowlists, unsupported markers, runtime comparisons, or
root-level logs were changed.

## Evidence Sources

- `docs/rv64_object_emission_cleanup/structure_baseline.md`
- `docs/rv64_object_emission_cleanup/aarch64_comparison.md`
- `plan.md`
- `tests/backend/CMakeLists.txt`
- `tests/backend/mir/CMakeLists.txt`

The ordering below follows the Step 1 region map and Step 2 owner comparison.
It intentionally peels low-risk leaf helpers before moving prepared traversal,
dispatch, symbol/fixup, or final module assembly boundaries.

## Global Guardrails

Every follow-up source idea materialized from this list should keep these
constraints:

- Behavior-preserving only: no RV64 capability repair, no gcc_torture
  expectation changes, no unsupported marker changes, and no target-side
  inference for missing prepared facts.
- Preserve public object route APIs in
  `src/backend/mir/riscv/codegen/object_emission.hpp` unless a later idea is
  explicitly scoped as an API-preserving wrapper migration.
- Keep `prepared_function_to_object_function`,
  `fragment_for_prepared_instruction`, symbol/fixup module assembly, and
  prepared data-object section assembly central until the listed prerequisites
  are complete.
- Prefer compiled `prepared_*_emit.*` and `rv64_line_assembler.*` owners over
  legacy RV64 layout files unless a follow-up first proves or creates live
  ownership for `calls.cpp`, `memory.cpp`, `globals.cpp`, `returns.cpp`,
  `prologue.cpp`, or `variadic.cpp`.
- Validation for implementation slices should include a build step and a
  focused CTest filter. Escalate to broader backend or runtime proof when the
  moved helper can affect emitted object bytes, relocations, public
  entrypoints, or executable behavior.

## Ordered Slices

### 1. Shared RV64 Encoding And Byte Append Helpers

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`

Scope:

- Move or share pure U/I/S/R/B/J encoding helpers and endian byte append
  helpers only when the APIs remain independent of prepared BIR, object module
  assembly, labels, and fixup attachment.
- Preserve `RiscvEncodedFragment` structure and object-route callers.

Prerequisites/dependencies:

- Confirm duplicate helper signatures between `object_emission.cpp` and
  `rv64_line_assembler.cpp`.
- Keep object-fragment labels and fixups in the object route unless a helper
  API is explicitly structured around fragments rather than text-line parsing.

Risks:

- Overcoupling `rv64_line_assembler` to object-module semantics.
- Accidentally changing instruction bytes for branch, JAL, AUIPC, LO12, or
  inline asm emission.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_object_emission|rv64_roundtrip_contract|cli_riscv64_return_zero_writes_elf_obj|cli_riscv64_vrm_insn_d_source_obj)'`.
- Add one affected runtime object filter if branch or call word emission is
  touched.

Reviewer reject signals:

- The slice moves symbol kind mapping, relocation mapping, label binding, or
  object module assembly.
- The slice routes object emission through text parsing.
- Object bytes change without a separately approved capability or encoding fix.

### 2. Pure Frame, Stack Offset, And Basic Stack Move Helpers

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`

Scope:

- Extract pure frame sizing, stack-slot offset, register-home lookup, basic
  stack load/store, and simple stack adjustment helpers that do not need
  object-module symbols or prepared instruction dispatch.

Prerequisites/dependencies:

- Slice 1 is not strictly required, but helper signatures should avoid copying
  low-level encoders if Slice 1 is active.
- Keep call-specific byval/sret argument publication and before-return bundles
  out of this slice.

Risks:

- Mixing frame math with call lowering or local memory semantics.
- Weakening prepared stack-layout diagnostics.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_object_emission|dump_riscv64_stack_passed_parameter_home_publication|obj_runtime_rv64_large_fixed_frame_slot_access|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)'`.

Reviewer reject signals:

- The slice changes stack-frame size, alignment, formal-entry home meaning, or
  unsupported diagnostics.
- The slice moves `prepared_function_to_object_function` or broad function
  traversal.

### 3. Prepared Module Admission And Diagnostics Shell

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.hpp`

Scope:

- Factor object-route prepared module admission and diagnostic construction
  only as a shell around existing function conversion and data-object assembly.
- Preserve `build_rv64_prepared_text_object_module_with_diagnostics` and
  `write_rv64_prepared_relocatable_elf_object_with_diagnostics` observable
  behavior.

Prerequisites/dependencies:

- Can follow Slice 2, but must not require moving function conversion.
- Keep parallel-copy diagnostics, prepared object consumer categories, and
  public entrypoint wrappers semantically identical.

Risks:

- Treating text-route `prepared_module_emit.cpp` as a drop-in object owner.
- Dropping diagnostic category detail or replacing rejection text with generic
  unsupported results.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(prepared_object_consumer_contract|riscv_object_emission|object_model_records|cli_riscv64_variadic_entry_missing_contract_obj|cli_riscv64_unsupported_global_diagnostic_obj)'`.

Reviewer reject signals:

- The slice infers missing prepared facts from BIR, target text, or object
  output.
- The slice changes pass/fail accounting or unsupported expectations.

### 4. Local Memory And Pointer-Value Access Helpers

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`

Scope:

- Extract local frame-slot load/store, local pointer materialization, and
  pointer-value base-plus-offset helper families after the frame helper
  boundary is stable.
- Keep global symbol materialization and prepared data-object section emission
  out of scope.

Prerequisites/dependencies:

- Slice 2 should land first so frame offsets and stack helper ownership are
  not duplicated.
- Address helper APIs must remain structured around prepared access facts, not
  new BIR inference.

Risks:

- Blending local memory with global symbol materialization.
- Changing local-array semantic GEP behavior while only claiming a movement
  slice.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_object_emission|codegen_route_riscv64_prepared_local_array|obj_runtime_rv64_local_temp|obj_runtime_rv64_large_fixed_frame_slot_access|rv64_runtime_riscv64_pointer_to_pointer_local_address)'`.

Reviewer reject signals:

- The slice rewrites local-array semantics, pointer provenance, or prepared
  memory facts.
- The slice adjusts gcc_torture expectations or unsupported markers.

### 5. Global Address And Direct Symbol Materialization Helpers

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp`

Scope:

- Extract global load/store and direct symbol/address materialization helpers
  that are still per-instruction fragments.
- Keep final `.rodata`, `.data`, `.bss`, global object symbol definition, and
  data relocation assembly in the late object-data slice.

Prerequisites/dependencies:

- Slice 1 should land first if symbol-address fragments use shared encoders.
- Slice 4 should separate local access from global/address helpers.

Risks:

- Confusing text-route global storage assembly with object data sections.
- Moving AUIPC/LO12 label-pair fixup contracts before final module assembly
  has a stable interface.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_object_emission|codegen_route_riscv64_aggregate_global|codegen_route_riscv64_global_store|obj_runtime_rv64_global_store_prepared_value_preserves_source|rv64_runtime_global_load)'`.

Reviewer reject signals:

- The slice moves `append_rv64_prepared_data_objects` or object section
  selection.
- The slice changes symbol visibility, relocation targets, or link-name
  spelling.

### 6. Scalar Arithmetic, Casts, Compare Branches, And Simple Returns

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`

Scope:

- Extract scalar binary, cast, compare-branch, move-to-register/location, and
  simple return helpers.
- Keep select-edge publication, before-return move bundles, and full
  terminator dispatch central unless explicitly narrowed.

Prerequisites/dependencies:

- Slices 1 and 2 should land first to avoid duplicating encoders and stack
  helpers.
- Local/global helper slices should define how address-producing scalar-like
  cases are routed.

Risks:

- A catch-all scalar move that hides select/publication and BIR producer
  coupling.
- Changing compare predicate normalization or branch-label fixup behavior.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_object_emission|dump_riscv64_prepared_fused_compare|codegen_route_riscv64_prepared_fused_compare|obj_runtime_rv64_return_add|obj_runtime_rv64_return_add_sub_chain)'`.

Reviewer reject signals:

- The slice rewrites select publication or prepared edge movement while named
  as scalar-only.
- The slice changes emitted branch behavior or diagnostics.

### 7. Select And Edge-Publication Object Helpers

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`

Scope:

- Extract selected select-source, publication move, and predecessor-edge helper
  fragments after scalar and local/global helper APIs are stable.

Prerequisites/dependencies:

- Slices 4, 5, and 6 should land first.
- Prepared publication-plan and consumer-classifier coupling must remain
  visible in APIs instead of hidden behind a generic helper.

Risks:

- Overfitting to a known select testcase rather than preserving generic
  prepared publication behavior.
- Losing diagnostic distinctions for selected source producer fragments.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_prepared_edge_publication|dump_riscv64_.*select|codegen_route_riscv64_.*select|rv64_runtime_riscv64_pointer_typed_select_publication|rv64_runtime_riscv64_short_circuit_select_false_lhs)'`.

Reviewer reject signals:

- The slice changes prepared publication facts, testcase expectations, or
  fallback behavior.
- The slice hides `fragment_for_prepared_instruction` fanout behind another
  monolithic dispatch file.

### 8. Call, Variadic, Prologue, And Return Object Fragments

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`
- possible new compiled RV64 object helper files if `variadic.cpp`,
  `prologue.cpp`, or `returns.cpp` are not revived first

Scope:

- Split object-route call fragments, variadic fragments, prologue/epilogue
  fragments, and return fragments only after lower-level frame/scalar/memory
  helpers have stable APIs.
- Treat legacy `calls.cpp`, `variadic.cpp`, `prologue.cpp`, and `returns.cpp`
  as references unless the follow-up explicitly creates live compiled
  ownership.

Prerequisites/dependencies:

- Slices 1, 2, and 6 should land first; Slices 4 and 5 may be required for
  byval/sret/address arguments.
- Call preservation and variadic helper resource authority must remain prepared
  fact consumers.

Risks:

- Mixing behavior-preserving movement with new call or variadic capability
  repair.
- Changing saved-register, sret, byval, or local-argument object runtime
  behavior.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_object_emission|codegen_route_riscv64_byval|dump_riscv64_byval|obj_runtime_rv64_local_arg_call|obj_runtime_rv64_callee_saved_gpr_live_across_call|cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)'`.

Reviewer reject signals:

- The slice revives legacy owner files without CMake/build ownership.
- The slice changes variadic admission, call-boundary effects, preserved
  register behavior, or runtime expectations.

### 9. Prepared Object Function Traversal Facade

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.hpp`
- possible new compiled object-route function facade files

Scope:

- After family helpers are peeled out, create a small object-function traversal
  facade around `prepared_function_to_object_function` without changing its
  admission semantics or diagnostics.
- Keep `fragment_for_prepared_instruction` central unless the facade delegates
  to already-extracted family APIs.

Prerequisites/dependencies:

- Slices 2 through 8 should be complete or explicitly waived by a reviewer.
- The facade must expose prepared lookup/context dependencies instead of
  burying them in a new all-purpose object file.

Risks:

- Moving the central high-coupling anchor too early.
- Creating a second monolith under a new name.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  '^backend_(riscv_object_emission|prepared_object_consumer_contract|object_model_records|obj_runtime_rv64_|codegen_route_riscv64_)'`.
- Consider full `^backend_` proof if this touches traversal order or
  diagnostic aggregation.

Reviewer reject signals:

- The diff is mostly mechanical relocation but leaves the same hidden coupling.
- Diagnostics, block traversal order, or function admission results change.

### 10. Object Data, Symbol/Fixup, And Final Module Assembly

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- possible new compiled object-data or object-module assembly files

Scope:

- Move late/central boundaries only after fragment producers have stable fixup
  contracts: `build_rv64_text_object_module`, relocation mapping,
  local-label binding, undefined symbol declaration,
  `append_rv64_prepared_data_objects`, object section selection, and public
  ELF entrypoints.

Prerequisites/dependencies:

- Earlier slices should leave producers with clear fragment and fixup
  contracts.
- Data-object relocation and text relocation ownership should be reviewed as
  separate sub-slices unless one tiny owner can preserve both contracts.

Risks:

- Highest-risk behavior boundary: object bytes, relocations, ELF flags, symbol
  bindings, section alignment, zero-fill reservation, and public entrypoints.
- Merging instruction fixups with data-object pointer relocations too early.

Validation expectation:

- Build plus `ctest --test-dir build -j --output-on-failure -R
  'backend_(riscv_object_emission|object_model_records|cli_riscv64_.*obj|obj_runtime_rv64_|rv64_roundtrip_contract)'`.
- Escalate to broader `^backend_` proof before accepting a slice that changes
  object module layout, relocation attachment, section emission, or ELF
  writing.

Reviewer reject signals:

- The slice changes object bytes, relocation entries, symbol binding, section
  names, section alignment, or ELF flags while claiming behavior preservation.
- The slice combines final module assembly with semantic RV64 capability
  repair or gcc_torture expectation updates.

## Step 4 Materialization Guidance

Step 4 should create one or more `ideas/open/*.md` source ideas from the
ordered slices above. Good first materialization candidates are:

1. one narrow low-risk idea for Slice 1 or Slice 2;
2. one mid-risk grouped idea covering Slices 4 and 5 only if it keeps local
   memory, global address materialization, and object data sections separated;
3. one explicit late-boundary idea for Slice 10 that documents why
   symbol/fixup/module assembly stays central until prerequisites land.

Each created idea should include owned files, prerequisites, behavior-preserving
scope, validation commands, and reviewer reject signals copied or narrowed from
this artifact. Do not create an idea that mixes implementation movement with
RV64 capability repair or gcc_torture expectation changes.

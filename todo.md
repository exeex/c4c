Status: Active
Source Idea Path: ideas/open/347_rv64_inline_asm_custom_vector_integration_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Closed Child Evidence

# Current Packet

## Just Finished

Step 1 - Map Closed Child Evidence completed by inspection. The parent umbrella
criteria have matching closed-child evidence and current test/source surfaces:

- Stage 1 standard scalar `.insn`: closed by
  `ideas/closed/346_rv64_standard_insn_scalar_inline_asm_object_route.md`.
  Evidence covers structured `.insn r opcode, funct3, funct7, rd, rs1, rs2`
  metadata through HIR/LIR/BIR, scalar `r`/`=r`/`+r` and tied operands,
  side-effect preservation, malformed-form diagnostics, and c4c-owned object
  bytes. Current tests/surfaces:
  `tests/frontend/frontend_hir_tests.cpp::test_inline_asm_insn_r_structured_metadata_and_diagnostics`,
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp::inline_asm_lir_lowering_preserves_structured_operand_metadata`,
  and `tests/backend/mir/backend_riscv_object_emission_test.cpp` fixtures
  `builds_prepared_inline_asm_insn_r_object`,
  `builds_structured_prepared_inline_asm_insn_r_object_without_text_reparse`,
  `builds_prepared_inline_asm_insn_r_tied_input_object`,
  `builds_structured_prepared_inline_asm_insn_r_readwrite_object`, plus the
  nearby `rejects_prepared_inline_asm_insn_r_*` negative cases.
- Stage 2 vector register constraints: closed by
  `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`.
  Evidence covers `VR`, `VRM2`, and `VRM4` classification as vector register
  classes; width/alignment occupancy; legal base allocation; non-overlap for
  untied operands; tied/read-write reuse; base-register substitution; and
  impossible or incompatible carrier diagnostics. Current tests/surfaces:
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp::inline_asm_lir_lowering_preserves_structured_operand_metadata`
  for constraint classification, `tests/backend/bir/backend_prepare_liveness_test.cpp`
  fixtures `check_vector_grouped_linear_scan`,
  `check_rv64_inline_asm_vector_group_allocation`,
  `check_rv64_inline_asm_vector_impossible_allocation`, and
  `check_rv64_inline_asm_scalar_vrm2_negative`, plus
  `tests/backend/bir/backend_prepared_printer_test.cpp` fixtures
  `rv64_inline_asm_vector_carrier_contract` and
  `rv64_inline_asm_impossible_vector_allocation_diagnoses_missing_home`.
  RV64 substitution is also covered in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` by
  `substitutes_prepared_rv64_vector_inline_asm_base_registers`,
  `substitutes_prepared_rv64_mixed_scalar_vector_inline_asm_registers`, and
  `substitutes_prepared_rv64_tied_vector_inline_asm_base_register`.
- Stage 3 EV 64-bit `.insn.d`: closed by
  `ideas/closed/342_rv64_ev_insn_d_inline_asm_stage3.md`. Evidence covers the
  positional `.insn.d %4, %5, %0, %1, %2, %3, %6` shape, prepared compile-time
  immediate fields, scalar/vector register operands using existing allocation
  and substitution, deterministic 64-bit encoding, object emission without an
  external assembler, and negative coverage for malformed fields, missing or
  non-constant immediates, unsupported operand classes, named operands, and
  `%c` template modifiers. Current tests/surfaces:
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` fixtures
  `classifies_prepared_inline_asm_insn_d_positional_shape`,
  `encodes_prepared_inline_asm_insn_d_positional_shape`,
  `builds_prepared_inline_asm_insn_d_object`,
  `builds_prepared_inline_asm_insn_d_adjacent_template_object`,
  `builds_prepared_inline_asm_insn_d_helper_template_object`, and
  `rejects_prepared_inline_asm_insn_d_*`.
- Final consteval/template string route: closed by
  `ideas/closed/343_rv64_consteval_inline_asm_template_strings.md`. Evidence
  covers literal templates remaining unchanged, accepted compile-time string
  expressions folding to the same final inline asm template text, preservation
  of constraints/operands/volatility/clobbers, helper-style `.insn.d` object
  bytes matching the literal route, and runtime template rejection. Current
  tests/surfaces:
  `tests/frontend/frontend_hir_tests.cpp::test_inline_asm_string_literal_plus_folds_to_literal_metadata`,
  `tests/frontend/frontend_hir_tests.cpp::test_inline_asm_insn_d_string_literal_plus_folds_to_literal_metadata`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp::builds_prepared_inline_asm_insn_d_helper_template_object`,
  and `tests/cpp/internal/negative_case/bad_inline_asm_runtime_template.cpp`
  via CTest `cpp_negative_tests_bad_inline_asm_runtime_template`.

No concrete missing umbrella criterion or stale child dependency was found in
Step 1. Step 2 can proceed to verify the composed route fixtures directly.

## Suggested Next

Proceed with Step 2 - Verify Composed Route Fixtures. The next packet should
inspect/prove the representative literal `.insn.d`, compile-time-generated
`.insn.d`, `VRM*` operand, and object-byte route, with special attention to
whether the consteval/helper path and literal path share the same lowering,
metadata, substitution, and RV64 object emission surfaces.

## Watchouts

- Keep this child focused on review/proof of composition.
- Do not reopen completed child implementations unless a concrete integration
  failure is found.
- If a required capability is missing, create or request a focused follow-up
  child idea instead of expanding this review runbook.
- The parent umbrella must remain open until this child closes and the parent
  close review accepts it.
- The umbrella examples mention GNU named operands and `%c[...]` modifiers, but
  the closed Stage 3 and this integration child both keep that surface out of
  scope. Treat it as a follow-up only if Step 2 proves it is required for the
  already-accepted umbrella criteria.
- The Stage 1 closed child intentionally excludes FPR constraints; the parent
  wording made `f`/`=f` conditional on existing RV64 FPR readiness, so this was
  not classified as a missing criterion for the scalar custom-vector route.

## Proof

Inspection-only packet. No build or CTest run was required for Step 1.

Commands used for evidence lookup:

```bash
git status --short
sed -n '1,260p' plan.md
sed -n '1,260p' todo.md
sed -n '1,260p' ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md
sed -n '1,260p' ideas/open/347_rv64_inline_asm_custom_vector_integration_review.md
sed -n '1,300p' ideas/closed/346_rv64_standard_insn_scalar_inline_asm_object_route.md
sed -n '1,300p' ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md
sed -n '1,340p' ideas/closed/342_rv64_ev_insn_d_inline_asm_stage3.md
sed -n '1,340p' ideas/closed/343_rv64_consteval_inline_asm_template_strings.md
rg -n "insn_d|insn_r|VRM2|VRM4|VR\\b|inline_asm_template|runtime_template|consteval.*asm|asm template" tests/frontend tests/backend tests/cpp/internal -g '*.cpp' -g 'CMakeLists.txt'
sed -n '7460,7710p' tests/frontend/frontend_hir_tests.cpp
sed -n '1740,2275p' tests/backend/mir/backend_riscv_object_emission_test.cpp
sed -n '2300,2770p' tests/backend/mir/backend_riscv_object_emission_test.cpp
sed -n '3560,3770p' tests/backend/bir/backend_lir_to_bir_notes_test.cpp
sed -n '7040,7285p' tests/backend/bir/backend_prepare_liveness_test.cpp
sed -n '3760,3925p' tests/backend/bir/backend_prepared_printer_test.cpp
sed -n '4290,4388p' tests/backend/bir/backend_prepared_printer_test.cpp
sed -n '1,80p' tests/cpp/internal/negative_case/bad_inline_asm_runtime_template.cpp
sed -n '1,38p' tests/cpp/internal/negative_case/CMakeLists.txt
```

Proof log path: none for this inspection-only packet; `test_after.log` was not
modified.

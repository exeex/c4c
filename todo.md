Status: Active
Source Idea Path: ideas/open/347_rv64_inline_asm_custom_vector_integration_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify Composed Route Fixtures

# Current Packet

## Just Finished

Step 2 - Verify Composed Route Fixtures completed. Existing tests already prove
the composed route; no new fixture or implementation change was needed.

Composition evidence:

- Compile-time/helper template to inline asm metadata:
  `tests/frontend/frontend_hir_tests.cpp::test_inline_asm_insn_d_string_literal_plus_folds_to_literal_metadata`
  compares literal `.insn.d %4, %5, %0, %1, %2, %3, %6` with helper-style
  string-literal `+` assembly and requires identical final template text,
  constraints, output count, and input count in HIR inline asm metadata.
  `test_inline_asm_string_literal_plus_folds_to_literal_metadata` separately
  proves folded templates preserve ordinary inline asm constraints,
  volatility/side-effect metadata, clobbers, and operand counts.
- Runtime template rejection:
  `tests/cpp/internal/negative_case/bad_inline_asm_runtime_template.cpp`, run as
  CTest `cpp_negative_tests_bad_inline_asm_runtime_template`, rejects runtime
  asm template expressions before lowering, so the composed route stays
  compile-time-only.
- BIR inline asm operand metadata:
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp::inline_asm_lir_lowering_preserves_structured_operand_metadata`
  proves inline asm constraints lower into structured operand facts. Its RV64
  vector fixture classifies `VR`, `=VR`, `+VR`, `VRM2`, `=VRM2`, `+VRM2`,
  `VRM4`, `=VRM4`, and `+VRM4` as vector operands with the expected group
  widths while unsupported vector-looking constraints remain explicit facts.
- Vector group allocation and tied/overlap semantics:
  `tests/backend/bir/backend_prepare_liveness_test.cpp` fixtures
  `check_rv64_inline_asm_vector_group_allocation`,
  `check_rv64_inline_asm_vector_impossible_allocation`, and
  `check_rv64_inline_asm_scalar_vrm2_negative` prove `VRM2`/`VRM4` group widths
  seed regalloc, legal aligned caller/callee spans are used, untied groups do
  not overlap, read-write/tied operands keep grouped vector homes, impossible
  pressure reports the missing home path, and scalar values cannot satisfy
  vector-register carrier compatibility.
- Prepared carrier completion:
  `tests/backend/bir/backend_prepared_printer_test.cpp` fixtures around
  `rv64_inline_asm_vector_carrier_contract` prove prepared inline asm carriers
  carry vector home facts for `VRM2`, `=VRM4`, and tied `=VRM2,0` operands,
  including shared vector tied-home authority without incompatible-register
  diagnostics.
- RV64 substitution and object bytes:
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` proves grouped
  vector placeholders substitute the base register with
  `substitutes_prepared_rv64_vector_inline_asm_base_registers`,
  `substitutes_prepared_rv64_mixed_scalar_vector_inline_asm_registers`, and
  `substitutes_prepared_rv64_tied_vector_inline_asm_base_register`. The same
  file then proves `.insn.d` classification, field placement, exact 64-bit
  encoding, and object bytes through c4c's RV64 object route with
  `classifies_prepared_inline_asm_insn_d_positional_shape`,
  `encodes_prepared_inline_asm_insn_d_positional_shape`,
  `builds_prepared_inline_asm_insn_d_object`, and
  `builds_prepared_inline_asm_insn_d_helper_template_object`. The helper-style
  object fixture uses the same final template text shape as the frontend
  helper-folding test and the same `=VRM2,VRM2,VRM2,VRM2,i,i,i` prepared
  carrier shape as the literal `.insn.d` object fixture.

No missing composed-route fixture was found for Step 2. The current proof shows
literal and compile-time/helper `.insn.d` forms converge on the same final
template text and metadata, then the prepared RV64 route applies `VRM2` carrier
facts, base-register substitution, 64-bit EV field encoding, and object-byte
emission.

## Suggested Next

Proceed with Step 3 - Run Closure-Quality Validation. The supervisor should
select the broader validation/guard command and decide whether to compare
`test_before.log` and `test_after.log` now or capture a wider closure-quality
baseline first.

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
- There is not one monolithic source-to-object integration test for
  consteval/helper `.insn.d`; instead, the proof is compositional across
  frontend folded-template equality and backend prepared-carrier object-byte
  equality. This matches the review packet and did not expose a semantic gap.

## Proof

Proof command run exactly as delegated:

```bash
bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|backend_lir_to_bir_notes|backend_prepared_printer|backend_prealloc_inline_asm|backend_prepare_liveness|backend_riscv_object_emission|cpp_negative_tests_bad_inline_asm_runtime_template)$" >> test_after.log 2>&1'
```

Result: PASS. `test_after.log` reports `100% tests passed, 0 tests failed out
of 7`.

Tests run:

- `frontend_hir_tests`
- `backend_lir_to_bir_notes`
- `backend_prepared_printer`
- `backend_prealloc_inline_asm`
- `backend_prepare_liveness`
- `backend_riscv_object_emission`
- `cpp_negative_tests_bad_inline_asm_runtime_template`

Proof log path: `test_after.log`.

Supervisor acceptance:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. The focused seven-test composed-route proof had no
passed/failed/total delta and no new failures.

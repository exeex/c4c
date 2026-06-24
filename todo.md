Status: Active
Source Idea Path: ideas/open/347_rv64_inline_asm_custom_vector_integration_review.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Run Closure-Quality Validation

# Current Packet

## Just Finished

Step 3 - Run Closure-Quality Validation completed. The supervisor-selected
broader validation subset passed with no failures or blockers.

The proof covered:

- frontend HIR inline asm and folded template metadata,
- BIR inline asm metadata and vector constraint classification,
- prepared inline asm carriers, prealloc, and liveness,
- RV64 `.insn`/`.insn.d` object emission,
- RV64 backend route and runtime coverage selected by the supervisor,
- x86 inline asm backend route smoke coverage,
- consteval/deferred-consteval HIR coverage,
- runtime inline asm template rejection, and
- GNU asm adjacent string parse coverage.

No failure needed blocker classification. No separate follow-up gap was found
by this validation packet.

## Suggested Next

Proceed with Step 4 - Decide Parent Umbrella Readiness. The supervisor should
run any desired regression-log comparison against `test_before.log` and
`test_after.log`, then route to plan-owner close review if the broader
validation is accepted.

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
- This executor packet did not run the regression guard because the delegated
  contract only specified the matching broader proof command. The matching
  baseline and proof logs are present as `test_before.log` and `test_after.log`
  for supervisor-side comparison.

## Proof

Proof command run exactly as delegated:

```bash
bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|backend_lir_to_bir_notes|backend_prepared_printer|backend_prealloc_inline_asm|backend_prepare_liveness|backend_riscv_object_emission|backend_codegen_route_riscv64_.*|backend_rv64_runtime_riscv64_.*|backend_codegen_route_x86_64_inline_asm_.*|cpp_negative_tests_bad_inline_asm_runtime_template|cpp_hir_consteval_.*|cpp_hir_deferred_consteval_.*|cpp_parse_gnu_asm_adjacent_string_template_dump)$" >> test_after.log 2>&1'
```

Result: PASS. `test_after.log` reports `100% tests passed, 0 tests failed out
of 80`.

Representative test groups run:

- `frontend_hir_tests`
- `backend_lir_to_bir_notes`
- `backend_prepared_printer`
- `backend_prealloc_inline_asm`
- `backend_prepare_liveness`
- `backend_riscv_object_emission`
- `backend_codegen_route_riscv64_.*`
- `backend_rv64_runtime_riscv64_.*`
- `backend_codegen_route_x86_64_inline_asm_.*`
- `cpp_negative_tests_bad_inline_asm_runtime_template`
- `cpp_hir_consteval_.*`
- `cpp_hir_deferred_consteval_.*`
- `cpp_parse_gnu_asm_adjacent_string_template_dump`

Proof log path: `test_after.log`.

Supervisor acceptance:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. The 80-test broader proof had no passed/failed/total delta and
no new failures.

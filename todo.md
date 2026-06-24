Status: Active
Source Idea Path: ideas/open/347_rv64_inline_asm_custom_vector_integration_review.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Decide Parent Umbrella Readiness

# Current Packet

## Just Finished

Step 4 - Decide Parent Umbrella Readiness completed. The integration review
child is ready for plan-owner close review, and the parent umbrella is ready
for close review after this child is closed.

Readiness decision:

- Step 1 mapped every parent umbrella stage and completion criterion to closed
  child evidence, current tests, or explicit out-of-scope boundaries. No
  required missing criterion or stale child dependency was found.
- Step 2 found the existing composed-route fixtures sufficient: literal and
  compile-time/helper `.insn.d` templates converge on the same final inline asm
  metadata, then the prepared RV64 route exercises `VRM2` carrier facts,
  base-register substitution, EV 64-bit encoding, and object-byte emission.
  The focused seven-test proof passed, and supervisor regression guard accepted
  the matching logs.
- Step 3 broader closure-quality validation passed 80 tests across frontend
  inline asm, consteval/deferred consteval, BIR/prealloc/vector carriers, RV64
  object emission, RV64 route/runtime coverage, and x86 inline asm route smoke
  coverage. Supervisor regression guard accepted the matching
  `test_before.log` / `test_after.log` logs.

No concrete focused follow-up child idea is required for the already-stated
umbrella acceptance criteria. Optional future work such as GNU named operands,
`%c[...]` modifiers, mask-specific constraints, broader GNU assembler
compatibility, or FPR `.insn` constraints remains out of scope unless the
parent close review chooses to queue it separately.

## Suggested Next

Route to plan-owner close review for this integration child. After this child
closes, route the parent umbrella
`ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md` to
plan-owner close review.

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
- Do not close the parent umbrella before the plan-owner has closed this active
  integration child.

## Proof

No new tests were required for Step 4. Readiness is based on the accepted
Step 1 evidence map, Step 2 focused proof, and Step 3 broader proof.

Latest broader proof command already recorded:

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

Step 4 blocker classification: none.

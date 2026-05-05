# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed the invalid aggregate/global initializer
shape failures in `c_testsuite_src_00216_c` and
`llvm_gcc_c_torture_src_20040709_{1,2,3}_c`. The shared TypeSpec aggregate owner
normalizer now rejects direct owner keys whose indexed rendered tag disagrees
with the TypeSpec's available final structured spelling, then canonicalizes via
the HIR owner index before constant-initializer layout lookup. This keeps
`%struct.SS` and bitfield-containing `%struct.C` globals paired with their own
layouts instead of colliding with unrelated records after cross-table TextId
drift. No tests were weakened and no rendered-tag consumer fallback was added.

## Suggested Next

Continue Step 6 against the remaining delegated-subset C failure:
`llvm_gcc_c_torture_src_20040703_1_c`, which now remains a runtime abort rather
than an invalid initializer/no-binary failure.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- The fixed mirror failures were producer-side identity mismatches. Keep future
  call/GEP aggregate work on normalized `StructNameId` production; do not relax
  `verify.cpp` mirror/text checks to make the remaining parity failures pass.
- The current delegated GEP layout parity leftovers are cleared. The remaining
  observed failures in this proof subset are C-side constant-initializer or
  runtime-semantics failures, not structured layout parity mismatches.
- `00216.c` now compiles and runs in the delegated proof; the prior `@sinit16`
  invalid `%struct.SS` initializer shape is fixed.
- `20040709-{1,2,3}.c` now compile and run in the delegated proof; the prior
  bitfield global initializer type mismatches such as `@sC` are fixed.
- `20040703-1.c` still compiles to a runnable binary but aborts at runtime; that
  is a semantic mismatch, not a no-output/no-binary failure.
- The repaired timeout came from open-coded injected reparses of dependent
  template owner prefixes. Future injected parser routes should use
  `parse_injected_base_type()` or otherwise include an EOF token and restore
  the full parser token stream.
- Rejected baseline candidate: accepting the visible lexical alias
  type-argument failure as baseline drift is not valid because
  `frontend_parser_tests` contains the intended non-fabrication guard and the
  regression is local to parser TypeSpec metadata production.
- Keep idea 141 parked until normalized identity boundaries are stable.
- Do not close idea 143 while full ctest still has failures.

## Proof

Current accepted proof is in `test_before.log`:
`cmake --build build --target c4cll && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_src_00216_c|llvm_gcc_c_torture_src_20040709_(1|2|3)_c|llvm_gcc_c_torture_src_20040703_1_c|llvm_gcc_c_torture_src_const_addr_expr_1_c)$'`.

The delegated proof now passes 5/6. Passing tests:
`c_testsuite_src_00216_c`, `llvm_gcc_c_torture_src_20040709_{1,2,3}_c`, and
`llvm_gcc_c_torture_src_const_addr_expr_1_c`. Remaining failure:
`llvm_gcc_c_torture_src_20040703_1_c` as `[RUNTIME_FAIL] ... c2ll_exit=Subprocess aborted`.

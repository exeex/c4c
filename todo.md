# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed one real C runtime/no-output failure in
`llvm_gcc_c_torture_src_const_addr_expr_1_c`. Global constant initializers for
`&(array + n)->field` and `&array->field` now resolve field slots through the
array element aggregate identity instead of asking the structured-layout lookup
to resolve the whole array type. This emits real `getelementptr` initializers
for `Upgd_minor_ID` and `Upgd_minor_ID1` instead of `ptr null`, eliminating the
segfault. No tests were weakened and no rendered-tag consumer fallback was
added.

Accumulated Step 6 review in
`review/143_step6_accumulated_identity_review.md` found no blocking findings,
judged the route on track, and recommended continuing the current Step 6 route.
The accepted focused proof was rolled forward to `test_before.log`.

## Suggested Next

Continue Step 6 against the remaining C external-suite failures in the
delegated subset, starting with invalid aggregate initializer shape in
`00216.c` or `20040709-{1,2,3}.c`; both still surface as no-output/no-binary
harness failures after clang rejects the generated IR.

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
- `00216.c` still emits invalid aggregate initializer shape around `@sinit16`;
  manual clang reports "initializer with struct type has wrong # elements".
- `20040709-{1,2,3}.c` still emit invalid global initializer shape for some
  bitfield-containing structs; manual clang on `20040709-1.c` reports
  "element 0 of struct initializer doesn't match struct element type" at `@sC`.
- `20040703-1.c` now compiles to a runnable binary but aborts at runtime; that
  is a semantic mismatch, not a no-output failure.
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

The delegated proof now passes 1/6. Passing test:
`llvm_gcc_c_torture_src_const_addr_expr_1_c`. Remaining failures:
`c_testsuite_src_00216_c` as `[RUNTIME_NONZERO] ... exit=no such file or
directory`, `llvm_gcc_c_torture_src_20040709_{1,2,3}_c` as `[RUNTIME_FAIL] ...
c2ll_exit=no such file or directory`, and
`llvm_gcc_c_torture_src_20040703_1_c` as `[RUNTIME_FAIL] ...
c2ll_exit=Subprocess aborted`.

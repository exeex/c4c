# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed the delegated C/LIR aggregate identity
failures where structured mirrors disagreed with rendered text. Call aggregate
return mirrors now reuse the cross-table-aware `typespec_aggregate_owner_key`
path and normalize the selected `StructNameId` against the rendered LLVM type
before attaching it. Indexed/member GEP aggregate mirrors now normalize stale
carried ids the same way. No tests were weakened and no rendered-tag consumer
fallback was added.

## Suggested Next

Continue Step 6 against the remaining C external-suite families exposed after
identity normalization: structured layout parity mismatches for `%struct.Wrap`
and `%struct.lock_chain`, plus the `20040709-{1,2,3}.c` runtime failures that
now get past LIR call-return mirror verification.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- The fixed mirror failures were producer-side identity mismatches. Keep future
  call/GEP aggregate work on normalized `StructNameId` production; do not relax
  `verify.cpp` mirror/text checks to make the remaining parity failures pass.
- The current GEP leftovers are no longer mirror/text disagreements. They are
  layout identity/parity questions: `%struct.Wrap` has legacy size 0/fields []
  vs structured fields [ptr], and `%struct.lock_chain` has legacy [i8, i16] vs
  structured [i32].
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

Accepted proof is in `test_after.log`:
`cmake --build build --target c4cll && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_src_00216_c|llvm_gcc_c_torture_src_20040709_(1|2|3)_c|llvm_gcc_c_torture_src_pr71083_c)$'`.

The delegated subset still fails 5/5, but the failure family changed as
intended. The original `LirCallOp.return_type` mirror/text failures for
`20040709-{1,2,3}.c` are fixed and those tests now reach `RUNTIME_FAIL`
(`c2ll_exit=no such file or directory`). The original `LirGepOp.element_type`
mirror/text failures for `00216.c` and `pr71083.c` are fixed and now fail as
`LirStructuredLayoutObservation.parity` mismatches for `%struct.Wrap` and
`%struct.lock_chain`.

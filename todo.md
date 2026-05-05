# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed a real structured layout parity mismatch
by constraining legacy HIR layout selection to the normalized LIR aggregate
identity. `lookup_structured_layout` now replaces a stale owner/compatibility
legacy declaration when it disagrees with the already-normalized
`StructNameId`, which fixes the `%struct.lock_chain` case that was pairing the
structured `[i32]` layout with `lock_chain1`'s packed `[i8, i16]` legacy
layout. The same correction removes the `%struct.Wrap` parity failure; no
verifier/layout parity checks were weakened, no tests were weakened, and no
rendered-tag consumer fallback was added.

## Suggested Next

Continue Step 6 against the remaining C external-suite runtime families exposed
after identity normalization. In the delegated subset, `00216.c` now gets past
the `%struct.Wrap` layout parity check and fails later as
`[RUNTIME_NONZERO] ... exit=no such file or directory`; previous
`20040709-{1,2,3}.c` runtime failures remain outside this packet's proof
subset.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- The fixed mirror failures were producer-side identity mismatches. Keep future
  call/GEP aggregate work on normalized `StructNameId` production; do not relax
  `verify.cpp` mirror/text checks to make the remaining parity failures pass.
- The current delegated GEP layout parity leftovers are cleared. The remaining
  observed failure in this proof subset is C-side runtime harness behavior for
  `00216.c`, not a structured layout mismatch.
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

Proof is in `test_after.log`:
`cmake --build build --target c4cll && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_src_00216_c|llvm_gcc_c_torture_src_pr71083_c|frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_global_type_ref|frontend_lir_extern_decl_type_ref)$'`.

The delegated proof fails 1/6 after the layout fix. Passing tests:
`llvm_gcc_c_torture_src_pr71083_c`, `frontend_lir_call_type_ref`,
`frontend_lir_function_signature_type_ref`, `frontend_lir_global_type_ref`, and
`frontend_lir_extern_decl_type_ref`. Remaining failure:
`c_testsuite_src_00216_c` now reaches `[RUNTIME_NONZERO] ... exit=no such file
or directory`; its previous `%struct.Wrap` structured layout parity mismatch is
no longer present.

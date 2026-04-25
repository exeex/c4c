Status: Active
Source Idea Path: ideas/open/114_lir_type_text_authority_demotion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Verify Emission Parity and Authority Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 4 validation/lifecycle review for the accepted
verifier demotion route covering `LirCallOp`, `LirGlobal.llvm_type_ref`, and
`LirExternDecl.return_type`.

Accepted converted verifier checks:

- `LirCallOp.return_type` now validates with `require_type_ref` first and
  treats a declared, struct-typed `StructNameId` as the structured identity
  authority. Declared text still rejects a mismatched `StructNameId`, and known
  struct text without a `StructNameId` still fails.
- `LirCallOp.arg_type_refs` now accepts stale mirror text when parsed call
  argument text names a declared struct and the mirror `StructNameId` matches
  that struct. Missing or mismatched `StructNameId` cases still fail.
- Argument mirrors without a declared struct boundary still fall back to
  explicit mirror-text comparison, including structured mirrors whose text no
  longer matches the unresolved call text.
- `frontend_lir_call_type_ref` now proves stale-text structured acceptance for
  both call returns and argument mirrors, plus missing-ID, mismatched-ID, and
  undeclared-boundary fallback rejection. Printer behavior remains unchanged
  for the normal lowered module.
- `LirGlobal.llvm_type_ref` and `LirExternDecl.return_type` remain aligned with
  the same structured-first verifier pattern: declared struct boundaries accept
  stale mirror text only when the mirror `StructNameId` still matches the
  declared struct identity, with legacy text retained as fallback/proof.

Reviewer recheck `review/idea114_verifier_demotions_recheck.md` judged the
route `on track`, matching the source idea, with no testcase overfit or
expectation downgrade. The fresh full-suite proof in `test_after.log` passed
2980/2980 tests.

## Suggested Next

Keep idea 114 active for now. Do not start another adjacent verifier family
unless the supervisor explicitly expands the current selected scope.

Next bounded lifecycle packet: decide whether to close/deactivate after
normalizing close-scope regression evidence, or delegate a narrow Step 3/Step 4
packet for `LirFunction.signature_return_type_ref` and
`LirFunction.signature_param_type_refs` verifier parity if function signature
text authority is still considered in scope.

## Watchouts

- This route intentionally did not change printer output paths, backend, MIR,
  `LirGlobal::llvm_type`, `LirExternDecl::return_type_str`,
  `LirFunction::signature_text`, or raw `LirTypeRef` equality.
- `LirCallOp.return_type` has no separate legacy return-text field, so the
  structured accept path is bounded by the mirror `StructNameId` resolving to a
  declared struct and by rejecting mismatches when the mirror text itself names
  another declared struct.
- `LirCallOp.arg_type_refs` keeps the stricter declared-boundary behavior:
  stale mirror text is accepted only when parsed call argument text resolves to
  the same declared struct.
- Remaining blocked or undecided surfaces:
  - `LirFunction.signature_text` remains printer/bridge text, and the current
    verifier still compares signature return/parameter mirrors against parsed
    signature text.
  - Broad raw `LirTypeRef` text equality/output semantics remain
    `type-ref-authority-blocked`.
  - Backend and MIR legacy text consumers remain out of scope for this route.
- `clang-format` is not installed in this workspace; formatting was kept
  manual and local.

## Proof

Fresh Step 4 proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` shows the build completed and `100% tests
passed, 0 tests failed out of 2980`.

Close-time regression note: existing canonical logs are not same-scope
acceptance logs. `test_before.log` is the earlier 5-test focused subset, while
`test_after.log` is the full 2980-test suite. A closure packet should refresh
matching close-scope logs before moving the source idea to `ideas/closed/`.

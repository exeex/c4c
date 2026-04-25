Status: Active
Source Idea Path: ideas/open/114_lir_type_text_authority_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repeat Demotion for Adjacent Safe Families

# Current Packet

## Just Finished

Completed `plan.md` Step 3 repair for the reviewer-flagged `LirCallOp`
verifier demotion drift.

Converted verifier checks:

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

## Suggested Next

Run supervisor/reviewer acceptance for the repaired Step 3 call verifier slice
and decide whether any remaining adjacent families need another bounded packet
or lifecycle review.

## Watchouts

- This repair intentionally did not change printer output paths, backend, MIR,
  globals, extern declarations, function signatures, or raw `LirTypeRef`
  equality.
- `LirCallOp.return_type` has no separate legacy return-text field, so the
  structured accept path is bounded by the mirror `StructNameId` resolving to a
  declared struct and by rejecting mismatches when the mirror text itself names
  another declared struct.
- `LirCallOp.arg_type_refs` keeps the stricter declared-boundary behavior:
  stale mirror text is accepted only when parsed call argument text resolves to
  the same declared struct.
- `clang-format` is not installed in this workspace; formatting was kept
  manual and local.

## Proof

Ran the supervisor-selected proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_global_type_ref|frontend_lir_|positive_split_llvm_)') > test_after.log 2>&1`

Result: passed. `test_after.log` shows the build completed and 5/5 selected
tests passed.

Status: Active
Source Idea Path: ideas/open/114_lir_type_text_authority_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repeat Demotion for Adjacent Safe Families

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for the selected `LirGlobal.llvm_type_ref`
verifier family.

Converted verifier checks:

- `LirGlobal.llvm_type_ref` now validates the mirror through `require_type_ref`
  first, then accepts declared struct globals by comparing the mirror
  `StructNameId` to the declared struct named by `global.llvm_type`.
- Structured global mirrors must still resolve to an existing declared struct;
  mismatched structured identity is rejected with a `LirGlobal.llvm_type_ref`
  verifier error.
- Legacy text comparison remains the fallback/proof path for non-structured
  mirrors or structured mirrors whose `llvm_type` cannot be resolved to a
  declared struct.
- `frontend_lir_global_type_ref` now asserts both the structured-first accept
  path and the legacy text fallback reject path. Printer behavior remains
  unchanged and still uses `global.llvm_type`.

## Suggested Next

Choose the next Step 3 adjacent family only if the supervisor wants another
small structured-first verifier demotion; otherwise run reviewer/supervisor
validation before deciding whether the plan needs lifecycle review.

## Watchouts

- This slice intentionally did not change `LirGlobal::llvm_type`, printer
  output paths, backend, MIR, extern declarations, function signatures, or raw
  `LirTypeRef` equality.
- The new structured-first accept path is limited to globals whose legacy
  `llvm_type` resolves to a declared struct in the same module.
- Non-structured global mirrors still use the old text proof, so incomplete or
  non-aggregate cases are not silently widened.

## Proof

Ran the supervisor-selected proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_global_type_ref|frontend_lir_|positive_split_llvm_)') > test_after.log 2>&1`

Result: passed. `test_after.log` shows the build completed and 5/5 selected
tests passed.

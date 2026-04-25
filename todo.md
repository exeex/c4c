Status: Active
Source Idea Path: ideas/open/114_lir_type_text_authority_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repeat Demotion for Adjacent Safe Families

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for the selected `LirExternDecl.return_type`
verifier family.

Converted verifier checks:

- `LirExternDecl.return_type` now validates the mirror with `require_type_ref`
  first, then accepts declared struct extern returns by comparing the mirror
  `StructNameId` to the declared struct named by `return_type_str`.
- Structured extern return mirrors must still be struct-typed, resolve through
  `StructNameTable`, and point at an existing declared struct.
- Legacy text comparison remains the fallback/proof path for non-structured
  mirrors or structured mirrors whose `return_type_str` cannot be resolved to a
  declared struct boundary.
- `frontend_lir_extern_decl_type_ref` now asserts the structured-first accept
  path with stale mirror text and the fallback reject path without a declared
  struct boundary. Printer behavior remains unchanged and still uses
  `return_type_str`.

## Suggested Next

Run reviewer/supervisor validation for Step 3 route quality before choosing
another adjacent family or deciding whether the plan needs lifecycle review.

## Watchouts

- This slice intentionally did not change `LirExternDecl::return_type_str`,
  printer output paths, backend, MIR, globals, function signatures, or raw
  `LirTypeRef` equality.
- The new structured-first accept path is limited to extern declarations whose
  legacy `return_type_str` resolves to a declared struct in the same module.
- Non-structured extern return mirrors still use the old text proof, so
  incomplete or non-aggregate cases are not silently widened.
- `clang-format` is not installed in this workspace; formatting was kept
  manual and local.

## Proof

Ran the supervisor-selected proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_global_type_ref|frontend_lir_|positive_split_llvm_)') > test_after.log 2>&1`

Result: passed. `test_after.log` shows the build completed and 5/5 selected
tests passed.

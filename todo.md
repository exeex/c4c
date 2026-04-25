Status: Active
Source Idea Path: ideas/open/114_lir_type_text_authority_demotion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Verify Emission Parity and Authority Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 4 bounded verifier/test packet for
`LirFunction.signature_return_type_ref` and
`LirFunction.signature_param_type_refs`.

`LirFunction.signature_*` verification now uses parsed `signature_text` as the
declared boundary, but no longer treats rendered mirror text as authoritative
when a mirror carries a declared `StructNameId`. Return and parameter mirrors
accept stale mirror text only when the parsed signature return/parameter names
the same declared struct identity. Missing `StructNameId` for known struct
signature text still fails, and mismatched `StructNameId` still fails.

`frontend_lir_function_signature_type_ref` now proves stale-text acceptance for
structured return and parameter mirrors, missing-ID rejection, mismatched-ID
rejection, undeclared-boundary fallback rejection, and byval/raw text fallback
rejection. Existing printer assertions remain unchanged and continue to prove
that `signature_text` is still the printer/bridge text.

## Suggested Next

Supervisor should review this Step 4 slice for acceptance and decide whether
idea 114 is ready for close/deactivate handling with matching regression
evidence.

## Watchouts

- This route intentionally did not change printer output paths, backend, MIR,
  `LirGlobal::llvm_type`, `LirExternDecl::return_type_str`,
  `LirFunction::signature_text`, or raw `LirTypeRef` equality.
- Function signature parameter parsing only treats exact declared struct text
  or a leading declared struct token followed by the parameter name as a
  structured boundary. Byval fragments such as `ptr byval(%struct.Big) align 8`
  and raw aggregate fragments remain on the explicit text/named fallback path.
- Broad raw `LirTypeRef` text equality/output semantics remain
  `type-ref-authority-blocked`.
- Backend and MIR legacy text consumers remain out of scope for this route.
- `clang-format` is not installed in this workspace; formatting was kept
  manual and local.

## Proof

Fresh Step 4 signature verifier proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_global_type_ref|frontend_lir_|positive_split_llvm_)') > test_after.log 2>&1`

Result: passed. `test_after.log` shows the build completed and `100% tests
passed, 0 tests failed out of 5`.

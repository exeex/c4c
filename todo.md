# Current Packet

Status: Active
Source Idea Path: ideas/open/181_function_pointer_signature_type_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Propagate Structured Signature Identity

## Just Finished

Step 3 from `plan.md`: added the first structured indirect-call signature
carrier on `LirCallOp` and populated it from
`CallTargetInfo::callee_fn_ptr_sig` without changing rendered LIR syntax.

Changed boundary:

- `LirCallOp` now has optional `callee_signature` metadata carrying a structured
  return type ref, fixed parameter ABI type spellings, fixed parameter type
  refs, variadic state, unspecified-parameter-list state, and void-parameter
  list state.
- `make_lir_call_op_with_return_type_ref()` and `make_lir_call_op()` accept the
  optional carrier while preserving the existing `callee_type_suffix` and
  `args_str` display/compatibility text.
- `src/codegen/lir/hir_to_lir/call/target.cpp` populates the carrier only for
  indirect calls with `CallTargetInfo::callee_fn_ptr_sig`; direct calls and
  calls without structured function-pointer metadata continue using the
  rendered suffix path.
- LIR verification checks the structured carrier against return/parameter
  mirrors and call arguments when metadata is available.
- BIR indirect-call parsing now prefers `LirCallOp::callee_signature` before
  falling back to reparsing `callee_type_suffix`, keeping the rendered suffix as
  compatibility spelling rather than semantic authority for metadata-rich
  indirect calls.

## Suggested Next

Add collision-prone coverage for metadata-rich indirect calls whose rendered
suffix or lowered ABI shape would otherwise collide, especially nominal record
parameter identity and variadic function-pointer calls.

## Watchouts

- Keep display text intact: `callee_type_suffix`, `signature_text`, and printed
  LIR/BIR output remain output/compatibility spelling.
- The remaining high-risk boundary is proof coverage: existing targeted tests
  validate build, LIR verifier compatibility, and BIR handoff, but they do not
  yet add a new collision-prone function-pointer identity fixture.
- Guard same-feature cases, not just one fixture: fixed-arity indirect call,
  variadic function pointer, unspecified params, typedef-backed function
  pointer param, nested return function pointer, and nominal record parameter
  identity.
- Do not expand into full parser or canonical type replacement. The selected
  path should be one call-site metadata bridge plus verifier/BIR preference.
- `ideas/open/182_type_identity_migration_closure_gate.md` remains blocked
  until this source idea is complete.

## Proof

Passed. Log: `test_after.log`.

Command:

`( cmake --build build --target frontend_lir_call_type_ref_test frontend_lir_function_signature_type_ref_test backend_lir_to_bir_notes_test -j && ctest --test-dir build -R '^(frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref|backend_lir_to_bir_notes)$' --output-on-failure ) > test_after.log 2>&1`

Result: build completed and CTest passed
`frontend_lir_function_signature_type_ref`, `frontend_lir_call_type_ref`, and
`backend_lir_to_bir_notes` (3/3).

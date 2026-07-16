Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer migration for
`LirCastOp.from_type` / `LirCastOp.to_type` native scalar cast endpoints.

- Added a verifier endpoint helper that routes integer endpoints through
  `render_integer_type_ref` and floating endpoints through
  `render_floating_type_ref`, while preserving the existing generic
  `require_module_type_ref` path for pointer, aggregate, vector, void,
  compatibility, and other non-scalar endpoints.
- Updated `verify_cast_op_authority` floating endpoint exactness and width
  checks to use native floating builtin authority rather than mutable display
  text, without weakening the existing cast-kind and direction checks.
- Updated the cast printer to render native integer and floating scalar
  endpoints from structured authority while leaving non-scalar endpoints on the
  generic renderer.
- Added focused stale-display coverage for an integer cast endpoint and a
  floating cast endpoint in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not touch universal model deletion, Raw-BIR,
734/797 surfaces, or unrelated LIR consumers. Cast pointer and non-scalar
endpoints still rely on the existing generic verifier/printer paths.

## Proof

Completed cast endpoint packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`

Status: Active
Source Idea Path: ideas/open/186_bir_direct_symbol_identity_validation_closure.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Strengthen metadata-rich direct-symbol validation

# Current Packet

## Just Finished

Step 2 completed for idea 186. LIR-to-BIR direct-call lowering now treats a
direct global `LirCallOp` with structured `callee_signature` as generated
metadata-rich user/extern call metadata and requires
`direct_callee_link_name_id` to name a declared function symbol before it can
fall through to raw callee spelling. Missing ids and present-but-unknown ids
fail closed in the direct-call semantic family.

Focused backend coverage was added for:

- structured direct call with missing `direct_callee_link_name_id` failing
  closed
- structured direct call with an unknown/stale present `LinkNameId` failing
  closed
- raw/no-metadata direct call compatibility still lowering with an invalid id
- existing structured direct-call success fixtures declaring their semantic
  callees explicitly

Existing pointer-initializer coverage still exercises present
`initializer_function_link_name_ids` failing closed instead of falling back to
raw function-symbol lookup.

## Suggested Next

Run Step 3 validation review for idea 186: confirm direct-symbol closure still
has the intended raw/no-id and runtime/placeholder carveouts, and decide
whether additional BIR verifier coverage is needed before lifecycle closeout.

## Watchouts

- The new direct-call fence keys on structured `callee_signature`; runtime and
  placeholder compatibility should remain on no-signature/raw call routes.
- Raw/no-id direct calls without structured callee metadata remain accepted.
- Pointer initializer `initializer_function_link_name_ids` already fails
  closed on present-but-unknown ids and should not be widened through raw
  lookup.

## Proof

`git diff --check -- src/backend/bir/lir_to_bir/calling.cpp tests/backend/backend_lir_to_bir_notes_test.cpp todo.md` passed.

Proof command passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

`test_after.log`: 100% tests passed, 0 failed out of 109 backend tests.

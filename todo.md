Status: Active
Source Idea Path: ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Or Select Structured Byval Signature Metadata

# Current Packet

## Just Finished

Completed Step 2 by adding explicit structured byval metadata at the existing
function signature metadata boundary.

Changes:
- Added `LirSignatureParam::is_byval` as the structured byval carrier.
- Populated `is_byval` from the same AMD64 SysV aggregate-byval predicate used
  by generated function signature rendering.
- Updated LIR verification to reject disagreement between
  `signature_params[index].is_byval` and the corresponding
  `signature_param_type_refs[index]` byval ABI fragment.
- Extended the frontend signature metadata test so declared and defined byval
  functions assert explicit structured byval metadata, and verifier tests reject
  both missing-byval-flag and invalid-explicit-byval shapes.

## Suggested Next

Execute Step 3: route BIR function-signature aggregate/byval collection through
structured `LirSignatureParam::is_byval` metadata when structured signature
metadata exists, keeping `signature_text` parsing as the legacy no-metadata
fallback only.

## Watchouts

- Backend lowering was intentionally not changed in Step 2; `collect_aggregate_params`
  still needs Step 3 work to stop reading byval state from `signature_text` for
  metadata-rich functions.
- Legacy raw LIR fixtures without `signature_params` still need the text parser
  compatibility path.
- Step 2 verifier checks only run when a structured `LirSignatureParam` is
  present; metadata-free `signature_param_type_refs` compatibility remains
  accepted for existing aggregate-param fixtures.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green.

Proof log: `test_after.log`.

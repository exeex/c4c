Status: Active
Source Idea Path: ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate And Prepare Completion Evidence

# Current Packet

## Just Finished

Completed Step 5 by running final validation and recording close-preparation
evidence for idea 191.

Implementation summary:
- `LirSignatureParam::is_byval` is now the structured signature metadata carrier
  for explicit incoming byval aggregate parameters.
- HIR-to-LIR generation fills the structured byval fact alongside signature
  parameter type refs, and LIR verification rejects inconsistent metadata-rich
  byval mirror state instead of letting rendered text repair it.
- BIR aggregate parameter collection consumes structured signature facts when
  they are present, so generated metadata-rich lowering no longer parses
  `signature_text` to decide explicit byval handling.
- Backend tests cover positive structured byval lowering, stale/missing
  structured byval metadata failing closed, and the retained raw compatibility
  path.

## Suggested Next

Supervisor should hand idea 191 to plan-owner for lifecycle close review.
Recommended lifecycle decision: close idea 191, assuming the supervisor accepts
the recorded full-suite `test_after.log` as the canonical after proof for the
current slice.

## Watchouts

- Compatibility boundary: `signature_text` remains final output spelling and
  raw/no-metadata fixture input only. Metadata-rich generated LIR must use
  structured `signature_params` and `signature_param_type_refs` authority.
- Legacy parsing remains intentionally reachable only when structured signature
  metadata is absent; tests clear both `signature_params` and
  `signature_param_type_refs` to exercise that fallback.
- The stale structured byval failure reports through the local-memory semantic
  family because aggregate-param alias materialization is where the missing
  structured byval fact becomes fatal.
- Idea 188 should be able to treat idea 191 as a closed dependency after
  plan-owner closure, with idea 194 still remaining independently open.

## Proof

Focused proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green.

Backend subset proof:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, `109/109` runnable tests green; `12` matching tests disabled.

Final full-suite proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`

Result: passed, `3137/3137` runnable tests green; `12` disabled tests did not
run.

Proof log: `test_after.log`.

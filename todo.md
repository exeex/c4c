Status: Active
Source Idea Path: ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate And Prepare Completion Evidence

# Current Packet

## Just Finished

Completed Step 4 by recording the existing focused evidence that stale
`signature_text` cannot override structured function-signature byval metadata.

Evidence recorded:
- Positive structured byval: `expect_structured_incoming_byval_param_materializes_from_type_ref`
  lowers a metadata-rich incoming byval parameter through `StructNameId`,
  preserves BIR byval ABI metadata, and materializes the structured leaf loads.
- Stale/missing metadata rejection: `expect_metadata_rich_incoming_byval_param_without_byval_flag_fails_closed`
  leaves stale byval spelling in `signature_text` but clears
  `signature_params[0].is_byval`; lowering must fail instead of recovering the
  byval fact from rendered text.
- Structured metadata validity: frontend verifier coverage rejects byval mirror
  text without explicit `LirSignatureParam::is_byval`, rejects explicit byval
  metadata without a byval mirror, and accepts stale rendered parameter text
  when the structured `signature_param_type_refs` fact remains authoritative.
- Legacy fallback boundary: `expect_legacy_incoming_byval_param_without_signature_type_ref_uses_legacy_layout`
  clears `signature_params` and `signature_param_type_refs`, proving the
  remaining `signature_text` parsing path is isolated to no-metadata fixtures.

## Suggested Next

Execute Step 5: run the supervisor-selected final validation, then record the
implementation summary, compatibility boundary, and completion proof for
lifecycle review.

## Watchouts

- Step 5 should preserve the distinction between metadata-rich generated LIR
  and raw/no-metadata compatibility fixtures; final evidence should call out
  that `signature_text` remains output spelling and legacy fallback input only.
- The stale structured byval failure reports through the local-memory semantic
  family because aggregate-param alias materialization is where the missing
  structured byval fact becomes fatal.
- Final validation should refresh the canonical proof log requested by the
  supervisor; this packet did not touch proof logs.

## Proof

No new command run; this was an evidence-only packet per delegation.

Existing focused proof cited:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green.

Proof log: `test_after.log`.

Existing backend subset cited:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, `109/109` runnable tests green; `12` matching tests disabled.

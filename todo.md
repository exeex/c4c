Status: Active
Source Idea Path: ideas/open/127_lir_structured_function_signature_metadata_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Backend/BIR Signature Lowering

# Current Packet

## Just Finished

Completed `plan.md` Step 3, `Convert Backend/BIR Signature Lowering`.

Converted BIR consumers:

- `lower_signature_return_info` now reads
  `LirFunction::signature_return_type_ref` before any legacy fallback, so
  declaration return ABI lowering no longer parses `signature_text` when
  structured return metadata is present.
- `lower_function_params_with_layouts` now reads `signature_params`,
  `signature_param_type_refs`, `signature_is_variadic`, and
  `signature_has_void_param_list` before any legacy fallback, so fixed
  parameters, variadic status, void parameter lists, byval aggregate parameter
  ABI, and sret insertion are driven by structured metadata.
- `collect_aggregate_params` now consumes the same structured parameter view
  and normalizes byval type-ref fragments before aggregate layout lookup.
- Existing `parse_function_signature_params` use remains only as an explicit
  compatibility fallback for hand-built legacy LIR fixtures that do not
  populate the structured signature fields.
- Added a backend structured-context test where drifted `signature_text` says
  `void(void)` while structured metadata says `i32`, byval `%struct.Pair`, and
  variadic; BIR lowering follows the structured metadata.

## Suggested Next

Delegate Step 4 to convert LIR verifier signature checks so semantic
signature validation reads structured metadata first and any retained
`signature_text` parsing is limited to final-render consistency or diagnostics.

## Watchouts

Remaining non-BIR consumers still need the later verifier/aarch64 steps from
the runbook. BIR still has a text parser for legacy hand-built LIR fixtures
with missing structured signature fields; generated LIR should not take that
path once Step 2 metadata is present.

## Proof

Supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
passed. Proof log: `test_after.log`.

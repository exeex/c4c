Status: Active
Source Idea Path: ideas/open/383_string_authority_guard_unclassified_symbols.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Replace Prepared Computed Value String Containers

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by replacing the remaining
`PreparedComputedValueLookup` string-keyed computed-value containers with
interned `ValueNameId` authority.

- `PreparedComputedValueLookup` now stores binaries and global loads by
  `ValueNameId`.
- Computed-value producers intern BIR result names once before inserting into
  the prepared lookup maps.
- `classify_computed_value` now probes binaries, global loads, parameters, and
  recursive active entries through `ValueNameId`.
- `classify_supported_immediate_binary` now checks source operands through
  `ValueNameId` instead of string spelling.
- The focused guard no longer reports
  `PreparedComputedValueLookup::named_binaries` or
  `PreparedComputedValueLookup::named_global_loads`.

## Suggested Next

Ask the supervisor to review the completed Step 4 slice and decide whether the
active runbook is ready for lifecycle closure or broader validation.

## Watchouts

- `scripts/string_authority_classifications.json` was not edited.
- A compatibility overload remains for existing direct test calls that pass
  spelling-keyed temporary maps; it immediately converts those inputs to
  `ValueNameId` maps before using the shared classifier.
- No in-scope reports remain from the focused guard after this packet.

## Proof

Ran the delegated proof command and preserved combined output in
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(string_authority_guard|backend_prepare_(liveness|stack_layout|frame_stack_call_contract|structured_context)|backend_prepared_printer)$'`

The build completed successfully. `string_authority_guard`,
`backend_prepare_liveness`, `backend_prepare_stack_layout`,
`backend_prepare_frame_stack_call_contract`,
`backend_prepare_structured_context`, and `backend_prepared_printer` all
passed.

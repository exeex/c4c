Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow The Selected Boundary

# Current Packet

## Just Finished

Step 2 - Remove Or Narrow The Selected Boundary: narrowed
`materialize_missing_frame_slot_call_arguments` so frame-slot call argument
publication no longer calls `find_bir_value_for_prepared_name`.

The fallback now requires the existing prepared before-call move bundle to
classify the argument move, consumes the prepared frame-slot source from
`PreparedCallArgumentPlan` plus `PreparedValueHome`, and emits a normal
call-boundary move record for the prepared frame-slot load into the ABI
register. If those prepared facts are missing, the fallback skips instead of
recovering publication authority by retained-BIR prepared-name scan.

## Suggested Next

Supervisor should review whether Step 2 is sufficient to close the selected
boundary leak or whether the next packet should remove any now-dead bridge
surface around frame-slot call argument fallback publication.

## Watchouts

- `find_bir_value_for_prepared_name` remains for
  `materialize_call_boundary_source_to_destination`; this packet removed only
  the selected frame-slot call-argument fallback dependency.
- The narrowed fallback is intentionally silent when the prepared before-call
  move/binding or frame-slot source facts are absent, matching the prior
  best-effort fallback shape without reintroducing retained-BIR lookup.
- No tests or expectation contracts were changed.

## Proof

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_call_boundary_effect_plan)$') > test_after.log 2>&1`

Passed. Proof log: `test_after.log`.

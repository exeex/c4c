Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3.1.3
Current Step Title: Result Source Authority Completion
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed Step 3.1.3 "Result Source Authority Completion" packet work for
idea 88 by re-auditing the scalar result-side identity route and proving the
previous `source_value_id` addition was redundant. For covered scalar results,
`destination_value_id` is already populated from the call result value itself
via `find_regalloc_value_by_name(*regalloc_function, value_name_id)`, so it
already names the direct prepared/regalloc owner rather than a separate
carrier-only or destination-only identity. Removed the redundant route and
restored focused proof around the existing field.

Current packet result:
- Removed the redundant `PreparedCallResultPlan::source_value_id` route.
- Confirmed the existing `destination_value_id` already publishes the direct
  scalar result owner identity for covered cases, including register-backed,
  float-register, and spilled-to-slot results.
- Focused frame-stack/call contract coverage now proves scalar result identity
  through `destination_value_id` while keeping the independently useful source
  storage-shape publication (`source_storage_kind`, `source_reg`,
  `source_stack_offset_bytes`) intact.
- Prepared call-plan printer coverage now proves stack-backed scalar results
  still dump direct owner identity through `destination_value_id` next to the
  published source/destination storage facts.

## Suggested Next

Step 3.1.3 appears complete from the scalar semantic-identity angle; ask the
supervisor to decide whether to close this substep and advance to the next Step
3 call-boundary packet, or to request a separate ergonomics-only follow-up such
as human-readable result owner naming in dumps.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- Treat result source shape and result source identity as separate facts:
  `source_storage_kind` answers "what ABI carrier shape produces the scalar
  result," while `destination_value_id` already answers "which prepared scalar
  owner that result semantically belongs to" for covered cases.
- If a later follow-up wants a human-readable result owner name in summaries,
  keep it as dump ergonomics rather than reopening the semantic identity route
  or duplicating existing ids.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.

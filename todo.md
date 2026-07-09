Status: Active
Source Idea Path: ideas/open/625_prepared_stack_slot_preservation_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh stack-slot preserve evidence

# Current Packet

## Just Finished

Activation created the runbook from
`ideas/open/625_prepared_stack_slot_preservation_source_publication.md`; no
implementation work has started.

## Suggested Next

Execute Step 1 from `plan.md`: refresh current ordinary same-module
stack-slot preserve diagnostics, record the command and proof artifact, and
classify first-owner buckets before any implementation work.

## Watchouts

- Do not infer preserve source registers inside RV64 from ABI parameter
  position, function storage summaries, source filename, testcase shape, or
  final assembly layout.
- Keep `src/20020529-1.c` and adjacent ordinary-call preserve rows as breadth
  or guard evidence, not named-case implementation targets.
- Do not broaden into outgoing stack argument destination offsets,
  variadic/library policy, runtime mismatch, local/global producers,
  stack-frame consumers, expectations, unsupported markers, allowlists,
  timeouts, or accounting changes.

## Proof

Lifecycle activation only; no build or test proof required.

Status: Active
Source Idea Path: ideas/open/279_phase_f5_x86_memory_accesses_public_field_parity_gate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run Acceptance Proof

# Current Packet

## Just Finished

Completed Step 4 by running the focused x86 acceptance proof for the
public-field parity gate non-applicability outcome.

The proof passed with both selected x86 backend tests green. This packet did
not weaken any fallback, status-debug, route-debug, prepared-printer, wrapper,
helper-oracle, exact-output, or baseline contract.

## Suggested Next

Supervisor should decide whether to close, retire, or replace this active
runbook. The Step 4 acceptance proof is complete.

## Watchouts

Do not claim x86 public-field parity from the existing x86 Route 3 / Route 5
source-memory agreement tests: they are real backend consumers, but they consume
`edge_publications`, `PreparedEdgePublication::source_memory_access`, and
`PreparedAddressingFunction`, not the public `memory_accesses` lookup field.
Do not use helper-only lookup tests, prepared-printer output, wrappers,
copied-publication paths, or renamed helpers as substitutes for the missing
boundary. The current gate preserves existing fallback/source-memory behavior
while recording public-field non-applicability; it is not an x86 capability
repair.

## Proof

Ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_handoff_label_authority|backend_x86_prepared_decoded_home_storage)$'; } > test_after.log 2>&1
```

Result: passed. Build was up to date, and the focused CTest subset passed 2/2:
`backend_x86_prepared_handoff_label_authority` and
`backend_x86_prepared_decoded_home_storage`.

Proof log path: `test_after.log`.

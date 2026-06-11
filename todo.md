Status: Active
Source Idea Path: ideas/open/184_phase_e_route1_producer_constant_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff and contraction guard

# Current Packet

## Just Finished

Step 5 prepared the Route 1 publication-source migration handoff and
contraction guard for the selected consumer:
`value_publication_may_read_register_index(...)`.

The selected Route 1 view is the publication producer view used by that
consumer for same-block publication-source reads. The route-first behavior is:
when a usable same-block Route 1 producer is available, the consumer answers
through the publication producer view before relying on prepared
source-producer indexes; when the Route 1 view is incomplete or out of scope,
the consumer falls through to the existing prepared helper/home behavior.

The final focused coverage names the selected consumer and proves the migrated
route-first behavior against a same-block nonconstant binary producer. It also
keeps the prepared fallback/oracle surfaces visible: prepared same-block
producer oracle comparison, no-producer/home fallback, missing-producer
fail-closed behavior, future-producer fallback, recursive operand register
dependencies after clearing prepared source-producer indexes, and rejection of
an unrelated register.

## Suggested Next

Supervisor/plan-owner review can decide whether to close or replace the active
runbook. No executor follow-up packet is recommended from this handoff alone.

## Watchouts

This handoff claims only the selected
`value_publication_may_read_register_index(...)` Route 1 consumer migration.
It does not claim broad prepared API deletion, route-wide migration, or
aggregate contraction.

Residual prepared APIs remain intentional guardrails for this slice: target
register, home, storage, move, machine-record policy, fallback behavior, and
oracle comparison surfaces still live in the existing AArch64/prepared paths.
`NoProducer` is not treated as an authoritative negative; incomplete Route 1
views still fall through to prepared helper/home behavior.

The hook-produced full-suite baseline candidate in `test_baseline.new.log` was
rejected by the supervisor because it had 3
`c_testsuite_aarch64_backend_*` failures. It is not an accepted baseline.

## Proof

No build/tests were run for Step 5 because this packet is a `todo.md` handoff
summary only.

Final focused proof already passed:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$'; } > test_after.log 2>&1`

The supervisor rolled the accepted focused proof forward to `test_before.log`.
Accepted proof log: `test_before.log`. The rejected
`test_baseline.new.log` full-suite baseline candidate is not accepted proof.

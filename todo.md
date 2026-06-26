Status: Active
Source Idea Path: ideas/open/384_prepared_global_symbol_memory_access_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Producer Source For Global Lanes

# Current Packet

## Just Finished

Lifecycle switched from closed idea 383 to the upstream prepared-data
publication owner in idea 384. No implementation packet has run for this
runbook yet.

## Suggested Next

Delegate Step 1 to audit the producer path for `LoadLocalInst` global-address
lanes in `src/20030914-2.c`, starting with `main`, block `entry`, inst `0`.
The packet should identify whether the complete prepared global-symbol
memory-access fact set is available to publish or whether a narrower upstream
contract prerequisite is missing.

## Watchouts

- Do not add target-side fallback inference from raw `LoadLocalInst addr
  <global>` spelling; idea 383 closed that route as fail-closed.
- Keep behavior independent of testcase `src/20030914-2.c`, global `gs`, and
  lane offset `68`.
- Keep `plan.md` and this `todo.md` pointed at
  `ideas/open/384_prepared_global_symbol_memory_access_publication.md`.

## Proof

No implementation proof has run for Step 1 yet. The lifecycle close/switch gate
uses `test_before.log` and `test_after.log`.

Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Make Grouped Spill And Reload Publication Truthful
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3.3 added grouped float spill/reload parity proof on top of the existing
vector and grouped GPR coverage, proving that a width-2 RISC-V call-crossing
float value can be evicted from the only legal `fs1,fs2` callee span, keep
truthful `spill_register_authority`, publish matching grouped spill/reload
ops, and preserve grouped stack-storage identity through liveness checks,
direct prepared consumption, and prepared-printer output.

## Suggested Next

Ask the supervisor whether Step 3.3 can now close and move on to the next
remaining grouped-width runbook packet, since grouped spill/reload publication
now has direct vector, grouped GPR, and grouped float proof.

## Watchouts

- The grouped general and grouped float spill fixtures both need real
  post-call pressure to force eviction from the lone width-2 callee span; if
  that pressure drops, the proof silently becomes a non-spill case again.
- Grouped spill/reload publication now has direct proof for vector LMUL,
  width-2 general spans, and width-2 float spans without reopening grouped
  allocation policy.
- Do not reopen width-aware pool policy or idea 90 follow-on work from this
  packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after adding focused backend liveness, direct prepared-consumer,
and prepared-printer proof for grouped width-2 float spill/reload parity.
Log: `test_after.log`

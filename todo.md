Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Make Grouped Spill And Reload Publication Truthful
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3.3 proved the grouped spill/reload publication seam through a non-vector
grouped GPR case, adding focused backend coverage that a width-2 RISC-V
call-crossing general value can be evicted from the only legal `s1,s2` callee
span, keep truthful `spill_register_authority`, publish matching grouped
spill/reload ops, and preserve grouped stack-storage identity through direct
prepared consumption and printer output instead of vector-only coverage.

## Suggested Next

Ask the supervisor whether Step 3.3 should close now or whether a final
follow-on packet should add grouped float spill/reload parity proof before
moving on to the remaining grouped-width runbook work.

## Watchouts

- The new grouped general spill fixture needs real post-call pressure to force
  eviction from the lone width-2 GPR callee span; if that pressure drops, the
  proof silently becomes a non-spill case again.
- Grouped spill/reload publication now has direct proof for vector LMUL and
  width-2 general spans; grouped float spill/reload parity is the remaining
  obvious sibling if the supervisor wants more Step 3.3 confidence.
- Do not reopen width-aware pool policy or idea 90 follow-on work from this
  packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after adding focused backend liveness, direct prepared-consumer,
and prepared-printer proof for grouped width-2 general spill/reload
publication. Log: `test_after.log`

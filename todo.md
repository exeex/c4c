Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Make Width-Aware Grouped Allocation Decisions Truthful
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3.2 repaired the first truthful grouped-allocation decision seam by making
target register-pool span generation width-aware for non-vector numeric
register classes whenever the published pool itself is contiguous, so grouped
width `> 1` general/float values stop collapsing into scalar-only candidate
sets. The packet also added focused backend proof that a grouped cross-call GPR
value on RISC-V now receives a real contiguous callee-saved span and that the
candidate-span surface publishes truthful grouped general/float spans instead
of vector-only behavior.

## Suggested Next

Advance to Step 3.3 and repair the first grouped spill/reload publication seam
that still falls back to scalar-shaped authority after a width-aware grouped
allocation decision has been made.

## Watchouts

- The Step 3.2 repair only publishes grouped spans when the explicit saved
  register pool is itself numerically contiguous; sparse pools still truthfully
  yield no grouped candidate span instead of inventing unavailable units.
- Grouped allocator consumers should compare/publish occupied span units rather
  than only base register names when deciding whether an existing assignment
  already satisfies a grouped destination.
- Keep the follow-on Step 3.3 work focused on spill/reload publication, not on
  widening pool policy or reopening call-boundary consumer logic from Step 3.1.
- Do not reopen idea 90 out-of-SSA follow-on work from this packet.
- Reject testcase-shaped shortcuts; grouped-width progress still needs to
  generalize beyond one seeded grouped cross-call fixture.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after the Step 3.2 repair and focused backend liveness coverage
for grouped general/float candidate spans plus grouped cross-call GPR
allocation. Log: `test_after.log`

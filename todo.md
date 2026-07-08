Status: Active
Source Idea Path: ideas/open/590_branch_stack_load_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Completed Step 3, "Wire Freshness Authority Into One Route", from `plan.md`.

Wired the selected scalar `Condition` branch stack-load collector route through
`BranchStackLoadSource` freshness selection. The collector now synthesizes a
branch-stack-slot freshness candidate at the exact BIR branch terminator point
(`block_index` plus `block.insts.size()`), queries it before accepting
`LoadFromStackSlot`, and sets `stack_slot_fresh_at_branch` only after the
selected authority matches the branch source value/home/use/source/proof/rank
and terminator point.

The route fails closed for no candidate, invalid source/proof/rank, ambiguous
candidates, wrong value, wrong use, wrong stack home, stale terminator point,
future terminator point, and stack-home-only authority. Pointer `Lhs`/`Rhs`
collector rows remain inventory-only with `policy=none`; frame-slot source
facts, target emission, typed/aggregate producer facts, expectations,
unsupported markers, allowlists, and runtime output were not changed.

## Suggested Next

Execute Step 4 from `plan.md`: add focused proof or prepared dump visibility
for the migrated scalar `Condition` route, including accepted explicit
freshness, missing/invalid freshness, and a stack-home-only rejection, while
keeping existing 587, 588, and closed-589 freshness tests green.

## Watchouts

- Prepared printer implementation was not in the Step 3 owned file set, so
  source-freshness candidate details are recorded on the authority object but
  the existing branch-stack-load printer row still shows only role/value/policy/
  pointer/status/slot/object/offset/size/align.
- Step 4 should decide whether printer-source ownership is needed for expanded
  source-freshness visibility; do not edit printer implementation without that
  ownership.
- The first exact proof attempt hit an unrelated parallel `cc1plus` OOM while
  compiling `backend_aarch64_instruction_dispatch_test.cpp.o`; a serial build
  completed the object, then the exact proof command passed.

## Proof

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
passed after a serial `cmake --build --preset default -j1` recovered from the
first exact build attempt's unrelated `cc1plus` OOM kill. Proof log:
`test_after.log`.

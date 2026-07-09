Status: Active
Source Idea Path: ideas/open/622_repeated_stack_destination_fan_in_order_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Authority In RV64 Prepared Move Bundles

# Current Packet

## Just Finished

Step 3 RV64 consumption slice completed for the
`stack_destination_register_fan_in` authority family.

Threaded the prepared move-bundle consumer classification's explicit
`stack_destination_fan_in_authority` fact into RV64 prepared move-bundle
lowering. RV64 now accepts stack-destination register fan-in only when the
bundle and move carry `StackDestinationRegisterFanIn` and the explicit fact has
owner `prepared_stack_destination_register_fan_in`, authority kind
`StackDestinationRegisterFanIn`, select-materialization
preserved-stack-fallback semantics, the stack destination value/home, and
source facts whose candidate order matches the move bundle rows. Tag-only
authority no longer authorizes the RV64 stack-destination path.

Focused backend coverage now asserts the legal authorized select-shaped
stack-destination fan-in emits, missing explicit prepared authority reports
`missing_stack_destination_fan_in_authority_fact`, unsupported and unknown
authority remain fail-closed, ambiguous non-parallel fan-in remains rejected,
and malformed authority without the preserved stack fallback reports
`mismatched_stack_destination_register_fan_in_move_authority`.

## Suggested Next

Run the next focused coverage or closure packet for this plan: review whether
the Step 3 RV64 consumer plus Step 2 producer fully covers the source idea's
supported select-materialization preserved-stack-fallback shape, then either
close the active runbook or delegate any remaining non-select producer fact as a
separate packet.

## Watchouts

This slice does not infer destination ordering from move rows or authority kind
alone; RV64 consumes the source candidate order from the prepared fact. The
only admitted family remains the legal select-materialization stack-destination
shape with two or more register sources and one preserved stack fallback.
Repeated stack destinations inside out-of-SSA parallel-copy bundles still need
an explicit producer fact or a separate authority family before RV64 can
consume them. No `src/pr71631.c` special-case was added.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. `test_after.log` is preserved as the proof log.

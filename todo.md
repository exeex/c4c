Status: Active
Source Idea Path: ideas/open/622_repeated_stack_destination_fan_in_order_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Positive And Negative Backend Coverage

# Current Packet

## Just Finished

Step 4 focused backend coverage completed for the supported and rejected
`stack_destination_register_fan_in` authority shapes.

Added consumer-contract coverage that pins the public status and diagnostic
category names for unsupported non-parallel multi-source stack-destination
authority and mismatched stack-destination register fan-in authority. Tightened
the RV64 positive authorized select-materialization test so it now proves the
prepared authority-backed fan-in path emits both register-source stack stores,
while the existing negative coverage continues to prove missing explicit
authority, unsupported/unknown authority, malformed missing preserved-stack
fallback, generic ambiguous, and mismatched shape rejection paths.

## Suggested Next

Run Step 5 regression/closure readiness: compare the completed Step 2 producer,
Step 3 RV64 consumer, and Step 4 coverage against the source idea, then decide
whether the active runbook is ready for closure or needs a separate non-select
producer/authority packet.

## Watchouts

The positive byte assertion intentionally checks the two register-source stores
only; preserved-stack fallback handling remains covered through the prepared
authority shape and existing malformed-without-fallback rejection. Repeated
stack destinations inside out-of-SSA parallel-copy bundles still need an
explicit producer fact or a separate authority family before RV64 can consume
them. No `src/pr71631.c` special-case was added.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. `test_after.log` is preserved as the proof log.

Status: Active
Source Idea Path: ideas/open/119_bir_block_label_structured_identity_for_assembler.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prove Compatibility And Handoff Remaining Work

# Current Packet

## Just Finished

Step 6 compatibility proof and handoff inventory completed.

Structured ids are available for BIR block labels, branch targets,
conditional branch true/false targets, scalar phi incoming labels, label-address
rendering, and the prepared control-flow handoff. BIR printing resolves those
ids through `bir::Module::names.block_labels` when possible and preserves raw
strings as compatibility fallback text.

The broad backend proof stayed green with the current prepared/backend handoff
in place.

## Suggested Next

Next coherent packet: supervisor/plan-owner should decide whether the active
runbook is ready for closure or whether the deferred fallback inventory should
be split into a later cleanup idea.

## Watchouts

- Remaining raw-string fallback dependencies are deferred:
  - BIR structs still store raw labels beside ids for blocks, branch targets,
    conditional branch targets, phi incoming labels, and label-address bases.
  - Validation and fixture-style construction paths can still create raw-only
    BIR until the lowering/interning boundary or explicit tests attach ids.
  - CLI focus filters and matching still compare requested function/block/value
    text against raw BIR/prepared spellings.
  - Prepared liveness, stack-layout, dynamic-stack, and out-of-SSA paths still
    intern or match some block/predecessor labels from raw strings.
  - Legacy prepared/raw consumers and target-local MIR/codegen routes still
    depend on raw label spelling; MIR migration is outside this runbook.
- Keep raw label strings as fallbacks until a later cleanup idea removes them.
- Same-spelled labels such as `entry` across functions share an id under the
  module table, matching the prepared backend spelling-table model.

## Proof

Passed. Proof log: `test_after.log`.

Command run exactly:
`( cmake --build build-backend && ctest --test-dir build-backend/tests/backend --output-on-failure ) > test_after.log 2>&1`

Result: `cmake --build build-backend` completed successfully, and backend CTest
reported 107 runnable tests passed, 0 failed. The existing disabled backend CLI
trace/focus tests remained disabled.

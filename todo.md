# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Preserve block-entry and value-home helpers as prepared consumers

## Just Finished

Completed Step 3's narrow `value_publication_may_read_register_index` repair.

The helper now consumes current block-entry/value-home facts before producer
lookup, validates prepared source-producer facts by block label, instruction
index, result value, and payload pointer before recursive cast/binary/select
dependency checks, and preserves the existing same-block fallback only when
prepared source facts do not answer.

## Suggested Next

Continue Step 3 by checking the remaining block-entry/value-home publication
helpers for any producer-sensitive decisions that still bypass prepared source
or value-home authority.

## Watchouts

This packet intentionally left the legacy same-block query as a fallback for
cases where prepared source-producer facts are absent or invalid; prepared facts
take precedence when they validate. No raw move-bundle/value-name scans,
expectation downgrades, local-slot address changes, or test-specific shortcuts
were added.

## Proof

Ran the supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. The build completed and CTest reported 100% tests passed, 0
tests failed out of 163. Proof log: `test_after.log`.

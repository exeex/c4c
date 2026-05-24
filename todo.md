Status: Active
Source Idea Path: ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Reuse Path For x86 Prepared Blocks

# Current Packet

## Just Finished

Step 4 proved the x86 prepared reuse path for the prealloc block-entry
publication helper.

`x86::prepared::Query` now exposes
`collect_block_entry_publications(...)`, a narrow wrapper over
`prepare::collect_prepared_block_entry_publications(...)` for prepared block
facts by successor label.

The existing x86 prepared internal test now builds block-entry
`OutOfSsaParallelCopy` facts and verifies available publication facts,
missing-home status, successor-label filtering, register-name availability,
and preservation of source `PreparedMoveBundle`, source
`PreparedMoveResolution`, and value-home authority.

This is reuse evidence only. x86 block lowering was not rewritten, and x86
target move emission, register classes, operand spelling, and instruction
selection remain target-local.

## Suggested Next

Step 5 should validate behavior and anti-overfit coverage for the active
edge-copy/block-entry plan.

## Watchouts

Do not broaden validation into implementation changes unless a real blocker is
found. AArch64 edge producer walking, select-chain materialization, target
register spelling/parsing, scratch-register selection, machine records, target
operands, move diagnostics, and x86 target emission remain target-local.

The helper intentionally reports facts and statuses for target consumers; it
does not silently downgrade unsupported destination/storage forms to fallback
emission.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 155/155 backend tests. Proof log: `test_after.log`.

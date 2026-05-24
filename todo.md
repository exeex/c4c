Status: Active
Source Idea Path: ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Adapt AArch64 To Consume The Shared Helper

# Current Packet

## Just Finished

Step 3 adapted AArch64 block-entry/publication consumers to use the prealloc
block-entry publication helper.

`dispatch_publication.cpp` now calls
`prepare::collect_prepared_block_entry_publications(...)` for the repeated
current-block `BlockEntry` plus `OutOfSsaParallelCopy` bundle filtering used
by current publication detection, publication register resolution,
scalar-state recording, and clobber checks.

`dispatch_producers.cpp` now uses the same helper for the current block join
parallel-copy source check, while preserving the existing AArch64-local
producer/source rules.

The adaptation keeps target register parsing/spelling, physical-register
aliasing, scalar-state recording, scratch choice, load/store mnemonics,
frame/sp address policy, inline asm and machine-instruction record
construction, concrete edge-copy emission, and diagnostic behavior in AArch64.
Unsupported/missing forms remain filtered by the local consumers with the same
strictness as before.

## Suggested Next

Step 4 should prove an x86 prepared block reuse path for the shared
block-entry publication helper without rewriting x86 block lowering or moving
x86 target emission into prealloc.

## Watchouts

Do not broaden the next packet into AArch64 edge producer walking,
select-chain materialization, target register spelling/parsing,
scratch-register selection, machine records, target operands, or move
diagnostics. Those remain target-local.

The helper intentionally reports facts and statuses for target consumers; it
does not silently downgrade unsupported destination/storage forms to fallback
emission.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 155/155 backend tests. Proof log: `test_after.log`.

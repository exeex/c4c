Status: Active
Source Idea Path: ideas/open/63_aarch64_shared_select_chain_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Migrate ALU And Value-Materialization Select Consumers

# Current Packet

## Just Finished

Completed Step 4 non-call migration slice for ALU and generic value
materialization select consumers.
`PreparedScalarSelectChainMaterialization` now represents a generic prepared
select-chain materialization root, with direct-global dependency facts retained
inside the same record when present. The ALU direct-global fast path still
requires the existing direct-global dependency fact and no longer rebuilds
edge-publication source-producer lookups locally when prepared lookups are
missing. Generic value publication now queries the prepared scalar select-chain
materialization fact before calling the AArch64 select emitter; missing facts
fail closed instead of falling back to semantic discovery in the value
publication path. The old AArch64 producer wrapper no longer falls back to raw
MIR dependency traversal when prepared select-chain facts are unavailable.

## Suggested Next

Supervisor should review whether Step 5 should move the remaining AArch64
select materialization emitter out of broad dispatch edge-copy ownership into a
more precise local owner.

## Watchouts

This packet intentionally left indirect-callee select-chain handling untouched.
`emit_select_chain_value_to_register` still owns AArch64 recursive select
emission and still uses prepared same-block select producer lookup to spell the
target-local branch/label sequence; the migrated callers now require prepared
materialization/dependency facts before entering that emitter. The remaining
`find_prepared_direct_global_select_chain_dependency` call in `calls.cpp` is
still the indirect-callee path and remains outside this packet.

## Proof

Proof command run exactly:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed; `test_after.log` is the canonical proof log. `git diff --check`
also passed.

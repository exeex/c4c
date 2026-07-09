Status: Active
Source Idea Path: ideas/open/631_direct_global_symbol_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Narrow RV64 Direct-Global Consumer Admission

# Current Packet

## Just Finished

Step 4 added narrow RV64 object-emission admission for scalar prepared
`LoadLocalInst` and `StoreLocalInst` local-memory rows whose selected address
carrier is `PreparedAddressBaseKind::GlobalSymbol`. The encoded fragment path
now emits PC-relative direct-global address materialization plus 1/2/4/8-byte
GPR load/store instructions only when the selected prepared access is default
address space, non-volatile, direct materialization policy, scalar layout
authority, publication-authorized, width-matched, alignment no larger than
width, symbol-identified, base-plus-offset capable, and signed-12-bit offset
encodable.

`object_emission.cpp` now admits the same fact shape before rejecting local
memory as unsupported, while preserving the existing raw `LoadLocalInst`
global-address diagnostic when no prepared direct-global facts are present.
`tests/backend/mir/backend_riscv_object_emission_test.cpp` adds a direct-global
scalar local-memory fixture proving relocation-backed `sw`/`lw` emission and
fail-closed coverage for missing prepared access, missing symbol identity,
missing base-plus-offset, non-default address space, volatile access,
unsupported addressing policy, incomplete extent, missing range, wrong layout
authority, large/non-encodable offset, wrong base kind, and unsupported width.

## Suggested Next

Step 5 evidence packet: re-run the focused direct global-symbol local-memory
row probe/allowlist from Step 1 and classify the remaining rows after the RV64
consumer admission. Record whether `src/pr46309.c` or adjacent direct
global-symbol local-memory rows moved past `unsupported_local_memory_access`,
whether any rows still fail due to direct-global policy gaps, and which
residual rows belong to out-of-scope owners.

## Watchouts

The Step 4 consumer deliberately requires `ScalarLayout`; aggregate lane global
memory remains on the existing global-memory path and byte-storage aggregate
local-memory rows remain rejected. Large direct-global offsets still fail
closed unless a future packet adds an explicit supported materialization
sequence. The next packet should not widen this admission to string constants,
aggregate homes, move bundles, runtime mismatch rows, or prepared global
value-location rows owned by idea 621.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: passed. `test_after.log` is the preserved proof log.

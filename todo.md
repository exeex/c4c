Status: Active
Source Idea Path: ideas/open/383_rv64_global_aggregate_lane_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Global Aggregate Lane Materialization

# Current Packet

## Just Finished

Step 3 routed the raw global aggregate lane boundary without adding fallback
lowering from raw address spelling.

The supported explicit path remains `LoadGlobalInst` plus a
`PreparedAddressBaseKind::GlobalSymbol` prepared memory-access fact. The raw
`LoadLocalInst` shape with `bir::MemoryAddress::BaseKind::GlobalSymbol` now
fails closed in `diagnose_unsupported_prepared_instruction_fragment` before
the generic local-memory diagnostic and reports:

```text
unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes
```

The focused rejected fixture
`rejects_raw_load_local_global_address_lane_without_prepared_access` now
asserts that upstream prepared-data publication diagnostic.

## Suggested Next

Step 4 should rerun the representative torture proof to confirm the first
remaining boundary is now reported as a missing prepared global-symbol
memory-access publication fact rather than generic local-memory addressing.

## Watchouts

- The new diagnostic keys only on the semantic BIR memory-address base kind
  `GlobalSymbol`; it does not use `src/20030914-2.c`, `gs`, or raw address
  spelling as behavior keys.
- There is still no object-emission lowering from raw `LoadLocalInst` global
  addresses. Upstream must publish authoritative prepared global-symbol
  memory-access facts for this lane family.

## Proof

Delegated Step 3 proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1
```

Result: passed. The supervisor-selected focused backend object-emission subset
is sufficient for this diagnostic routing slice. Proof log: `test_after.log`.

Status: Active
Source Idea Path: ideas/open/383_rv64_global_aggregate_lane_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Supported Or Rejected Fixtures

# Current Packet

## Just Finished

Step 2 added focused RV64 object-emission fixtures for the global aggregate
lane publication boundary.

New supported fixture:

- `emits_prepared_global_aggregate_lane_load_from_explicit_facts` builds an
  aggregate-like 72-byte constant global with 18 `i32` lanes, publishes an
  explicit `LoadGlobalInst` plus `PreparedAddressBaseKind::GlobalSymbol`
  prepared memory access at byte offset `68`, and proves object emission
  creates global storage, a defined object symbol, the PC-relative relocation
  pair, and `lw` with offset `68`.

New rejected fixture:

- `rejects_raw_load_local_global_address_lane_without_prepared_access` builds a
  raw `LoadLocalInst` carrying `addr aggregate_lanes+68` spelling and a BIR
  global-address payload, but publishes no prepared global-symbol memory-access
  record. RV64 object emission remains fail-closed and does not infer object
  identity, section, storage, or relocation semantics from raw address spelling.

## Suggested Next

Step 3 should route the representative `LoadLocalInst addr <global>+offset`
shape to its upstream prepared-data publication owner, or improve the rejected
raw-shape diagnostic to name the missing prepared global-symbol memory-access
fact precisely before rerunning the representative.

## Watchouts

- Do not implement RV64 object-emission fallback parsing of `addr gs`; that
  would bypass the prepared contract and reopen idea 357 by inference.
- Keep the focused supported path semantic: explicit `LoadGlobalInst` plus
  prepared `GlobalSymbol` access facts and module global storage are
  target-consumable. A raw `LoadLocalInst` global-address spelling without the
  prepared record remains an upstream publication gap.
- `src/20030914-2.c` and the spelling `gs` remain representative evidence
  only, not behavior keys.

## Proof

Delegated Step 2 proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1
```

Result: passed. The supervisor-selected focused backend object-emission subset
is sufficient for this fixture-only slice. Proof log: `test_after.log`.

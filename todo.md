Status: Active
Source Idea Path: ideas/open/aarch64-codegen-prepared-boundary-recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Boundary Review and Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by moving the target-independent Prepared
value-home/name lookup responsibility into shared `prealloc` support:
`value_locations.hpp` now exposes helpers to resolve a `PreparedValueId` from
`ValueNameId` and a `PreparedValueHome` from either id or name, including the
existing optional indexed lookup contract.

Updated AArch64 consumers so `dispatch_lookup.cpp` delegates to the shared
helpers and `dispatch_entry_formals.cpp` no longer carries duplicate anonymous
Prepared value-id/home lookup copies. AArch64 register parsing, register views,
instruction selection, records, and printer logic stayed in AArch64.

## Suggested Next

Execute `plan.md` Step 3 by reviewing the boundary move against the source
idea, confirming no `.cpp` merge/header consolidation/test weakening slipped
in, and deciding whether dependent header consolidation is unblocked or another
Prepared/MIR boundary runbook is needed.

## Watchouts

- The new shared helpers are templated only for the optional indexed lookup
  adapter shape; the actual Prepared value-id/home interpretation now lives in
  `prealloc`.
- `prepared_named_value_id` remains an AArch64 wrapper around the existing
  prepared name-table resolver because the packet only moved value-home/id
  lookup and did not relocate generic name resolver declarations.
- Do not perform header-family consolidation or `.cpp` merging in this plan.
- Do not weaken tests, expectations, or backend contracts.

## Proof

Ran exact delegated proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: passed. Build completed, and `ctest -R '^backend_'` reported 149/149
tests passed.

Proof log path: `test_after.log`.

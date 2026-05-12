Status: Active
Source Idea Path: ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Or Tighten LinkNameId Authority For The Selected Route

# Current Packet

## Just Finished

Completed Step 2 by adding `LinkNameId` authority to the selected dynamic
global scalar-array materialization route.

Implementation summary:
- `DynamicGlobalAggregateArrayAccess` and `DynamicGlobalScalarArrayAccess` now
  carry `LinkNameId` metadata alongside final global spelling.
- Dynamic global aggregate-to-aggregate, aggregate-to-scalar, and scalar
  GEP projections preserve the carried id.
- `load_dynamic_global_scalar_array_value(...)` resolves the emitted
  `LoadGlobalInst::global_name_id` from the carried id when present, validates
  it against the current module global table and link-name table, and fails
  closed on mismatch or missing structured identity instead of falling back
  through raw spelling.
- Raw/no-id compatibility is retained only for `kInvalidLinkName` accesses.
- `backend_lir_to_bir_notes_test` now keeps the existing matching-id drifted
  display success case, adds a missing-link-name-spelling rejection case, and
  adds an explicit raw/no-id compatibility case that proves the route does not
  invent `LinkNameId` metadata.

## Suggested Next

Execute Step 3 by checking the downstream selected-route consumers for any
remaining spelling-first semantics after dynamic scalar-array materialization,
then record whether the BIR `LoadGlobalInst` boundary is already sufficient or
whether another owned handoff needs to preserve the structured id.

## Watchouts

- Do not re-prove only the addressed-global pointer route already closed by
  idea 187.
- Do not treat local route names, local slots, SSA temporaries, or block labels
  as semantic global symbols.
- The new fail-closed check lives at materialization time. Public LIR fixtures
  can cover missing carried-id spelling, while mismatched carried-id proof may
  still need consumer-level evidence or a narrower test harness because the
  producer derives the id from the same `GlobalInfo` entry as final spelling.
- Raw/no-id fallback remains available only when the selected access has no
  structured id; preserving final BIR spelling is not the same as using raw
  spelling as semantic identity.

## Proof

Ran delegated proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' > test_after.log`

Result: passed, `1/1` selected test green. Proof log: `test_after.log`.

Supervisor then ran broader same-scope acceptance proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`

Regression guard against `test_baseline.log`: passed, before `3137/3137`,
after `3137/3137`, no new failures.

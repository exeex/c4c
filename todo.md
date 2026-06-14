Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Design the Value-Home Agreement Boundary

# Current Packet

## Just Finished

Step 2 design complete for the `names` value-home lookup candidate.

- Boundary placement: add a narrow shared agreement helper in
  `src/backend/prealloc/lookup_agreement.hpp` /
  `src/backend/prealloc/lookup_agreement.cpp`, then call it from the selected
  BIR-value value-home path in `src/backend/prealloc/prepared_lookups.cpp`.
- The helper should sit beside the existing prepared lookup agreement helpers
  because it must compare BIR-name spelling, prepared name-table resolution,
  backing function locations, and optional indexes before any BIR-value result
  is treated as structurally agreed.
- Keep `src/backend/prealloc/value_locations.hpp` behavior and public
  prepared lookup helpers compatible; do not move the public first-emplace or
  scan fallback semantics into a BIR agreement claim.
- Accepted agreement row: the BIR value is named and non-empty, the prepared
  name table resolves that spelling to exactly one `ValueNameId`, function
  locations are available, the resolved prepared value id exists, the prepared
  value home exists in backing `PreparedFunctionLocations`, and any supplied
  `PreparedValueHomeLookups` row points to the same backing value id/home.
- Rejected/fail-closed rows: null function locations, immediate or otherwise
  non-named BIR values, empty BIR names, missing prepared name-table entries,
  absent prepared value ids, missing prepared homes, stale supplied indexes,
  duplicate or conflicting prepared names, duplicate or conflicting prepared
  value ids, and prepared/BIR spelling drift.
- Public compatibility rows to preserve: direct prepared lookups can still use
  the current index-or-scan behavior, including first row retained by
  `emplace` and null results for missing data. Those rows remain compatibility
  behavior only and must not become evidence of structured BIR agreement.
- Candidate implementation files for Step 3:
  `src/backend/prealloc/lookup_agreement.hpp`,
  `src/backend/prealloc/lookup_agreement.cpp`,
  `src/backend/prealloc/prepared_lookups.cpp`, and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Step 3 should not edit `value_locations.hpp` unless the existing declarations
  prevent the adapter from calling already-public prepared lookup helpers.

## Suggested Next

Execute Step 3: implement the narrow value-home agreement bridge.

Step 3 executor packet:

```text
to_subagent: c4c-executor
Objective: implement the shared value-home agreement helper for idea 260 `names` value-home lookup candidate.
Plan Step: Step 3: Implement Narrow Value-Home Bridge
Owned Files: src/backend/prealloc/lookup_agreement.hpp, src/backend/prealloc/lookup_agreement.cpp, src/backend/prealloc/prepared_lookups.cpp, tests/backend/bir/backend_prepared_lookup_helper_test.cpp, todo.md
Do Not Touch: plan.md, ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md, src/backend/prealloc/value_locations.hpp unless the existing public helper declarations block the adapter, route-debug files, target-output baselines, unsupported expectations, same-block lookup code, semantic resolver code, control-flow code, store-source publication code, printer/debug code.
Context: Step 2 chose a shared helper in `lookup_agreement.*`. Public prepared value-home lookup compatibility must remain unchanged: direct prepared lookup rows may keep current index-or-scan and first-emplace behavior, but BIR-value agreement must fail closed on stale indexes, duplicate/conflicting prepared names or ids, missing backing rows, and prepared/BIR spelling drift.
Task: Add the narrow helper/adaptor, wire it only into `find_prepared_value_home_for_bir_value(...)`, and add focused prepared lookup helper rows for one accepted named value-home lookup plus nearby fail-closed rows. Keep the helper scoped to value-home agreement; do not absorb other idea 260 candidates.
Proof: (cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
Done When: focused helper tests prove the accepted value-home row and fail-closed agreement rows, `todo.md` records the implementation/proof result, and no unrelated output or unsupported expectation files are changed.
If Blocked: stop and report the exact ambiguity; do not edit plan.md or source idea.
```

## Watchouts

- Keep this runbook limited to the selected `names` value-home lookup
  candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, prepared value-home records, prepared lookup
  indexes, public aggregate compatibility, and current null fallback behavior.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings,
  same-block lookup, semantic resolver API, control-flow, store-source
  publication, or backend lowering behavior to claim progress.
- Stale supplied indexes are currently authoritative when non-null; Step 2
  requires agreement with the backing value locations before Step 3 wires any
  BIR-value authority through them.
- Duplicate/conflicting prepared ids or names are currently first-inserted by
  the lookup builder; Step 3 should fail closed for BIR agreement without
  breaking existing public prepared lookup compatibility.
- The helper must distinguish "public lookup returned a row" from "structured
  BIR agreement exists"; stale indexes and duplicate rows are the key route
  risks for this packet.

## Proof

Ran `git diff --check -- todo.md`; proof passed. No `test_after.log` is needed
for this design-only packet.

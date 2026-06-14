Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Value-Home Lookup Contract

# Current Packet

## Just Finished

Step 1 inventory complete for the `names` value-home lookup candidate.

- Selected lookup row: `find_prepared_value_home_for_bir_value(...)` resolves a
  named BIR value by `PreparedNameTables::value_names`, then
  `find_indexed_prepared_value_id(...)`, then
  `find_indexed_prepared_value_home(...)`.
- Direct indexed row: `find_indexed_prepared_value_home(indexes, locations,
  PreparedValueId)` returns the indexed `homes_by_id` pointer when indexes are
  supplied; otherwise it scans `value_locations->value_homes`.
- Direct name row: `find_indexed_prepared_value_id(indexes, regalloc,
  locations, ValueNameId)` returns the indexed `value_ids` mapping when indexes
  are supplied; otherwise it scans regalloc first, then value homes.
- `make_prepared_value_home_lookups(...)` currently builds `homes_by_id` and
  `value_ids` with `emplace`, so duplicate prepared ids or names keep the first
  inserted row and do not currently report ambiguity.
- Current null behavior to preserve: null function locations, immediate or
  otherwise non-named BIR values, empty BIR names, missing prepared name-table
  entries, missing indexed value ids, missing homes for resolved ids, and stale
  indexes all return `nullptr` or `std::nullopt`.
- Missing `PreparedNameTables` is not accepted by the BIR-value helper because
  it requires a reference; query wrappers such as current block-entry
  publication report `MissingNames` before reaching value-home lookup.
- Immediate values are rejected by `find_prepared_value_home_for_bir_value`
  before name lookup; publication fallbacks separately avoid source-home lookup
  when a move has `source_immediate_i32`.
- Candidate implementation files: `src/backend/prealloc/value_locations.hpp`,
  `src/backend/prealloc/prepared_lookups.cpp`, and possibly
  `src/backend/prealloc/lookup_agreement.hpp` /
  `src/backend/prealloc/lookup_agreement.cpp` if Step 2 chooses a shared
  agreement helper.
- Candidate test file:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, especially the
  prepared function lookup rows around value-home/indexed lookup and the
  current block-entry publication fixture.

## Suggested Next

Execute Step 2: design the value-home agreement boundary in `todo.md` only.

Step 2 packet:

- Decide whether the boundary is an inline adapter in `value_locations.hpp` or
  a shared helper in `lookup_agreement.*`.
- Define accepted rows as non-empty named BIR value, exactly one prepared
  `ValueNameId`, available function locations, resolved prepared value id,
  existing prepared value home, and matching indexed row when indexes are
  supplied.
- Define rejected rows as null function locations, immediate/non-named values,
  empty names, missing prepared name ids, missing homes, stale id/name indexes,
  duplicate or conflicting prepared names or ids, and prepared/BIR name drift.
- Keep public raw/index compatibility as current prepared behavior only; do not
  treat fallback spelling or first-emplace duplicate behavior as structural
  BIR agreement.
- Proposed Step 2 proof: `git diff --check -- todo.md`.

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
  should require agreement with the backing value locations before Step 3 wires
  any BIR-value authority through them.
- Duplicate/conflicting prepared ids or names are currently first-inserted by
  the lookup builder; Step 2 should fail closed for agreement without breaking
  existing public prepared lookup compatibility.

## Proof

Ran `git diff --check -- todo.md`; proof passed. No `test_after.log` is needed
for this inventory-only packet.

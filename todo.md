Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Semantic Resolver Contract

# Current Packet

## Just Finished

Completed Step 1: inventoried the current semantic resolver contract for the
`names` semantic resolver API candidate.

Selected resolver rows:

- `resolve_prepared_function_name_id(const PreparedNameTables&, string_view)`
  in `src/backend/prealloc/control_flow.hpp`.
- `resolve_prepared_block_label_id(const PreparedNameTables&, string_view)` in
  `src/backend/prealloc/control_flow.hpp`.
- `resolve_prepared_value_name_id(const PreparedNameTables&, string_view)` in
  `src/backend/prealloc/control_flow.hpp`.
- Construction-time comparison helper:
  `prepared_named_value_id(PreparedNameTables&, const bir::Value&)` in
  `src/backend/prealloc/names.hpp`.

Current resolver behavior:

- The three `resolve_prepared_*` helpers are non-interning wrappers around the
  prepared semantic name-table `find(...)` APIs. They accept spellings only,
  return `std::optional<...Id>`, and return `std::nullopt` when `find(...)`
  returns the domain invalid id.
- `SemanticNameTable::find(std::string_view)` and the underlying text table do
  not mutate. Empty spelling returns the invalid id, absent spelling returns
  the invalid id, and lookup does not create prepared text or semantic ids.
- The resolver helpers currently do not inspect BIR raw ids, do not validate a
  raw BIR id against a prepared spelling, and do not distinguish absent,
  empty, invalid raw id, duplicate/conflicting prepared rows, or prepared/BIR
  spelling drift beyond the single `std::nullopt` result.
- `prepared_named_value_id(PreparedNameTables&, const bir::Value&)` is separate
  construction-time behavior: it rejects non-named values and empty names, but
  interns a non-empty BIR value spelling into `names.value_names`. Step 2
  should not blur this mutating construction helper with non-interning
  resolver semantics.

Raw id and duplicate/conflict notes:

- Current resolver inputs are raw spellings, not raw BIR ids. Call sites that
  already hold a `bir::Value` or BIR label usually pass the spelling field to
  `resolve_prepared_*`; other construction paths still call direct
  `names.*.find(...)` or the mutating `prepared_named_value_id(...)`.
- Because prepared semantic tables are keyed by text id, normal `intern(...)`
  calls coalesce duplicate spellings to one prepared id. The current resolver
  API cannot report a duplicate/conflicting prepared-row condition, stale raw
  BIR id, or prepared/BIR spelling drift; those conditions need an explicit
  agreement helper/result if they become authoritative resolver facts.
- Existing direct `names.*.find(...)` call sites in prealloc and MIR backends
  are public compatibility surface for now. Step 2 should design a selected
  resolver API without rewiring unrelated same-block, value-home, control-flow,
  store-source, printer/debug, route-debug, or target lowering behavior.

Candidate implementation and proof files:

- Implementation candidates: `src/backend/prealloc/lookup_agreement.hpp`,
  `src/backend/prealloc/lookup_agreement.cpp`, and the inline resolver helpers
  in `src/backend/prealloc/control_flow.hpp` if Step 2 chooses a small result
  type or wrapper.
- Compatibility references: `src/backend/prealloc/names.hpp`, direct
  resolver consumers in `src/backend/prealloc/*`, and MIR backend helper
  wrappers such as AArch64 `prepared_named_value_id(...)` wrappers and x86
  `resolve_prepared_*` call sites.
- Focused tests: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`,
  using local `PreparedNameTables` fixtures to prove positive present-id rows,
  explicit absent/empty rows, non-interning table size preservation, and
  fail-closed behavior for raw-id/spelling drift or copied conflicting table
  rows without changing public direct lookup compatibility.

## Suggested Next

Execute Step 2: design the resolver agreement boundary for the three selected
`resolve_prepared_*` rows.

Suggested Step 2 packet:

- Define whether the API should introduce a small resolver agreement result in
  `lookup_agreement.*` or an inline helper near `control_flow.hpp` that can
  represent present id, explicit absent id, and fail-closed invalid/conflicted
  states without interning.
- Require empty spelling, missing prepared name, invalid raw BIR id,
  prepared/BIR spelling drift, and copied duplicate/conflicting table state to
  fail closed for authoritative semantic resolver agreement.
- Keep `prepared_named_value_id(...)` as construction-time interning only.
- Preserve existing public prepared aggregate compatibility and direct
  `names.*.find(...)` behavior until a later selected packet explicitly owns
  those call sites.
- Proof command for the next code/design packet:
  `(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Keep this runbook limited to the selected `names` semantic resolver API
  candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, public aggregate compatibility, existing
  construction-time interning behavior, and current absent/null resolver
  behavior.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings,
  same-block lookup, value-home lookup, control-flow, store-source
  publication, or backend lowering behavior to claim progress.
- Do not use `prepared_named_value_id(...)` as proof of resolver safety; it
  intentionally mutates prepared value names during construction.
- Treat raw BIR ids and raw spelling fallback as compatibility inputs only
  until the new resolver agreement explicitly validates them.

## Proof

Ran `git diff --check -- todo.md`; passed.

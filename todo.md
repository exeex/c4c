Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Design the Resolver Agreement Boundary

# Current Packet

## Just Finished

Completed Step 2: designed the resolver agreement boundary for the `names`
semantic resolver API candidate in canonical execution state only.

Chosen placement:

- Put the authoritative agreement boundary in
  `src/backend/prealloc/lookup_agreement.hpp` and
  `src/backend/prealloc/lookup_agreement.cpp`, alongside the other
  fail-closed prepared/BIR agreement helpers.
- Keep the existing inline `resolve_prepared_function_name_id`,
  `resolve_prepared_block_label_id`, and `resolve_prepared_value_name_id`
  wrappers in `src/backend/prealloc/control_flow.hpp` as compatibility
  surfaces unless Step 3 can wire them through the agreement helper without
  changing their public `std::optional` absent/null behavior.
- Keep `prepared_named_value_id(PreparedNameTables&, const bir::Value&)` in
  `src/backend/prealloc/names.hpp` as construction-time interning only. It
  must not be called from the non-interning resolver agreement path.

Agreement result design:

- Add a small explicit status model for semantic-name resolver agreement, for
  example `PreparedSemanticNameAgreementStatus` with `Unavailable`,
  `Available`, and `Conflicted` states, plus typed result structs carrying
  `FunctionNameId`, `BlockLabelId`, or `ValueNameId`.
- `Available` is the only authoritative present-id state. It requires a
  non-empty raw spelling, a non-invalid prepared id from the relevant prepared
  semantic name table, and spelling agreement between the raw BIR fact and the
  prepared table spelling for that id.
- `Unavailable` is allowed only for ordinary compatibility null/absent
  preservation when no authoritative prepared id can be exposed. It must not
  invent an id, intern text, or fall back to raw spelling as agreement.
- `Conflicted` is the fail-closed state for stale, ambiguous, invalid, or
  drifting name facts. Any consumer using the agreement API must treat it as no
  authoritative prepared resolver fact.

Accepted rows:

- Function names: non-empty BIR function spelling, existing prepared
  `FunctionNameId`, and `prepared_function_name(names, id)` equal to the raw
  function spelling.
- Block labels: non-empty raw block label spelling, valid raw BIR block label
  id when one is supplied, raw-id spelling agreement through
  `bir::NameTables::block_labels`, existing prepared `BlockLabelId`, and
  `prepared_block_label(names, id)` equal to the raw block label spelling.
- Value names: `bir::Value::Kind::Named`, non-empty value spelling, existing
  prepared `ValueNameId`, and `prepared_value_name(names, id)` equal to the BIR
  value spelling.

Rejected or fail-closed rows:

- Empty spelling for any function, block, or value resolver row.
- Missing prepared id for a non-empty raw spelling.
- Invalid raw BIR id when a block-label id participates in the agreement.
- Raw BIR id spelling that is empty or differs from the raw fallback spelling.
- Prepared table spelling that is empty, stale, or differs from the raw BIR
  spelling.
- Copied or manually corrupted prepared-name table state where `find(spelling)`
  returns an id whose stored spelling does not round-trip to that spelling.
- Any duplicate/conflicting prepared-row state observable through mismatched
  text ids, mismatched stored spelling, or a copied table whose ids no longer
  agree with the prepared text table.
- Any route that interns during a resolver query, including accidental use of
  `prepared_named_value_id(...)`, `names.*.intern(...)`, or non-const access to
  the prepared name tables.

Compatibility to preserve:

- Direct `names.function_names.find(...)`, `names.block_labels.find(...)`, and
  `names.value_names.find(...)` behavior remains public compatibility surface.
- Existing `resolve_prepared_*` wrappers continue to return
  `std::optional<...Id>` and preserve current `std::nullopt` behavior for
  empty or absent spellings unless Step 3 explicitly proves the same behavior
  through the new agreement helper.
- Backend/MIR wrappers that call `resolve_prepared_*` remain compatibility
  consumers, not proof that raw BIR ids or spelling fallback are semantic
  agreement.
- No same-block lookup, value-home lookup, control-flow dominance,
  store-source-publication, printer/debug, route-debug, target-output, or
  lowering behavior should move in this packet.

Candidate implementation and proof files:

- Implementation: `src/backend/prealloc/lookup_agreement.hpp`,
  `src/backend/prealloc/lookup_agreement.cpp`, and narrowly
  `src/backend/prealloc/control_flow.hpp` only if preserving wrapper semantics
  through the new helper is cleaner than leaving wrappers as compatibility
  adapters.
- Reference only: `src/backend/prealloc/names.hpp` to verify that
  `prepared_named_value_id(...)` remains a mutating construction helper.
- Tests: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, with
  local `PreparedNameTables` and BIR fixture rows for all three name families.

## Suggested Next

Execute Step 3: implement the narrow semantic resolver agreement API.

Suggested Step 3 packet:

- Owned files:
  `src/backend/prealloc/lookup_agreement.hpp`,
  `src/backend/prealloc/lookup_agreement.cpp`,
  `src/backend/prealloc/control_flow.hpp`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and `todo.md`.
- Add typed semantic-name agreement result structs and helpers for function
  names, block labels, and value names in `lookup_agreement.*`.
- Implement helpers as const, non-interning checks over `PreparedNameTables`
  and, for block-label id validation, `bir::NameTables`.
- Preserve the current direct `names.*.find(...)` compatibility surface and
  current `resolve_prepared_*` optional-return behavior. If wrappers are wired
  through the new helper, their empty/absent/nullopt behavior must remain
  unchanged.
- Add focused positive tests for agreed prepared function, block-label, and
  value-name ids.
- Add fail-closed tests for empty spelling, absent prepared id, invalid raw
  block-label id, raw-id spelling drift, prepared/BIR spelling drift, copied or
  corrupted prepared-name table mismatch, and non-interning table-size
  preservation.
- Proof command:
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
- Treat raw BIR ids and raw spelling fallback as compatibility inputs only.
  They become semantic resolver agreement only after the new helper validates
  spelling, id, and prepared-table round trip.
- BIR function and value rows currently expose spellings directly; raw-id
  validation is concrete for block labels through `bir::NameTables`. Do not
  invent new BIR ids for function or value names in this runbook.
- The agreement helper should fail closed, not downgrade tests or rewrite
  backend output expectations to claim progress.

## Proof

Ran `git diff --check -- todo.md`; passed.

# Parser Whitelist Boundary Audit

Status: Complete
Last Updated: 2026-03-29

## Goal

Review parser paths that still rely on broad "best effort" token skipping,
generic recovery, or permissive start-condition probes, and convert the risky
ones to whitelist-style boundary rules where practical.

The intent is not to make parsing brittle. The intent is to stop accepting
ambiguity by default in places where a narrow follow-set would be safer and
would prevent downstream state corruption.

## Why This Should Be A Separate Idea

The active `std::vector` bring-up has already exposed a repeated pattern:

- parser bugs are often not caused by one missing syntax form
- they are caused by a recovery or skip rule that is too wide
- the visible error then appears much later in an unrelated declaration

Recent examples include:

- record-member recovery that silently skipped to `}` and hid the real error
- `requires`-clause skipping that treated too many tokens as legal constraint
  payload and could eat into the following declaration
- broad declaration probes using `is_type_start()` in contexts where a smaller
  whitelist would be safer

That theme is broader than the current container bring-up and should be tracked
as parser-hygiene work in its own right.

## Audit Targets

The audit should inventory parser logic in categories like these:

1. Constraint / `requires` skipping

- `parse_optional_cpp20_requires_clause`
- `parse_optional_cpp20_trailing_requires_clause`
- `skip_cpp20_constraint_atom`

Questions:

- which tokens are currently treated as valid continuation by default?
- which declaration-boundary checks still rely on broad probes instead of an
  explicit follow-set?
- where can qualified names, template-ids, or operators be mistaken for the
  start of the next declaration?

2. Generic recovery loops

- `recover_record_member_parse_error`
- other token-skipping recovery helpers used after parse exceptions

Questions:

- which recoveries stop at `;` vs `}`?
- which ones can swallow an entire scope and let the real error surface much
  later?
- where should recovery results become tri-state or otherwise more explicit?

3. Start-condition probes

- `is_type_start()`
- `can_start_parameter_type()`
- declarator probes that use unresolved identifier fallbacks

Questions:

- where is a broad "could be a type" probe needed?
- where would a local whitelist of legal starters be more correct?
- which probes are reused in contexts that actually need different rules?

4. Parse-and-discard paths

- branches that return `NK_EMPTY`
- branches that skip unsupported syntax without preserving structure

Questions:

- is `NK_EMPTY` expected here, or is it hiding a missed parse?
- should the parser preserve a node, emit a targeted `not implemented`, or
  fail earlier instead of discarding the construct?

## Desired Output

This idea should produce:

- an inventory of non-whitelist parser boundaries and recovery rules
- a classification of which ones are intentionally broad vs accidentally broad
- a shortlist of the highest-risk rules to tighten first
- reduced repro tests for each tightened rule so the behavior stays stable

## Initial Tightening Heuristics

When reviewing a parser boundary, prefer these questions:

- can this boundary be expressed as "stop only on these explicit tokens"?
- does it currently delegate to a broader probe like `is_type_start()` that is
  valid in some contexts but too wide in this one?
- if the parser is unsure, should it stop and let the caller decide rather than
  continue consuming?
- if unsupported syntax is encountered, can we fail locally instead of
  recovering across scopes?

## Non-Goals

This idea should not require:

- rewriting every recovery path at once
- eliminating all parser heuristics
- blocking ongoing bring-up work until the full audit is complete

The point is to make the risky broad rules visible and then tighten them in a
reviewable order.

## Exit Criteria

- one curated list identifies parser sites that are not currently whitelist-led
- each site is tagged as acceptable breadth, suspicious breadth, or known bug
- the first batch of suspicious sites has reduced tests and a tightening plan
- future parser work can reference this audit instead of rediscovering the same
  "skip rule too wide" issue ad hoc

## Completion Notes

- Recorded the durable Step 1 / Step 2 inventory in
  `src/frontend/parser/BOUNDARY_AUDIT.md`.
- Landed the first tightening batch across top-level discard and recovery paths
  plus the older `types.cpp` record-member `requires` boundary helper.
- Added reduced parse-only regressions for the tightened parser-boundary slices,
  including
  `tests/cpp/internal/parse_only_case/record_member_requires_clause_recovery_preserves_following_decl_parse.cpp`
  for malformed member-template `requires` recovery.
- Validated the final runbook slice with focused parser coverage and the full
  regression guard; the suite remained monotonic with no newly failing tests.

## Leftover Issues

- The `types.cpp` record-member `requires` helper still has possible
  identifier-start false positives after incomplete `&&` / `||` chains because
  the current slice only stops before obvious non-identifier declaration
  starters.
- `recover_record_member_parse_error(int)` remains the next broader parser
  boundary candidate once this audit work is revisited.

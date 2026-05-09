# Parser Support Constexpr Type Helper Domain Tables

Status: Closed
Created: 2026-05-08
Closed: 2026-05-09

Parent Ideas:
- `ideas/closed/123_parser_legacy_string_lookup_removal_convergence.md`
- `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`

Related Open Ideas:
- `ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md`

## Goal

Move parser support helper APIs that still expose string-keyed semantic maps
toward TextId/domain-table contracts owned by parser and Sema.

The target is not literal parsing or diagnostic text. The target is helper
interfaces where `std::unordered_map<std::string, ...>` still looks like the
semantic environment for records, typedefs, named constants, or
`__builtin_types_compatible_p` style checks.

## Why This Idea Exists

`parser_support.hpp` still exposes helper APIs with string map contracts:

- `resolve_record_type_spec(... unordered_map<string, Node*>*)`
- `eval_const_int(... unordered_map<string, long long>*)`
- `resolve_typedef_chain(TypeSpec, unordered_map<string, TypeSpec>)`
- `types_compatible_p(TypeSpec, TypeSpec, unordered_map<string, TypeSpec>)`

Some are already documented compatibility bridges, but the API boundary still
invites new semantic callers to pass rendered strings. As parser/sema identity
cleanup progresses, these helpers should be split into structured parser-owned
tables and Sema-owned interpretation tables.

## Responsibility Split

Parser owns local syntax and provisional evaluation carriers:

- `TextId` named constants for parser-local constant folding
- provisional record carriers and direct `record_def`
- source-literal lexemes
- compatibility bridges for older tests and HIR proof paths

Sema owns semantic interpretation:

- typedef resolution under proper scope/domain tables
- record completion and layout authority where parser carriers are insufficient
- type compatibility under resolved nominal identity
- constant/value environments beyond parser-local syntax folding

## In Scope

- Inventory all call sites of parser support APIs that take string-keyed maps.
- Split pure parser-local TextId lookups from Sema-style semantic
  interpretation.
- Add TextId or domain-key overloads where parser-owned callers already have
  interned text.
- Demote string-map overloads to explicit compatibility wrappers with
  comments, tests, and removal conditions.
- Ensure `sizeof`, `alignof`, `offsetof`, typedef chain, and
  `types_compatible_p` helpers do not recover structured records through
  rendered maps after structured metadata misses.
- Add tests proving stale string maps do not override structured record or
  typedef identity.

## Out Of Scope

- Removing literal strings or source lexeme parsing.
- Full Sema type system redesign.
- Full C/C++ constant evaluator replacement.
- HIR/LIR layout cleanup already covered by idea 152.

## Acceptance Criteria

- Parser-owned helper call sites prefer TextId/domain-key overloads when
  structured metadata exists.
- String-map helper overloads, if retained, are explicit compatibility bridges
  and cannot override structured metadata misses.
- Sema-owned semantic interpretation is not hidden inside parser support
  string maps.
- Tests cover at least one stale record/typedef/named-constant string fallback
  that must not win over structured identity.

## Closure Rationale

Closed after the active runbook completed Step 6. Acceptance validation covered
the parser support, constexpr helper, type helper, template, and type-traits
subset with 294/294 tests passing. The proof included stale rendered fallback
coverage for record layout resolution and typedef/type-compatibility helpers,
and route review found no testcase-shaped matching, expectation-only progress,
or new ordinary parser-support `unordered_map<string, ...>` semantic contract.

Retained string-map overloads are compatibility bridges with documented removal
conditions rather than ordinary semantic environments, so the source acceptance
criteria are satisfied. The close-time regression guard passed in
non-decreasing mode against the canonical `test_before.log` and
`test_after.log`, both at 294/294 with no new failures.

## Reviewer Reject Signals

- A new parser helper accepts `unordered_map<string, ...>` as the ordinary
  semantic environment.
- A string-map fallback runs after a complete structured metadata miss.
- Parser support starts doing broader Sema record/type interpretation without
  a domain table.
- Tests are changed only to match rendered spelling behavior.

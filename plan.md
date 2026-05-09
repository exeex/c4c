# Parser Support Constexpr Type Helper Domain Tables Runbook

Status: Active
Source Idea: ideas/open/156_parser_support_constexpr_type_helper_domain_tables.md

## Purpose

Move parser-support helper APIs away from ordinary string-keyed semantic maps
and toward TextId/domain-table contracts owned by parser and Sema.

## Goal

Make parser-owned call sites prefer structured TextId or domain-key data while
turning retained string-map overloads into explicit compatibility wrappers that
cannot override structured metadata misses.

## Core Rule

Do not add new `std::unordered_map<std::string, ...>` helper contracts as the
normal semantic environment, and do not make rendered spelling win after a
structured record, typedef, or named-constant lookup has failed.

## Read First

- `ideas/open/156_parser_support_constexpr_type_helper_domain_tables.md`
- `src/c4c/Parser/parser_support.hpp`
- existing callers of:
  - `resolve_record_type_spec`
  - `eval_const_int`
  - `resolve_typedef_chain`
  - `types_compatible_p`

## Current Targets

- Parser-local constant folding helpers that can use `TextId` named constants.
- Record resolution helpers that already receive provisional record carriers or
  direct `record_def` metadata.
- Typedef and type-compatibility helpers where Sema-owned interpretation should
  be represented by domain tables instead of rendered strings.
- Tests that can prove stale rendered string maps do not override structured
  identity.

## Non-Goals

- Do not remove source lexeme parsing or literal strings.
- Do not redesign the full Sema type system.
- Do not replace the full C/C++ constant evaluator.
- Do not fold in HIR/LIR layout cleanup from other ideas.
- Do not retire unrelated parser/Sema compatibility tables.

## Working Model

- Parser owns local syntax, source lexemes, provisional record carriers,
  `TextId` named constants, and compatibility bridges needed by older tests or
  HIR proof paths.
- Sema owns semantic typedef resolution, record completion and layout authority,
  type compatibility under resolved nominal identity, and non-parser-local
  value environments.
- Compatibility string-map overloads may remain only when they are visibly
  marked as bridge APIs with comments, tests, and removal conditions.

## Execution Rules

- Keep changes in small steps that can each be built and narrowly tested.
- Prefer overloads or domain-table parameters that match data already available
  at the caller.
- Preserve behavior unless the source idea explicitly requires stale string
  fallback behavior to stop winning.
- Add tests before or with the step that changes fallback precedence.
- Treat expectation-only rewrites, named-test shortcuts, and broader Sema
  rewrites as route drift.
- For each code-changing step, run a fresh build or compile proof plus the
  narrow tests affected by that step.

## Steps

### Step 1: Inventory parser-support string-map helper contracts

Goal: identify every current string-map helper contract and classify it as
parser-local, Sema-owned, or compatibility-only.

Primary targets:
- `src/c4c/Parser/parser_support.hpp`
- direct callers of the string-map overloads named in the source idea

Actions:
- Inspect declarations and definitions for `resolve_record_type_spec`,
  `eval_const_int`, `resolve_typedef_chain`, and `types_compatible_p`.
- List caller groups by available structured data: `TextId`, provisional
  records, direct `record_def`, typedef domain data, or only rendered strings.
- Identify any helper path where a string map can currently recover after a
  structured metadata miss.
- Record the next implementation packet in `todo.md`; do not edit the source
  idea unless the source intent is wrong.

Completion check:
- `todo.md` names the first concrete implementation target and any discovered
  structured-metadata-miss fallback that must be changed.

### Step 2: Add parser-local TextId/domain-key helper entry points

Goal: give parser-owned callers structured overloads where interned names or
domain keys are already available.

Primary targets:
- named-constant evaluation around `eval_const_int`
- parser-local callers that already carry `TextId` or equivalent domain keys

Actions:
- Add the narrowest structured overloads needed by existing parser-owned
  callers.
- Keep any retained string-map overload as a compatibility bridge that delegates
  inward or is clearly isolated.
- Avoid pulling Sema semantic interpretation into parser support.

Completion check:
- Parser-owned call sites that have structured identifiers no longer need to
  construct rendered string maps for ordinary helper calls.
- The changed subset builds and the relevant constant-folding tests pass.

### Step 3: Enforce structured record and typedef precedence

Goal: prevent rendered maps from overriding structured record or typedef
identity once structured metadata is available or has failed definitively.

Primary targets:
- `resolve_record_type_spec`
- `resolve_typedef_chain`
- `sizeof`, `alignof`, and `offsetof` helper paths that consult record or
  typedef metadata

Actions:
- Route record and typedef resolution through structured carriers or
  domain-table data first.
- Make stale string-map fallback unreachable after a complete structured miss.
- Keep compatibility fallback only for callers with no structured metadata.
- Add or update tests for stale record and typedef spellings that must not win.

Completion check:
- Stale rendered record or typedef maps cannot recover a result after
  structured metadata rejects or misses it.
- The changed subset builds and the new stale-fallback tests pass.

### Step 4: Split type-compatibility interpretation from parser bridges

Goal: keep `types_compatible_p` semantic interpretation on the Sema/domain-table
side while retaining parser bridges only where required.

Primary targets:
- `types_compatible_p`
- callers for `__builtin_types_compatible_p` style checks

Actions:
- Introduce or reuse a domain-table contract for type compatibility where the
  caller needs semantic typedef or nominal identity resolution.
- Keep parser bridge behavior narrow and explicitly documented.
- Add tests covering stale typedef or nominal identity spelling where rendered
  strings should not decide compatibility.

Completion check:
- Semantic compatibility does not depend on a general rendered-string typedef
  map when structured domain information exists.
- The narrow type-compatibility tests pass with a fresh build or compile proof.

### Step 5: Mark retained string-map overloads as compatibility bridges

Goal: make any remaining string-map APIs visibly transitional and hard to use
as new semantic boundaries.

Primary targets:
- public declarations and nearby implementation comments for retained
  string-map overloads
- tests that intentionally exercise compatibility behavior

Actions:
- Add concise comments naming why each retained overload exists and what must
  be true before it can be removed.
- Prefer bridge wrappers over duplicated semantic logic.
- Ensure wrappers do not run after structured metadata misses.

Completion check:
- Every retained `unordered_map<string, ...>` parser-support overload has an
  explicit compatibility reason and removal condition.
- No new ordinary semantic caller is introduced for a string-map overload.

### Step 6: Acceptance validation and route review

Goal: prove the source idea's behavior and guardrails, not just one named case.

Actions:
- Run the supervisor-delegated build and narrow tests for parser support,
  constexpr helpers, and type compatibility.
- Include at least one stale record, typedef, or named-constant string fallback
  test that must not override structured identity.
- Escalate to broader validation if multiple helper families changed or if the
  diff crosses parser/Sema ownership boundaries.
- Check the diff for testcase-shaped matching, expectation-only progress, or
  new string-map semantic contracts.

Completion check:
- Acceptance criteria from the source idea are satisfied or remaining source
  scope is explicitly left for a follow-up runbook.

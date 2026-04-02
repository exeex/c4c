# Header Entry Surface And Function Naming Refactor

Status: Open
Last Updated: 2026-04-02

## Goal

Make parser/HIR-facing entry headers act as trustworthy navigation surfaces for
both humans and agents, and make function names describe parser/lowering
behavior consistently instead of reflecting historical implementation accidents.

This work should improve:

- first-read discoverability
- cross-translation-unit navigation
- LLM-agent entry quality
- rename safety when moving code between `.cpp` files


## Why This Idea Exists

Recent cleanup already moved parser and HIR file naming toward clearer module
boundaries:

- parser now uses `parser.hpp` plus `parser_*.cpp`
- HIR now uses `hir.hpp`, `hir_ir.hpp`, and `hir_lowering*.{hpp,cpp}`

That made the file layout better, but the declaration surface is still not as
clear as it could be:

- some headers still read like accumulation points rather than deliberate
  indexes
- large prototype blocks are not always grouped by reader task
- helper naming mixes action styles (`parse_*`, `make_*`, `lower_*`, `try_*`,
  `resolve_*`) without a clear convention boundary
- some function names describe syntax shape rather than actual parser behavior
  or contract


## Main Objective

Define and apply a naming/documentation convention so that:

1. entry headers tell a new reader where to look next
2. function names communicate behavior category at a glance
3. parser function names align with what the parser actually does at that call
   site
4. internal helper families read like one coherent API instead of organic
   growth


## Primary Targets

### 1. Entry / index headers

High-priority surfaces:

- `src/frontend/parser/parser.hpp`
- `src/frontend/hir/hir.hpp`
- `src/frontend/hir/hir_lowering.hpp`
- `src/frontend/hir/hir_lowerer_internal.hpp`

Desired outcome:

- each file declares whether it is public facade, shared internal surface, or
  implementation-only support
- declarations are grouped by responsibility, not historical insertion order
- the top of file contains a short implementation map instead of relying on a
  separate README


### 2. Parser function naming

Primary target area:

- `src/frontend/parser/parser.hpp`
- corresponding `src/frontend/parser/parser_*.cpp`

Desired outcome:

- function names line up with actual parse behavior
- probing helpers use probe-style names
- consuming helpers use consume/parse/skip names according to whether they
  commit tokens, build AST, or just advance
- conversion/build helpers use make/build/create consistently


### 3. HIR lowering function naming

Primary target area:

- `src/frontend/hir/hir_lowering.hpp`
- `src/frontend/hir/hir_lowerer_internal.hpp`
- `src/frontend/hir/hir_*.cpp`

Desired outcome:

- `build_*` means pipeline/module construction
- `lower_*` means AST/semantic lowering into HIR
- `resolve_*` means semantic lookup/substitution/fixup
- `materialize_*` means converting deferred/abstract state into concrete HIR
- `collect_*` means scan-only discovery with no lowering side effects


## Proposed Naming Rules

These rules should be applied consistently within a helper family.

### Parser-facing verbs

- `parse_*`
  - consumes tokens and produces semantic output or AST nodes
- `try_parse_*`
  - speculative parse that may fail without committing externally visible state
- `peek_*`
  - inspection only, no token consumption
- `consume_*`
  - commits token movement for a known syntax form, usually no standalone AST
- `skip_*`
  - advances through syntax intentionally without modeling it
- `expect_*`
  - validates required syntax and commits or throws

### Shared utility verbs

- `make_*`
  - small object/node construction with little policy
- `build_*`
  - larger composition or orchestration step
- `resolve_*`
  - symbol/type/overload/template identity resolution
- `normalize_*`
  - canonicalization without semantic lookup
- `collect_*`
  - read-only gathering/scanning pass
- `record_*`
  - persist discovered facts into owned state
- `finalize_*`
  - complete an object after earlier staged setup

### Predicate / capability verbs

- `is_*`
  - simple property or classification check
- `has_*`
  - presence/availability
- `can_*`
  - capability or syntactic legality
- `should_*`
  - policy decision


## Parser-Specific Renaming Principle

Do not keep a parser function name if it suggests the wrong level of behavior.

Examples of the rule:

- if a helper only checks whether a syntax form may begin, prefer `is_*` or
  `can_*`, not `parse_*`
- if a helper parses but may roll back, prefer `try_parse_*`
- if a helper only advances over ignored syntax, prefer `skip_*`
- if a helper both parses and commits semantic state, keep `parse_*`

The point is that a reader should be able to guess:

1. does this consume tokens?
2. does this construct AST?
3. can this fail softly?
4. is this a pure probe or a committed parse?


## Header Layout Rules

For each entry/index header:

1. begin with a short file-role comment
2. add a compact implementation map naming the main `.cpp` owners
3. group declarations by reader task
4. place the most important public entry points early
5. keep internal cross-TU helpers in clearly labeled sections
6. avoid long narrative comments that duplicate module history

Suggested section order for large headers:

1. public facade / top-level entry points
2. key shared structs and result carriers
3. stateful engine class declarations
4. grouped helper families
5. free helper declarations shared across translation units


## Execution Plan

### 1. Audit the current names and declaration groups

For parser and HIR lowering surfaces:

- list public headers that act as entry points
- identify prototype blocks that are currently mixed by responsibility
- identify functions whose names misdescribe behavior

Output:

- one rename table per subsystem
- one grouping plan per major header


### 2. Normalize header structure before deep renames

Reorder declarations in:

- `parser.hpp`
- `hir.hpp`
- `hir_lowering.hpp`
- `hir_lowerer_internal.hpp`

Do this first so later function renames land into stable sections.


### 3. Rename parser helpers by behavior category

Apply the verb conventions to parser helpers first because they are the primary
agent-entry surface.

Priority:

- probe vs committed parse helpers
- parse vs skip helpers
- build/make/finalize helpers


### 4. Rename HIR lowering helpers to match lowering semantics

Focus on cross-TU helpers and family consistency, not churn for its own sake.

Priority:

- `build_*` vs `lower_*`
- `collect_*` vs `record_*`
- `resolve_*` vs `materialize_*`


### 5. Validate navigability and compile health

Required checks:

- build succeeds
- no stale includes or outdated file-role comments remain
- entry headers can be read top-down as module indexes


## Non-Goals

- do not redesign the parser architecture
- do not collapse all internal headers into one giant surface
- do not rename stable external APIs unless clarity gain is real
- do not perform broad behavior changes under the guise of naming cleanup


## Acceptance Criteria

This idea is complete when:

1. parser and HIR entry headers read as intentional indexes
2. function families use a consistent verb taxonomy
3. parser helper names match actual parser behavior semantics
4. HIR lowering helper names match actual lowering lifecycle semantics
5. the build is clean after renames


## Notes To Future Agents

- Prefer semantic renames in focused batches, not repo-wide churn in one pass.
- Keep public facade headers small and obvious.
- If a rename changes meaning or scope, update the surrounding section comment at
  the same time.
- If a helper family is still too large after renaming, spin out a separate idea
  for structural decomposition instead of overloading this one.

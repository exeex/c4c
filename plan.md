# Parser Dual-Lookup Structured Identity Cleanup Runbook

Status: Active
Source Idea: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md

## Purpose

Finish the parser-side transition from rendered-string lookup identity to
structured `TextId` / `QualifiedNameKey` identity without changing downstream
rendered-name behavior.

## Goal

Make parser-owned lookup tables and caches use structured identity as the
primary semantic key, with legacy string paths retained only as compatibility
bridges and mismatch proof until stability is demonstrated.

## Core Rule

Use a dual-read / dual-write migration path: add structured storage, populate
both paths, compare results at focused proof points, keep behavior unchanged
while mismatches are possible, and remove or demote string paths only after
matching proof is stable.

## Read First

- `ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md`
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/parser.hpp`
- `tests/frontend/frontend_parser_tests.cpp`

## Current Scope

- Parser-owned value binding lookup and registration.
- `using` value import and `UsingValueAlias` structured identity handling.
- Parser-owned NTTP default-expression cache keys.
- Parser-owned template instantiation de-duplication keys.
- ID-first parser helper APIs and parser-owned call sites.
- Focused parser tests and regression proof for any behavior-facing changes.

## Non-Goals

- Do not remove AST/HIR/sema/codegen rendered string identity in this runbook.
- Do not rewrite broad HIR or sema data models as part of parser cleanup.
- Do not downgrade expectations or mark supported-path tests unsupported.
- Do not add testcase-shaped shortcuts for individual qualified names.
- Do not change generated symbol, link, diagnostic, or compatibility names
  unless equivalent behavior is explicitly proved.
- Do not switch fully from old lookup to new lookup without a dual-lookup
  proof window.

## Working Model

- Structured keys are the semantic identity for parser-owned lookup.
- Rendered strings remain bridge/output fields for AST, HIR, sema, codegen, and
  diagnostics.
- Compatibility string maps may remain populated during migration, but parser
  call sites should prefer structured lookup when the structured identity is
  already available.
- Mismatch checks should fail loudly in focused proof paths before any legacy
  path is demoted or removed.

## Execution Rules

- Keep each implementation step behavior-preserving unless the source idea
  explicitly permits a semantic change.
- Prefer small packets that can prove `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
- For any step touching HIR/sema-facing models or template instantiation
  behavior, use matching before/after regression logs and escalate to broader
  `ctest --test-dir build -j --output-on-failure` when blast radius warrants it.
- Do not delete public compatibility overloads until all relevant callers have
  migrated or the source idea is explicitly updated.
- Keep source idea edits out of routine execution; record packet progress and
  proof in `todo.md`.

## Ordered Steps

### Step 1: Inspect Current Parser Lookup Ownership

Goal: Map current string-first and structured lookup paths before editing.

Primary targets:
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/parser.hpp`

Actions:
- Inspect current storage for typedef, value, alias, template, NTTP default,
  and instantiation de-dup lookup.
- Identify bridge fields that must stay rendered strings for downstream
  compatibility.
- Choose the first narrow packet target and matching parser test focus.

Completion check:
- `todo.md` records the chosen first implementation packet, owned files,
  proof command, and any bridge paths that must not be touched.

### Step 2: Add Structured Value Binding Storage

Goal: Introduce structured value-binding lookup/register support beside the
legacy string-backed value binding path.

Primary targets:
- `src/frontend/parser/impl/parser_state.hpp`
- parser implementation files that register or query value bindings

Actions:
- Add storage keyed by structured identity, preferably `QualifiedNameKey` where
  qualified identity is available.
- Dual-write structured and legacy value binding tables during registration.
- Add structured-first lookup helpers while retaining existing string overloads
  as compatibility bridges.
- Add focused mismatch checks where both identities are naturally available.

Completion check:
- Existing value binding behavior is unchanged.
- Focused parser tests prove structured and string value lookup agree.
- Build and `frontend_parser_tests` pass.

### Step 3: Retarget Using Value Imports And Aliases

Goal: Make `using` value import and `UsingValueAlias` resolution prefer
structured identity without depending on rendered compatibility names.

Primary targets:
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/parser_state.hpp`

Actions:
- Compute the structured key once for import and alias resolution paths.
- Query structured value binding storage first, then compare with the legacy
  string result while compatibility is required.
- Treat `UsingValueAlias::target_key` as semantic identity.
- Keep `UsingValueAlias::compatibility_name` as bridge/output data only.

Completion check:
- Using value imports and namespace alias scenarios resolve through structured
  identity when possible.
- Rendered compatibility output remains unchanged.
- Focused parser proof covers namespace aliases, imports, and local shadowing.

### Step 4: Add Structured NTTP Default Cache Keys

Goal: Replace parser-owned string-shaped NTTP default-expression cache identity
with a structured cache path and compatibility mirror.

Primary targets:
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/impl/parser_state.hpp`

Actions:
- Add an explicit structured key such as `QualifiedNameKey` plus parameter
  index for NTTP default-expression tokens.
- Populate structured and legacy string caches during migration.
- Read both caches where possible and detect mismatches.
- Keep the string cache only as a temporary mirror until no parser path needs
  it as primary identity.

Completion check:
- NTTP default argument evaluation behavior is unchanged.
- Focused parser tests cover default-expression cache reuse and mismatch-free
  dual reads.
- Build and parser test proof pass.

### Step 5: Add Structured Template Instantiation De-Dup Keys

Goal: Give parser-owned template instantiation de-duplication a structured key
path beside the legacy rendered-string set.

Primary targets:
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/impl/parser_state.hpp`

Actions:
- Define a structured instantiation key that captures primary template identity
  plus structured type/value arguments.
- Populate the structured de-dup set and legacy string set in parallel.
- Check for duplicate or missing instantiations across both paths.
- Preserve final instantiated AST/HIR-facing rendered names.

Completion check:
- Template primary and specialization instantiation behavior is unchanged.
- Focused proof covers de-duplication and specialization reuse.
- Use broader validation if the implementation touches HIR/sema-facing data.

### Step 6: Prefer ID-First Parser Helpers

Goal: Move parser-owned call sites away from string-first helper APIs when
structured identity is already available.

Primary targets:
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/types/template.cpp`

Actions:
- Add or prefer ID-first alternatives for helpers such as typedef lookup,
  value lookup, value registration, typedef registration, and known-function
  lookup.
- Keep string overloads as explicit bridge helpers for downstream-facing or
  rendered-name-only call sites.
- Update parser-owned call sites incrementally, with mismatch checks where both
  identities exist.

Completion check:
- Parser-owned call sites prefer structured helpers where possible.
- Public compatibility overloads still exist for bridge users.
- Focused parser tests exercise both structured and compatibility paths.

### Step 7: Preserve Downstream Bridge Behavior

Goal: Add dual metadata only where practical while leaving downstream cleanup
to follow-on ideas.

Primary targets:
- Parser data structures that feed AST/HIR/sema/codegen rendered names

Actions:
- Identify any downstream-bound rendered identity touched by earlier steps.
- Preserve `Node::name`, `TypeSpec::tag`, rendered struct/function/global names,
  diagnostics, and link names.
- Add structured IDs beside rendered names only when the owning data model can
  carry them without a broad HIR/sema rewrite.
- Record follow-on cleanup needs in `todo.md` instead of expanding this runbook.

Completion check:
- Downstream consumers remain behaviorally unchanged.
- Any broader touched baseline has matching before/after regression logs with
  no unexpected drift.

### Step 8: Demote Parser-Owned Legacy String Paths

Goal: Remove or demote parser-owned legacy string lookup paths only after the
structured path has stable matching proof.

Primary targets:
- Parser-owned lookup tables and caches introduced or mirrored in earlier steps

Actions:
- Review mismatch proof from previous steps.
- Demote string paths from primary lookup to compatibility bridges only where
  all parser-owned call sites have structured identity available.
- Avoid deleting public compatibility overloads still needed by downstream or
  rendered-name-only callers.
- Run matching before/after regression logs when demotion changes lookup
  behavior or reduces fallback coverage.

Completion check:
- Parser-owned value binding, using alias, NTTP cache, and template de-dup
  paths have structured primary identity.
- Downstream string consumers still pass unchanged.
- Focused parser proof and required regression guard checks pass without
  unexpected baseline drift.

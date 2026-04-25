# Structured Identity Completion Audit and HIR Plan

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [95_parser_dual_lookup_structured_identity_cleanup.md](/workspaces/c4c/ideas/closed/95_parser_dual_lookup_structured_identity_cleanup.md)
- [96_sema_dual_lookup_structured_identity_cleanup.md](/workspaces/c4c/ideas/closed/96_sema_dual_lookup_structured_identity_cleanup.md)

## Goal

After ideas 95 and 96 have executed, audit whether parser and sema still have
lookup-identity leftovers that escaped their dual-lookup cleanup passes, then
inventory how HIR should adopt the same dual-write / dual-read migration
strategy.

This idea is intentionally a review-and-planning checkpoint. Its expected
outputs are follow-on source ideas:

- idea 98, only if parser/sema still have meaningful leftovers after 95/96
- idea 99, as the HIR starting plan for structured identity migration

## Why This Idea Exists

Ideas 95 and 96 deliberately avoid big-bang deletion of rendered-string lookup.
They first introduce structured lookup mirrors, dual-write registrations, and
dual-read proof so baseline drift is observable before legacy paths are
removed.

That staged strategy is safer, but it creates a natural completion question:

- did parser-owned lookup really become structured-first?
- did sema-owned lookup really become structured-first where possible?
- which remaining strings are legitimate bridge surfaces?
- which remaining strings are accidental semantic lookup paths?
- what HIR work should begin next, using the same dual-lookup discipline?

This idea provides that checkpoint so parser, sema, and HIR do not blur into
one unbounded migration.

## Preconditions

Do not execute this audit until:

- idea 95 is closed or explicitly marked complete enough for audit
- idea 96 is closed or explicitly marked complete enough for audit
- parser and sema cleanup slices have fresh focused validation
- any touched baseline has matching before/after logs where required by those
  ideas

If 95 or 96 is only partially complete, this idea may still run as a gap
analysis, but it must label any findings as pending upstream cleanup rather
than pretending the prior ideas are complete.

## Audit Scope

### Parser Completion Audit

Re-check parser tables, helper APIs, and call sites for string lookup that is
not merely bridge output.

Audit at least:

- value binding lookup and `using` value import paths
- `UsingValueAlias` resolution
- NTTP default-expression cache keys
- template instantiation de-dup keys
- parser string-first helper overloads
- parser tests that still prove only the legacy path
- any remaining use of rendered qualified names before semantic lookup

Classify each remaining string path as:

- `bridge-required`: AST/HIR/sema/codegen still consumes this spelling
- `diagnostic-only`: user-facing or debug output only
- `legacy-proof`: intentionally retained for dual-lookup mismatch checking
- `parser-leftover`: parser-owned semantic lookup that should have moved
- `blocked-by-downstream`: cannot be cleaned until a downstream data model
  carries structured identity

### Sema Completion Audit

Re-check sema lookup and validation state for string lookup that can be
structured without requiring HIR changes.

Audit at least:

- scoped local symbol tables
- global symbol and function signature maps
- C++ and reference overload sets
- enum and const-int bindings
- consteval function registry
- consteval interpreter locals and NTTP/type bindings
- canonical symbol helpers
- struct completeness and member/static-member lookup mirrors

Classify each remaining string path as:

- `bridge-required`
- `diagnostic-only`
- `legacy-proof`
- `sema-leftover`
- `blocked-by-hir`

### HIR Inventory

Inventory HIR string-keyed identity surfaces and decide how the same
dual-lookup strategy should start there.

Inspect at least:

- HIR module maps keyed by rendered struct/function/global names
- struct layout and `TypeSpec::tag` consumers
- HIR lowerer symbol lookup and method/member lookup
- compile-time engine template definition registries
- compile-time engine consteval definition registry
- template instantiation work queues and instance maps
- enum/const-int bindings used during HIR normalization
- link-name and codegen handoff boundaries

For each HIR surface, classify:

- whether a structured key already exists upstream
- whether HIR can carry a structured mirror without behavior changes
- which downstream codegen surfaces still need rendered names
- what focused proof would catch baseline drift
- whether the first HIR slice should be local to HIR or must coordinate with
  parser/sema metadata

## Required Outputs

### Review Artifact

Write an audit report under `review/`, for example:

```text
review/97_structured_identity_completion_audit.md
```

The report should include:

- parser findings grouped by classification
- sema findings grouped by classification
- HIR inventory grouped by subsystem
- proof gaps and recommended validation subsets
- recommendation on whether idea 98 is needed
- recommended scope for idea 99

### Idea 98

Create idea 98 only if parser/sema still have meaningful leftovers after 95/96.

Idea 98 should:

- target only parser/sema post-cleanup leftovers
- distinguish bridge-required strings from accidental semantic lookup
- keep the dual-lookup strategy if any legacy path remains
- avoid expanding into HIR cleanup
- include acceptance criteria based on mismatch-free dual lookup and regression
  stability

If no parser/sema leftovers remain, do not create idea 98. Record in the audit
report that idea 98 is intentionally unnecessary.

### Idea 99

Create idea 99 as the HIR starting point for structured identity migration.

Idea 99 should:

- use the same dual-write / dual-read / legacy-check strategy as 95 and 96
- start with HIR-owned lookup surfaces that can carry structured mirrors
  without changing codegen/link-name output
- explicitly preserve rendered names for codegen, diagnostics, and link names
- identify the first narrow HIR slice and focused validation subset
- avoid requiring parser or sema rewrites unless the audit proves a missing
  metadata handoff is blocking HIR

## Validation Strategy

This idea is mostly audit and planning, but any generated follow-on ideas must
be grounded in fresh evidence.

Recommended checks:

- `git grep` / `rg` inventory for string-keyed parser, sema, and HIR maps
- focused parser proof if parser leftovers are suspected
- focused sema/frontend proof if sema leftovers are suspected
- HIR-focused CTest subset for the HIR surfaces proposed in idea 99
- matching before/after regression logs if the audit includes any code or
  lifecycle cleanup beyond writing ideas and review artifacts

## Non-Goals

- no implementation cleanup in this audit idea
- no direct HIR migration before idea 99 exists
- no reopening 95 or 96 unless the audit proves they were closed with known
  blockers
- no deletion of string maps during the audit
- no expectation downgrades
- no testcase-shaped exceptions for individual names

## Acceptance Criteria

- A review artifact under `review/` classifies parser, sema, and HIR lookup
  identity state after 95/96.
- Parser/sema leftovers are explicitly separated from bridge-required and
  downstream-blocked strings.
- The audit decides whether idea 98 is required.
- If required, idea 98 exists under `ideas/open/` and targets only parser/sema
  leftovers.
- Idea 99 exists under `ideas/open/` and defines the HIR dual-lookup starting
  strategy.
- HIR cleanup remains separated from parser/sema completion work.

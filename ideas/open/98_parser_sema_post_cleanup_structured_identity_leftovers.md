# Parser/Sema Post-Cleanup Structured Identity Leftovers

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [95_parser_dual_lookup_structured_identity_cleanup.md](/workspaces/c4c/ideas/closed/95_parser_dual_lookup_structured_identity_cleanup.md)
- [96_sema_dual_lookup_structured_identity_cleanup.md](/workspaces/c4c/ideas/closed/96_sema_dual_lookup_structured_identity_cleanup.md)
- [97_structured_identity_completion_audit_and_hir_plan.md](/workspaces/c4c/ideas/closed/97_structured_identity_completion_audit_and_hir_plan.md)

## Goal

Clean up the parser and sema structured-identity leftovers found by idea 97
without expanding into HIR data-model migration.

This idea targets only parser/sema-owned semantic lookup or mirror gaps that
remain after ideas 95 and 96. It must preserve rendered-string bridges needed
by AST, HIR, consteval, diagnostics, codegen, and link-name output.

## Why This Idea Exists

The idea 97 audit found that most parser and sema lookup paths are now either
structured-first, bridge-required, diagnostic-only, legacy proof, or blocked by
HIR/type identity. It also found a small set of parser/sema-owned leftovers
specific enough to address before or alongside HIR migration.

Keeping this work separate prevents parser/sema cleanup from becoming an
unbounded HIR rewrite and preserves the dual-write / dual-read migration style
used by ideas 95 and 96.

## Parser Scope

Parser work is limited to the helper overload leftovers classified as
`parser-leftover` in `review/97_structured_identity_completion_audit.md`.

Audit evidence:

- `parser_support.hpp` still exposes string-keyed helper overloads for
  const-int evaluation, typedef-chain resolution, and type compatibility.
- `impl/support.cpp::eval_const_int` still has a string-map overload that
  performs lookup by `n->name`.
- A TextId-based const-int helper already exists, so this is parser cleanup
  rather than a downstream blocker.

Expected direction:

- Prefer TextId/structured helper variants for parser-owned call sites where a
  stable ID is available.
- Keep string helper overloads as explicit compatibility bridges while
  downstream callers still provide only rendered names.
- Add mismatch or fallback proof where both structured and legacy inputs are
  available.
- Do not touch parser struct/tag maps, template rendered names, or
  `TypeSpec::tag` outputs in this idea.

## Sema Scope

Sema work is limited to leftovers classified as `sema-leftover` in
`review/97_structured_identity_completion_audit.md`.

Audit evidence:

- Enum variant structured mirrors exist but are not populated for global and
  local enum variant bindings.
- Template NTTP placeholders in validation local scopes are string-only.
- Template type-parameter tracking in validation remains name-set based.
- Consteval call NTTP bindings expose text/key maps but only populate the
  legacy name map.
- Type-binding text mirror outputs are declared and threaded but deliberately
  unused.

Expected direction:

- Populate enum variant text/key mirrors where the AST provides stable
  identity, keeping rendered maps for compatibility.
- Add dual-write / dual-read handling for sema-owned template NTTP placeholders
  when parameter identity is available.
- Add structured or text-id mirrors for sema-owned template type-parameter
  checks without changing HIR-facing type tags.
- Populate consteval NTTP binding text/key mirrors where call metadata supports
  it and compare against the legacy name map.
- Either populate type-binding text mirrors or remove the unused mirror
  plumbing if source evidence proves they are only dead compatibility
  scaffolding.

## Out Of Scope

- HIR module maps, compile-time engine registries, and HIR lowerer symbol maps.
- `TypeSpec::tag`, HIR struct layout identity, and codegen type declaration
  names.
- Struct completeness, member lookup, static-member lookup, and method owner
  identity that remain blocked by HIR/type identity.
- Canonical symbol string names used for ABI-style rendering or diagnostics.
- Direct deletion of string maps before a stable dual-lookup proof window.
- Test expectation downgrades or testcase-shaped exceptions.

## Validation Strategy

At minimum for each implementation slice:

- run `cmake --build --preset default`
- run a focused frontend/parser/sema CTest subset covering the touched path

For enum, consteval, or template-binding slices:

- use matching before/after logs for the same focused CTest command when the
  behavior crosses parser/sema boundaries
- compare with the repo regression guard if the supervisor requires baseline
  drift proof
- broaden only when the touched path reaches HIR, compile-time engine behavior,
  or emitted codegen

Focused proof candidates:

- parser const-int helper calls with TextId and legacy fallback coverage
- enum constants in global, local, namespace, and record-adjacent contexts
- template NTTP placeholders used in sema validation
- template type-parameter checks and cast validation
- consteval calls with explicit and default NTTP values
- type-binding mirror lookups in consteval/template substitution paths

## Suggested Execution Decomposition

1. Inventory current parser/sema helper callers and separate parser-owned call
   sites from downstream string-only bridges.
2. Prefer TextId/structured parser helper overloads for parser-owned const-int
   evaluation and add paired legacy checks where possible.
3. Populate enum variant text/key mirrors for global and local sema bindings.
4. Add structured/text-id mirrors for sema template NTTP validation placeholders.
5. Add structured/text-id mirrors for sema template type-parameter validation.
6. Populate consteval NTTP binding text/key maps and dual-read lookup.
7. Resolve unused type-binding text mirror plumbing by either populating it or
   deleting it as proven-dead scaffolding.
8. Remove or demote parser/sema-owned legacy fallbacks only after mismatch-free
   focused proof and regression stability.

## Acceptance Criteria

- Parser-owned helper call sites prefer structured or TextId inputs where that
  identity is available.
- String parser helper overloads are either explicit compatibility bridges or
  have been demoted after proof.
- Sema enum variant mirrors are populated and checked for global and local
  bindings where stable identity is available.
- Sema template NTTP/type-parameter validation has structured or TextId mirrors
  where source metadata supports them.
- Consteval NTTP binding lookup populates and checks text/key mirrors when
  metadata is available.
- Type-binding text mirror plumbing is either meaningful and proven, or removed
  as dead scaffolding.
- HIR/type/codegen rendered-name bridges remain behaviorally unchanged.
- Focused parser/sema proof passes with no expectation downgrades.

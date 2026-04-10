# Frontend Helper Extraction For Parser And HIR

Status: Open
Last Updated: 2026-04-10

## Goal

Extract two growing frontend responsibility clusters into smaller helper
surfaces:

- parser namespace-qualified name resolution and visibility lookup
- HIR expression lowering subflows that currently live inside very large
  dispatcher-style functions

The intent is to make parser entrypoints read more like grammar coordination
and HIR lowering entrypoints read more like orchestration, instead of carrying
large amounts of lookup policy and call/temporary materialization inline.

## Why This Idea Exists

The parser currently has a coherent namespace model, but the implementation is
spread across several closely related methods that all walk the same context
stack and alias maps with slightly different lookup rules.

Representative responsibilities currently live together in
`src/frontend/parser/parser_core.cpp`:

- qualified-name canonicalization
- visible value lookup
- visible type lookup
- visible concept lookup
- namespace-context creation and traversal
- canonical-name synthesis for nested contexts

This is still maintainable today, but it is becoming harder to change one
lookup policy without re-reading the whole family.

HIR lowering now has a similar shape problem. In particular,
`src/frontend/hir/hir_expr.cpp` contains very large functions that mix:

- expression-kind dispatch
- overload and template resolution decisions
- temporary materialization
- reference/rvalue-reference argument lowering
- operator and method-call special cases

That shape is still workable for incremental feature work, but it is becoming
harder to reason about and it likely contributes to expensive optimized
single-TU builds.

## Main Objective

Introduce helper layers that preserve current frontend semantics while moving
repeated policy and subflow logic behind named operations.

For parser lookup, examples include:

- `NamespaceResolver`
- `VisibleNameLookup`
- `QualifiedNameResolver`

For HIR lowering, examples include helper seams such as:

- `lower_var_expr`
- `lower_member_expr`
- `try_lower_ctor_call`
- `try_lower_method_call`
- `try_lower_template_call`
- `lower_call_args`

The goal is not a broad frontend rewrite. The goal is to turn repeated
stack-walk / alias-check / canonical-name code and large HIR lowering subflows
into a smaller number of named operations with one place to maintain policy.

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/hir/hir_expr.cpp`
- nearby HIR helper headers or `.cpp` files if the extraction naturally belongs
  there
- nearby parser helper files if the extraction naturally belongs there

## Candidate Changes

1. Centralize namespace-stack traversal for visible-name lookup.
2. Centralize `parent_id + "::" + name` key construction and canonical-name
   synthesis.
3. Separate "lookup value / type / concept" policy from the mechanics of
   walking namespace ancestry.
4. Reduce duplicated alias handling between visible-value and visible-type
   resolution.
5. Make the parser core read more like grammar coordination and less like a
   symbol-table utility module.
6. Turn `hir_expr.cpp` into a dispatcher plus named lowering helpers instead of
   keeping major expression categories inline.
7. Split large call-lowering flows into smaller helpers for constructor calls,
   method calls, template calls, operator-call paths, and argument
   materialization.
8. Centralize repeated ref/rvalue-ref argument lowering and temporary
   materialization logic used by multiple HIR call sites.
9. Prefer extracting HIR helpers along semantic seams rather than line-count
   cuts, so each helper owns one lowering responsibility.

## HIR Extraction Shape

The intended HIR slice is not "split every function into tiny fragments." The
intended shape is:

1. Keep `lower_expr(...)` as the entrypoint and dispatcher.
2. Move heavy expression categories such as variable/name references, member
   access, and call lowering into dedicated helpers.
3. Keep `lower_call_expr(...)` as a call coordinator only if it becomes a thin
   orchestrator; otherwise split it further so constructor, method, template,
   and fallback call paths each have a named helper.
4. Move repeated local lambdas or repeated lowering snippets into reusable
   private helpers when they encode stable policy.
5. Prefer moving heavy non-trivial implementations out of large headers and
   into `.cpp` helpers where that improves incremental compile cost without
   hurting clarity.

## Constraints

- preserve current parser semantics and rollback behavior
- preserve current HIR lowering semantics and emitted IR shape
- keep the helper layer frontend-local for the first slice
- do not mix this idea with backend symbol or target work
- avoid bundling unrelated parser performance experiments into the same plan
- avoid combining helper extraction with unrelated data-structure swaps unless
  the helper boundary clearly needs them

## Validation

At minimum:

- existing parser regression suite
- existing HIR/frontend regression suite
- namespace-qualified lookup cases
- `using`-driven visibility cases
- dependent-name / concept-name parser coverage that currently relies on the
  existing resolution behavior
- representative HIR call/member/template lowering cases that exercise the
  extracted helper paths

## Non-Goals

- no backend refactor work
- no HIR or sema symbol-table redesign in the first slice
- no STL-container replacement by itself unless needed for the helper boundary
- no attempt to solve all compile-time issues in one pass; this idea is about
  helper extraction and clearer frontend seams first

# Parser Disambiguation Matrix For Type-Id And Declarators

Status: Open
Last Updated: 2026-04-09

## Goal

Systematically probe and harden the parser's declaration-versus-expression
disambiguation around C++ type-ids, qualified/dependent type spellings, and
parenthesized declarator forms, instead of continuing to rely on one-off fixes
for each newly exposed cast or local-declaration shape.

## Why This Idea Exists

The active C-style-cast follow-up review in
`ideas/open/43_c_style_cast_reference_followups_review.md` repeatedly exposed
the same deeper parser pressure point:

- the parser must first decide whether a token sequence is a type head,
  declarator head, or expression head
- that decision becomes fragile when `template-id`, `typename`, `::`, and
  parenthesized pointer/reference/member-pointer declarators are mixed together
- once the initial split goes the wrong way, later stages never get a chance to
  see the intended cast target or local declaration shape

Idea 43 has already produced multiple concrete fixes in this area, including
template-id function-pointer cast targets and qualified/dependent owner lookup.
That pattern suggests a dedicated parser-disambiguation follow-up should exist
as its own idea.

## Problem Surface To Review

### 1. Type-head recognition is still heuristic-heavy

- unresolved identifiers can be treated as type heads by local heuristics
- qualified and dependent owners such as `ns::T`, `::ns::T`,
  `typename Holder<T>::Type`, and
  `typename Holder<T>::template Rebind<U>::Type` are recognized through several
  partially overlapping paths
- parser behavior can drift between statement parsing, cast parsing, and other
  type-id contexts because each path performs slightly different token checks

### 2. Parenthesized declarator recognition is a nearby weak point

- `(*)`, `(&)`, `(&&)`, and `C::*` forms are routed through special-case logic
- template-id return types and parameter types have already shown that one
  suffix fix can leave neighboring suffix shapes unreviewed
- member-pointer and cv-qualified function-pointer forms are the next likely
  hole family after the current active Step 3 work

### 3. Declaration-versus-expression splitting is not centralized enough

- several call sites use local lookahead or ad hoc "looks like a type" rules
- the same token sequence can be judged differently depending on whether it
  appears in a cast, a local declaration, or another parenthesized context
- there is not yet one shared "can this form a type-id / declarator?" probe
  that higher-level parse routines can reuse

## Proposed Matrix

When this idea is activated, drive review by combining:

1. owner spelling
   `T`, `ns::T`, `::ns::T`, `typename H<T>::Type`,
   `typename H<T>::template Rebind<U>::Type`
2. declarator shape
   `*`, `&`, `&&`, `* const`, `(*)`, `(&)`, `(&&)`, `C::*`,
   `(*)(...)`, `(&)(...)`, `(&&)(...)`, `(C::*)(...)`
3. parse context
   C-style cast target, local declaration, parameter declaration,
   parenthesized type-id consumer, and ambiguous statement context

The goal is not exhaustive language completeness on day one. The goal is to
turn the current parser weak spots into an explicit reduction matrix so that
new holes are found by pattern coverage rather than by accident.

## Proposed Approach

1. Add focused parse-only reductions for the highest-risk matrix cells,
   especially member-pointer and cv-qualified function-pointer forms.
2. Group failures by earliest parser decision point:
   type-head recognition, declarator recognition, or declaration/expression
   split.
3. Evaluate whether the current heuristics should be replaced or wrapped by a
   shared tentative parser probe for "type-id or declarator head" detection.
4. Compare the resulting design against Clang's parser structure, especially
   its tentative parsing and token annotation flow under
   `ref/llvm-project/clang/lib/Parse/`.

## Likely Deliverables

- a regression matrix covering ambiguous type-id and declarator patterns
- a list of parser split points that still rely on duplicated heuristics
- one or more focused parser cleanups that centralize ambiguity handling rather
  than extending scattered lookahead checks

## Non-Goals

- broad sema or HIR cleanup unrelated to parser-side disambiguation
- turning this idea into a full parser architecture rewrite in one activation
- silently absorbing the still-active cast follow-up plan into a broader parser
  initiative before idea 43 reaches a clean stopping point

## Activation Hint

Activate this idea after the current cast-follow-up plan either completes Step
3 or clearly shows that the next unresolved bug is no longer a narrow cast
shape but a repeated parser-disambiguation pattern spanning multiple contexts.

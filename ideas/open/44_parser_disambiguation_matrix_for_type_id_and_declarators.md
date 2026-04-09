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

## Suggested Test Asset Strategy

Treat this idea as a test-surface build-out first, not only as a parser-fix
initiative. The immediate deliverable should be a reusable matrix-driven test
set that can expose parser split failures systematically.

### 1. Generate cases from structured metadata

Keep the matrix dimensions in machine-readable form and generate the first-pass
test corpus from them instead of hand-writing every file.

Recommended inputs:

- owner spelling family
- declarator family
- parse context
- intended verification mode
- optional risk tag such as `high_risk`, `member_pointer`, or
  `dependent_typename`

Recommended generator outputs:

- a flat pattern manifest for auditing coverage
- owner-grouped and declarator-grouped views for batch execution
- generated `.cpp` test files under a dedicated matrix folder

Current seed assets already exist in:

- `scripts/generate_parser_disambiguation_matrix.py`
- `ideas/open/44_parser_disambiguation_matrix_patterns.txt`
- `ideas/open/44_parser_disambiguation_matrix_patterns_by_owner.txt`
- `ideas/open/44_parser_disambiguation_matrix_patterns_by_declarator.txt`

### 2. Separate test layers by validation strength

Do not force every generated case to become a full runtime test on day one.
Split the matrix into explicit validation tiers:

1. `parse_only`
   - expected result: `build/c4cll --parse-only` succeeds
   - use for broad first-pass coverage of ambiguous forms
2. `compile_positive`
   - expected result: frontend pipeline succeeds through sema/HIR/lowering
   - use when the syntax is stable but runtime semantics are not yet worth
     asserting
3. `runtime_positive`
   - expected result: generated program builds, runs, and returns `0`
   - use for cases where aliasing, value category, field access, or dispatch
     semantics matter and can be checked cheaply

The first activation should bias toward `parse_only` breadth. Promote selected
high-risk families to `runtime_positive` only after the parser split behavior
is stable enough to justify stronger assertions.

### 3. Use a dedicated generated test area

Keep the generated matrix separate from hand-written reductions so the review
surface stays understandable.

Suggested layout:

- `tests/cpp/internal/generated/parser_disambiguation_matrix/`
- optional subfolders such as:
  - `parse_only/`
  - `compile_positive/`
  - `runtime_positive/`

Suggested filename shape:

- `owner_simple__decl_pointer__ctx_cast__parse_only.cpp`
- `owner_dependent_typename__decl_member_function_pointer__ctx_local_decl__runtime_positive.cpp`

Each generated file should carry a short metadata header, for example:

```cpp
// owner: dependent_typename
// declarator: member_function_pointer
// context: local_declaration
// verification: runtime_positive
```

### 4. Keep runtime-positive generation template-driven

Python should be allowed to generate `.cpp` files directly, but runtime
validation should be template-driven rather than fully free-form.

Recommended split of responsibilities:

- Python generator:
  - expands the matrix
  - chooses the output path and metadata
  - fills a known code template family
- subagents:
  - upgrade selected generated cases from parse-only or compile-only into
    runtime-verifying forms
  - refine the body logic when a family needs semantic assertions that are too
    specific for a generic generator template

This keeps large-scale generation deterministic while still allowing targeted
semantic strengthening where it matters.

### 5. Prefer `main() == 0` for runtime-positive pass/fail

For runtime-positive cases, align with the existing internal test style:

- `main()` returns `0` when behavior matches the expectation
- any mismatch returns nonzero

This avoids inventing a separate harness and keeps generated tests compatible
with the current `ctest` model.

Suitable runtime-positive checks include:

- reference aliasing updates the original storage
- overload resolution picks the expected `&` or `&&` path
- field access through casted references reads or writes the expected storage
- function pointer or member pointer targets parse, compile, and invoke the
  intended callable shape

### 6. Batch work by family, not by arbitrary file slices

When using subagents or other parallel workers, assign ownership by matrix
family instead of by random filename chunks.

Recommended worker packets:

- one owner family across all declarators for parse-only generation
- one declarator family across all owners for parser-focused suffix review
- one promoted high-risk family for runtime-positive upgrades

Good first runtime-positive promotion targets:

- `&` and `&&` reference families
- field/member access through casted references
- function-pointer and member-pointer declarators
- qualified or dependent owner spellings already known to have stressed parser
  lookup

Less urgent runtime-positive targets:

- pure type-id consumer cases such as `sizeof(...)` that mainly test parser
  classification rather than runtime semantics

### 7. Record explicit expectations in the manifest

The generated matrix should not only say what syntax to emit. It should also
record what each case is meant to prove.

Recommended manifest fields:

- stable case id
- owner family
- declarator family
- context
- verification tier
- expected earliest stage
- source template family
- high-risk yes/no
- notes or follow-up tags

That metadata will make it easier to:

- batch-generate files
- assign subagent work packets
- identify which failures are parser-only versus later semantic regressions
- promote a case from parse-only to runtime-positive without renaming the whole
  concept

## Proposed Approach

1. Add focused parse-only reductions for the highest-risk matrix cells,
   especially member-pointer and cv-qualified function-pointer forms.
2. Group failures by earliest parser decision point:
   type-head recognition, declarator recognition, or declaration/expression
   split.
3. Stand up the generated matrix test area and prove that broad parse-only
   coverage can be produced mechanically from the matrix manifest.
4. Promote one or two high-risk families to runtime-positive templates that use
   `main() == 0` as the correctness signal.
5. Evaluate whether the current heuristics should be replaced or wrapped by a
   shared tentative parser probe for "type-id or declarator head" detection.
6. Compare the resulting design against Clang's parser structure, especially
   its tentative parsing and token annotation flow under
   `ref/llvm-project/clang/lib/Parse/`.

## Likely Deliverables

- a regression matrix covering ambiguous type-id and declarator patterns
- a generated test corpus under a dedicated matrix test folder
- manifest-driven grouping that supports owner-based and declarator-based batch
  work
- a first wave of runtime-positive matrix cases that validate behavior with
  `main() == 0`
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

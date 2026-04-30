# HIR Structured Record Template Lookup Authority Cleanup

Status: Active
Source Idea: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md

## Purpose

Move HIR owner, member, method, static-member, and template lookup away from
rendered strings where structured carriers already exist.

## Goal

Make HIR record, member, method, static-member, and template lookup paths
structured-primary while keeping rendered names only for diagnostics, debug
output, display, generated spelling, or explicit compatibility fallback.

## Core Rule

Do not let rendered function, record, template, owner, or member spelling
decide HIR semantic lookup when `NamespaceQualifier`, declaration `TextId`,
`ModuleDeclLookupKey`, `HirRecordOwnerKey`, or an existing by-owner map can
identify the entity.

## Read First

- `ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md`
- HIR definitions and callers of `parse_scoped_function_name`
- HIR out-of-class method ownership paths that inspect `fn->name` or `n->name`
- HIR scoped static-member lookup paths
- HIR template struct primary and specialization lookup paths
- HIR struct method, static-member, and member-symbol lookup maps
- Existing frontend/HIR lookup tests for namespace-qualified records,
  out-of-class methods, scoped static members, templates, and member symbols

## Current Targets

- `parse_scoped_function_name`
- out-of-class method owner lookup derived from rendered names
- scoped static-member lookup derived from rendered names
- template struct primary and specialization lookup keyed by rendered spelling
- struct method lookup, static-member lookup, and member-symbol lookup maps
- structured carriers including `NamespaceQualifier`, declaration `TextId`,
  `ModuleDeclLookupKey`, `HirRecordOwnerKey`, and by-owner maps

## Non-Goals

- Do not remove final mangled names, link names, generated specialization
  names, diagnostics, dump text, labels, asm text, or display spelling.
- Do not rewrite parser AST production; parser payload cleanup belongs to
  idea 134.
- Do not bulk delete HIR compatibility string fields before all consumers have
  structured replacements.
- Do not fold Sema owner/static-member cleanup into this plan; that belongs to
  closed idea 135.
- Do not add testcase-shaped shortcuts as a substitute for structured HIR
  lookup authority.

## Working Model

- Structured carriers that already exist should be the HIR semantic lookup
  authority.
- Rendered maps may remain temporarily as compatibility mirrors, diagnostics,
  debug output, or explicit fallback when structured producer data is absent.
- Parity checks should become guards or mismatch visibility, not proof that a
  rendered map is allowed to decide lookup when a structured key exists.
- Tests should cover nearby same-feature behavior, especially owners,
  templates, or members whose rendered spelling can collide across namespace
  or record contexts.

## Execution Rules

- Keep source idea 136 unchanged unless durable source intent changes.
- Update `todo.md` for routine packet progress and proof.
- Prefer small, testable HIR/frontend slices over broad frontend rewrites.
- When retaining a rendered-name branch, classify it as display,
  diagnostic/debug, generated spelling, compatibility fallback, or legacy raw
  input fallback.
- Add focused tests before claiming that a rendered HIR lookup path is no
  longer semantic authority.
- Escalate for review if a slice mostly rewrites expectations, weakens tests,
  or proves only one named case while nearby same-feature cases remain
  unexamined.

## Step 1: Classify HIR Lookup String Authority

Goal: produce a precise map of HIR lookup paths where rendered spelling still
affects semantic owner, member, static-member, method, or template decisions.

Primary target:
- definitions and callers of `parse_scoped_function_name`
- out-of-class method owner lookup paths using `fn->name` or `n->name`
- scoped static-member lookup paths
- template struct primary and specialization lookup paths
- struct method, static-member, and member-symbol lookup maps

Actions:
- Trace each suspicious HIR path from parser/HIR inputs to the lookup result.
- Identify available structured carriers for each path, including
  `NamespaceQualifier`, declaration `TextId`, `ModuleDeclLookupKey`,
  `HirRecordOwnerKey`, direct declaration links, and by-owner maps.
- Classify rendered-name reads as `display-only`, `diagnostic/debug-only`,
  `generated spelling`, `compatibility fallback`, `parity mirror`, or
  `semantic string authority`.
- Record the first owned implementation packet in `todo.md`; do not edit
  implementation files in this classification step unless explicitly
  delegated.

Completion check:
- `todo.md` contains the classification map and names the first helper or
  lookup family whose authority will be changed.
- No implementation files are changed by this step unless the executor packet
  is explicitly delegated.

## Step 2: Make Out-of-Class Method Ownership Structured-Primary

Goal: stop out-of-class method owner lookup from deriving semantic ownership
from rendered function or record spelling when structured owner data exists.

Primary target:
- `parse_scoped_function_name`
- method owner lookup and validation callers
- HIR lowering paths that use `fn->name` or `n->name` for owner recovery
- tests for namespace-qualified and scoped method definitions

Actions:
- Prefer `NamespaceQualifier`, declaration `TextId`, `ModuleDeclLookupKey`,
  `HirRecordOwnerKey`, or direct declaration/type links when finding the owner
  record for out-of-class methods.
- Demote rendered method or record spelling to final display, diagnostics,
  debug output, generated spelling, or explicit compatibility fallback.
- Add mismatch visibility where structured owner identity and rendered spelling
  can disagree.
- Add focused frontend/HIR tests that distinguish the same rendered method or
  record spelling under different namespace or owner contexts.

Completion check:
- Out-of-class method lookup succeeds or fails according to structured owner
  identity, not rendered method or record spelling.
- Focused proof covers the touched HIR/frontend test target.

## Step 3: Make Scoped Static-Member Lookup Structured-Primary

Goal: make scoped static-member lookup use structured owner and member
identity where available instead of rendered owner/member strings.

Primary target:
- scoped static-member lookup callers
- static-member declaration and value lookup maps
- tests for scoped static members and colliding member names under different
  owners

Actions:
- Prefer owner keys, member `TextId`, direct declaration links, direct type
  links, or by-owner maps for scoped static-member lookup.
- Demote rendered owner/member names to diagnostics, debug output, generated
  spelling, or narrow fallback when structured identity is unavailable.
- Add mismatch visibility where structured owner/member identity conflicts
  with rendered spelling.
- Add or update focused HIR/frontend tests for scoped static members.

Completion check:
- Static-member tests prove lookup is keyed by structured owner/member
  identity where available.
- Remaining rendered-name static-member paths are guarded as compatibility
  fallback or non-semantic output.

## Step 4: Make Template Struct Lookup Structured-Primary

Goal: make template struct primary and specialization lookup prefer structured
owner and template identity over rendered template or specialization names.

Primary target:
- template struct primary lookup
- template struct specialization lookup
- owner-parity checks around template lookup
- tests for namespace-qualified templates and specializations under colliding
  rendered names

Actions:
- Prefer declaration `TextId`, `ModuleDeclLookupKey`, `HirRecordOwnerKey`,
  template identity, and by-owner maps for template primary and specialization
  lookup.
- Convert rendered template maps to compatibility mirrors or explicit fallback
  paths when structured producer data is absent.
- Preserve generated specialization names as emitted or display spelling, not
  semantic lookup authority.
- Add mismatch visibility where rendered and structured template owner
  identity disagree.
- Add focused tests for template primaries and specializations under owners
  with colliding rendered names.

Completion check:
- Template primary and specialization lookup are structured-primary when
  structured identity exists.
- Remaining rendered template maps cannot silently override structured lookup.

## Step 5: Make Struct Method and Member-Symbol Maps Structured-Primary

Goal: make struct method lookup, static-member lookup, and member-symbol lookup
return results from structured owner-key maps rather than only checking
structured maps for parity while returning rendered-map results.

Primary target:
- struct method lookup maps
- static-member lookup maps
- member-symbol lookup maps
- by-owner maps keyed by `HirRecordOwnerKey` or equivalent structured owner

Actions:
- Prefer by-owner structured maps as the returned semantic lookup result.
- Demote rendered tag/member maps to compatibility mirrors, diagnostics, debug
  output, or fallback when structured owner data is absent.
- Turn parity-only checks into assertions, mismatch counters, or explicit
  guardrails that cannot let rendered maps decide against structured keys.
- Add focused tests for methods, static members, and member symbols under
  owners with colliding rendered names.

Completion check:
- HIR lookup callers receive results selected by structured owner/member keys
  where available.
- `rg` over touched HIR code shows remaining rendered map reads are classified
  as compatibility fallback, parity guard, or non-semantic output.

## Step 6: Validation and Acceptance

Goal: prove HIR lookup authority is structured-primary without regressing
nearby frontend behavior.

Actions:
- Run the focused build or test target needed for touched HIR/frontend code.
- Run targeted CTest selectors for namespace-qualified records,
  out-of-class methods, scoped static members, template lookup, and
  member-symbol lookup.
- Run broader frontend/HIR coverage if multiple lookup families changed or
  compatibility fallback behavior was narrowed across shared helpers.
- Record exact commands and results in `todo.md` and `test_after.log`.

Completion check:
- Focused and required broader validation are green.
- Acceptance criteria from idea 136 are satisfied or any remaining fallback
  work is captured as a separate follow-up idea.

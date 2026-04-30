# Sema Structured Owner Static Member Lookup Cleanup

Status: Active
Source Idea: ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md

## Purpose

Move Sema owner and static-member lookup away from rendered method, record, or
member spelling where parser/AST structured identity is already available.

## Goal

Make Sema record-owner, method-owner, static-member, and nearby unqualified
lookup paths structured-primary while preserving rendered strings only for
diagnostics, display, debug output, or explicitly named compatibility fallback.

## Core Rule

Do not let rendered owner/member spelling decide Sema semantic lookup when
`SemaStructuredNameKey`, namespace context ids, AST `TextId` fields, direct
declaration links, or direct type links can identify the entity.

## Read First

- `ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md`
- Sema code defining or calling `resolve_owner_in_namespace_context`
- Sema code defining or calling `enclosing_method_owner_struct`
- Sema code defining or calling `lookup_struct_static_member_type`
- Sema unqualified variable/member lookup fallback paths that inspect
  `Node::name` or `Node::unqualified_name`
- AST/parser structures that provide owner, member, or unqualified-name
  `TextId` identity to those Sema consumers
- Focused frontend/Sema tests for namespace-qualified methods, static members,
  and unqualified lookup

## Current Targets

- `resolve_owner_in_namespace_context`
- `enclosing_method_owner_struct`
- `lookup_struct_static_member_type`
- unqualified variable lookup fallback that can recover from `n->name`
- structured AST spelling authority such as `n->unqualified_name`
- Sema ownership lookup keys and namespace context ids

## Non-Goals

- Do not rewrite parser name production.
- Do not redesign `ResolvedTypeTable`.
- Do not remove diagnostic, dump, debug, or display spelling from AST nodes.
- Do not do broad `std::string` cleanup outside Sema owner/member lookup
  authority.
- Do not fold HIR lookup-table authority cleanup into this plan; that belongs
  to idea 136.
- Do not add testcase-shaped special cases as a substitute for structured
  Sema lookup.

## Working Model

- Parser/AST spelling fields may remain as final display or compatibility
  payloads, but Sema should prefer structured identity when the producer
  supplied it.
- Compatibility fallback is acceptable only when structured identity is
  genuinely absent, and the fallback must be explicit, narrow, and covered by
  nearby tests.
- Mismatch handling should make it visible when rendered spelling and
  structured identity disagree instead of silently letting the rendered string
  win.
- Tests should cover nearby same-feature behavior, especially owners or
  members whose rendered spelling could mask identity mismatches.

## Execution Rules

- Keep source idea 135 unchanged unless durable source intent changes.
- Update `todo.md` for routine packet progress and proof.
- Prefer small, testable Sema slices over broad frontend rewrites.
- When retaining a rendered-name branch, classify it in code or tests as
  display, diagnostic/debug, generated spelling, or compatibility fallback.
- Add focused tests before claiming that a rendered Sema lookup path is no
  longer semantic authority.
- Escalate for review if a slice mostly rewrites expectations, weakens tests,
  or proves only one named case while nearby same-feature cases remain
  unexamined.

## Step 1: Classify Sema Owner and Static-Member String Authority

Goal: produce a precise map of Sema owner/member lookup paths where rendered
spelling still affects semantic ownership or static-member decisions.

Primary target:
- Sema definitions and callers of `resolve_owner_in_namespace_context`
- Sema definitions and callers of `enclosing_method_owner_struct`
- Sema definitions and callers of `lookup_struct_static_member_type`
- unqualified variable/member lookup fallback paths that inspect AST spelling

Actions:
- Trace parser/AST fields into each suspicious Sema lookup path.
- Identify the available structured carriers for each path, including
  `SemaStructuredNameKey`, namespace context ids, AST `TextId` fields, direct
  declaration links, and direct type links.
- Classify remaining rendered-name reads as `display-only`,
  `diagnostic/debug-only`, `generated spelling`, `fallback-only`, or
  `semantic string authority`.
- Record the first owned implementation packet in `todo.md`; do not edit
  implementation files in this classification step unless explicitly
  delegated.

Completion check:
- `todo.md` contains the classification map and names the first helper or
  lookup family whose authority will be changed.
- No implementation files are changed by this step unless the executor packet
  is explicitly delegated.

## Step 2: Make Namespace Owner Resolution Structured-Primary

Goal: make owner lookup in namespace context use structured keys and namespace
context ids as semantic authority instead of reparsed or rendered owner
spelling.

Primary target:
- `resolve_owner_in_namespace_context`
- owner lookup callers for namespace-qualified records and methods
- tests for namespace-qualified owner resolution

Actions:
- Prefer `SemaStructuredNameKey`, namespace context ids, declaration `TextId`,
  or direct declaration/type links for owner resolution.
- Demote rendered owner spelling to diagnostics, debug output, or a named
  compatibility fallback when structured identity is absent.
- Add mismatch visibility where structured and rendered owner identity can
  disagree.
- Add or update focused Sema/frontend tests for namespace-qualified records
  and methods with colliding or misleading rendered spelling.

Completion check:
- Namespace owner resolution tests prove structured identity decides the
  owner when rendered spelling would be ambiguous or misleading.
- Remaining rendered owner lookup branches are named compatibility fallback
  or non-semantic display/debug uses.

## Step 3: Make Enclosing Method Owner Lookup Structured-Primary

Goal: stop out-of-class or scoped method-owner lookup from deriving semantic
ownership from rendered method or record spelling when structured owner data is
available.

Primary target:
- `enclosing_method_owner_struct`
- method owner lookup and validation callers
- tests for namespace-qualified and scoped method definitions

Actions:
- Prefer structured owner keys, namespace context ids, AST name `TextId`s, or
  direct declaration links when finding the enclosing owner record.
- Keep rendered method/record spelling only as final display, diagnostics,
  debug output, or explicit fallback.
- Add tests that distinguish same rendered method or record spelling under
  different owners or namespace contexts.

Completion check:
- Method owner lookup succeeds or fails according to structured owner identity,
  not just rendered method/record spelling.
- Focused proof covers the touched Sema/frontend test target.

## Step 4: Make Static-Member Lookup Structured-Primary

Goal: make static-member type lookup use structured owner/member identity when
available instead of rendered owner/member strings.

Primary target:
- `lookup_struct_static_member_type`
- static-member lookup callers
- tests for scoped static members and colliding member names under different
  owners

Actions:
- Prefer structured owner keys, member `TextId`, direct declaration links, or
  direct type links for static-member lookup.
- Demote rendered member names to diagnostics, debug output, or narrow
  fallback when structured identity is unavailable.
- Add mismatch visibility where structured owner/member identity conflicts
  with rendered spelling.
- Add or update focused Sema/frontend tests for scoped static members.

Completion check:
- Static-member tests prove lookup is keyed by structured owner/member identity
  where available.
- Remaining rendered-name static-member paths are guarded as compatibility
  fallback or non-semantic output.

## Step 5: Tighten Unqualified Name Fallbacks

Goal: make unqualified variable/member lookup recover from structured AST
unqualified-name authority before falling back to rendered `Node::name`
spelling.

Primary target:
- unqualified variable lookup fallback paths
- AST `Node::unqualified_name`
- AST `Node::name`
- tests for unqualified variable/member lookup under ambiguous rendered names

Actions:
- Prefer AST unqualified-name `TextId` or direct symbol/declaration links when
  available.
- Keep `Node::name` fallback explicit, narrow, and covered by tests for
  producer paths that still lack structured identity.
- Avoid changing final AST display spelling except where necessary to remove
  semantic reliance on compatibility spelling.
- Add tests for unqualified lookup cases where rendered full spelling could
  previously mask the intended unqualified identity.

Completion check:
- Unqualified lookup tests distinguish structured unqualified-name identity
  from rendered full spelling.
- `rg` over touched Sema code shows remaining `Node::name`-based lookup paths
  are classified as fallback-only or non-semantic.

## Step 6: Validation and Acceptance

Goal: prove Sema owner/member lookup is structured-primary without regressing
nearby frontend behavior.

Actions:
- Run the focused build or test target needed for touched Sema/frontend code.
- Run targeted CTest selectors for namespace-qualified owner lookup, scoped
  methods, static members, and unqualified lookup.
- Run broader frontend/Sema coverage if multiple lookup families changed or
  compatibility fallback behavior was narrowed across shared helpers.
- Record exact commands and results in `todo.md` and `test_after.log`.

Completion check:
- Focused and required broader validation are green.
- Acceptance criteria from idea 135 are satisfied or any remaining fallback
  work is captured as a separate follow-up idea.

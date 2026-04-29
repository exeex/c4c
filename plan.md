# Cross IR String Authority Audit And Followup Queue Runbook

Status: Active
Source Idea: ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md

## Purpose

Create a cross-IR inventory of remaining `std::string`, raw spelling, and
rendered-name uses that may still act as semantic identity after structured
carriers became available.

## Goal

Classify string and rendered-spelling authority across Parser, Sema, HIR, LIR,
BIR, and backend handoff surfaces, then map every suspicious case to focused
follow-up work under `ideas/open/`.

## Core Rule

Do not treat text, rendered spelling, or `std::string` as cross-IR semantic
authority when a structured carrier exists; generated names, diagnostics,
dumps, labels, mangling, and final emitted text are legitimate only when they do
not decide semantic lookup or producer/consumer identity.

## Read First

- `ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md`
- `ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md`
- `ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md`
- `ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md`
- `ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md`
- `ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md`
- `src/frontend/`
- `src/backend/`

## Current Targets

- Parser record, namespace, visible-name, template, and AST payload boundaries.
- Sema AST ingress, owner lookup, method-owner lookup, static-member lookup,
  and unqualified-name fallback boundaries.
- HIR lowering and lookup paths for records, members, methods, static members,
  templates, and owner-key maps.
- LIR, BIR, and backend handoff paths where string-keyed maps or rendered
  fields may still bridge semantic identity.
- Existing follow-up ideas that already represent suspicious Parser, Sema, and
  HIR authority cleanup.

## Non-Goals

- Do not bulk replace all `std::string` uses.
- Do not remove final emitted text, mangled names, generated labels,
  diagnostic strings, dump strings, or display names.
- Do not implement the cleanup work owned by follow-up ideas inside this audit.
- Do not weaken or rewrite tests to claim audit progress.
- Do not edit unrelated open ideas unless the audit finds a concrete
  string-authority issue that belongs there.

## Working Model

- Structured semantic identity should be carried by tables and ids such as
  `TextTable`, `LinkNameTable`, `StructNameTable`, `MemberSymbolTable`,
  `BlockLabelTable`, parser symbol ids, `QualifiedNameKey`,
  `SemaStructuredNameKey`, `NamespaceQualifier`, `ModuleDeclLookupKey`, and
  owner/member lookup keys.
- Text is acceptable as generated spelling, final artifact spelling,
  compatibility display, diagnostics, dumps, and debug output.
- A string path is suspicious when it is the only authority for lookup,
  substitution, ownership, producer/consumer handoff, or cross-IR identity.
- Existing follow-up ideas 132 through 136 are part of the current queue and
  should be reconciled before creating duplicates.

## Execution Rules

- Keep this plan as an audit and queue-reconciliation runbook, not an
  implementation cleanup plan.
- Search across Parser, Sema, HIR, LIR, BIR, and backend handoff surfaces, but
  classify uses before proposing code changes.
- Prefer existing structured carriers when deciding whether a text path is
  legitimate, compatibility-only, or suspicious.
- If a suspicious case needs implementation, create or update a focused
  `ideas/open/` follow-up instead of expanding this plan.
- When touching idea files, keep edits minimal and durable; routine packet
  findings belong in `todo.md`.
- For any comment-only or idea-only changes, supervisor proof can be
  lifecycle/docs-oriented. If code changes become necessary, stop and request a
  separate implementation plan.

## Ordered Steps

### Step 1: Reconcile Existing Follow-Up Queue

Goal: Determine whether the current open follow-up ideas already cover the
known suspicious Parser, Sema, and HIR string-authority paths from the audit
source.

Primary target: `ideas/open/132_*.md` through `ideas/open/136_*.md`.

Actions:

- Read the open follow-up ideas that name this audit as a parent.
- Map each suspicious path named in idea 131 to an existing follow-up idea or a
  gap.
- Do not change implementation files.
- Record any duplicate, missing, or too-broad follow-up concerns in `todo.md`
  first.

Completion check:

- The executor can state which suspicious Parser, Sema, and HIR paths are
  already covered by existing follow-up ideas and which, if any, need new or
  adjusted idea coverage.

### Step 2: Audit Parser And AST Boundary Text Authority

Goal: Classify remaining parser-side string and rendered-name authority risks
outside the already recorded follow-up queue.

Primary target: parser record/template lookup, namespace/visible-name lookup,
and parser-produced AST/template payload boundaries.

Actions:

- Search parser surfaces for `std::string`, raw spelling, rendered name fields,
  string-keyed maps, and compatibility overloads near semantic lookup.
- Classify each relevant use as structured identity, legitimate display/final
  text, compatibility fallback, or suspicious authority.
- Cross-check suspicious findings against ideas 132, 133, and 134 before
  creating or editing any idea.

Completion check:

- Parser and AST-boundary findings are classified, and uncovered suspicious
  cases have a concrete follow-up target or a clear `todo.md` note explaining
  why no new idea is needed.

### Step 3: Audit Sema And HIR Text Authority

Goal: Classify Sema and HIR paths where rendered owner, member, method, record,
or template names may still decide semantic lookup.

Primary target: Sema owner/static-member lookup and HIR record/member/method/
template lookup authority.

Actions:

- Search Sema and HIR surfaces for string-keyed lookup, rendered name parsing,
  compatibility spelling, and parity-only structured maps.
- Classify each relevant use against existing structured carriers.
- Cross-check suspicious findings against ideas 135 and 136 before creating or
  editing any idea.

Completion check:

- Sema and HIR findings are classified, and uncovered suspicious cases have a
  concrete follow-up target or a clear `todo.md` note explaining why no new
  idea is needed.

### Step 4: Audit LIR, BIR, And Backend Handoff Text Authority

Goal: Check later IR and backend handoff surfaces for remaining string
authority that was not already covered by the Parser, Sema, and HIR follow-up
queue.

Primary target: LIR, BIR, and backend handoff structures, maps, labels, and
final emitted-text paths.

Actions:

- Search LIR, BIR, and backend handoff code for string-keyed semantic maps,
  raw names, rendered fields, and producer/consumer boundary text.
- Separate legitimate final artifact text, labels, mangling, diagnostics, and
  dumps from semantic identity.
- Create or update focused follow-up ideas only for suspicious authority that
  cannot be classified as legitimate or already covered.

Completion check:

- Later-IR and backend string uses are classified enough for the supervisor to
  see whether the cross-IR audit is complete.

### Step 5: Close Audit Evidence

Goal: Leave durable evidence that every suspicious string-authority path is
either classified as acceptable or represented by a focused follow-up idea.

Actions:

- Summarize the final inventory outcome in `todo.md`.
- Ensure every suspicious case found by Steps 1 through 4 maps to an existing
  or newly created `ideas/open/` follow-up.
- Run the supervisor-delegated lifecycle/docs proof. Do not run code
  validation unless code was changed under an explicitly separate plan.
- Hand back whether the source idea appears complete, blocked, or ready for a
  plan-owner close decision.

Completion check:

- `todo.md` contains fresh audit evidence and proof notes.
- The supervisor can decide whether idea 131 can close or whether another
  audit runbook is needed.

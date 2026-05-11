# Whole-Codebase String Authority Final Audit Runbook

Status: Active
Source Idea: ideas/open/167_whole_codebase_string_authority_final_audit.md

## Purpose

Perform a final whole-codebase audit for remaining string-keyed semantic
authority after the parser, sema, HIR, LinkNameId, BIR, and rendered-registry
cleanup series.

## Goal

Produce a reviewable inventory that classifies meaningful string-keyed paths as
semantic authority, compatibility bridge, display/output, diagnostic/debug,
route-local identity, generated temporary name, or false positive, then record
follow-up work for any remaining semantic-authority strings.

## Core Rule

Do not treat every string as suspect. Classify by ownership and domain, and do
not claim progress by weakening tests, deleting output text, or relabeling a
semantic lookup as display without evidence.

## Read First

- Source idea:
  `ideas/open/167_whole_codebase_string_authority_final_audit.md`
- Dependency:
  `ideas/closed/166_hir_rendered_registry_mirror_retirement_audit.md`
- Parent ideas:
  `ideas/closed/158_sema_validate_string_authority_audit.md`
- Parent ideas:
  `ideas/closed/159_sema_consteval_domain_table_authority.md`
- Parent ideas:
  `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- Parent ideas:
  `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- Parent ideas:
  `ideas/closed/162_link_name_id_backend_symbol_authority.md`
- Prior cleanup surfaces in parser, sema, HIR, LinkNameId, BIR, LIR, and
  backend registry/lowering code.

## Current Scope

- Search for string-keyed semantic patterns across the codebase, including
  `unordered_map<std::string, ...>`, `map<std::string, ...>`, `find(name)`,
  `lookup(name)`, `*_by_name`, `*_name_map`, `mangled_name`, `source_name`,
  `qualified_name`, and raw rendered fallback helpers.
- Classify meaningful hits into the source idea's categories.
- Verify retained compatibility bridges are fenced by explicit no-metadata,
  invalid-id, or compatibility-only boundaries.
- Verify complete structured misses fail closed in already-covered domains.
- Record retained bridges, semantic-authority leftovers, route-local domains,
  and concrete follow-up recommendations.

## Non-Goals

- Do not start a large code rewrite during the audit pass.
- Do not remove compatibility bridges before classifying them.
- Do not replace route-local names such as SSA temps, labels, stack slots,
  registers, or string-pool labels unless a follow-up idea explicitly owns that
  work.
- Do not treat output, dump, diagnostic, or display strings as bugs because
  they are strings.
- Do not expand this audit into idea 168, 169, or 170 implementation work.

## Working Model

- Structured source, semantic, and link-visible identity should be carried by
  domain-specific ids or keys.
- Rendered strings may still be valid for display, diagnostics, dumps, final
  output, insertion order, compatibility import, or route-local generated names.
- A compatibility bridge is acceptable only when its owner, boundary, and
  removal condition are clear.
- A complete structured lookup miss in a covered domain must not silently
  recover through rendered spelling.

## Execution Rules

- Start by building an inventory. Do not edit implementation or tests before
  the first classification table and search method are recorded in `todo.md`.
- Search broadly but classify narrowly; preserve evidence for why each retained
  string path is valid.
- Use structured source inspection for ambiguous hits instead of relying on
  grep count alone.
- If a tiny obvious semantic-authority bug is fixed during the audit, record
  the owner, proof, and why it is not route drift in `todo.md`.
- Create or recommend separate follow-up ideas for work that belongs to bridge
  retirement, route-local identity cleanup, or regression guards.
- Escalate to reviewer or supervisor before accepting a slice that mainly
  changes expectations, renames helpers, or downgrades supported behavior.

## Ordered Steps

### Step 1: Establish Audit Method and Initial Inventory

Goal: define the search patterns, code regions, and classification format used
for the whole-codebase audit.

Primary targets:
- parser, sema, HIR, LIR, BIR, and backend source directories
- tests and fixtures that encode string-authority expectations
- helper APIs that use rendered names, `std::string` keys, or name-based
  lookup wrappers

Actions:
- Run broad searches for the patterns named in the source idea.
- Group hits by owner, domain, and apparent purpose.
- Record the audit method, classification legend, known false-positive rules,
  and first candidate families in `todo.md`.
- Identify which regions need AST-backed or local call-chain inspection before
  classification is trusted.

Completion check:
- `todo.md` contains the audit method, classification categories, first
  candidate inventory, and suggested proof command for the first audit packet.
- No implementation or test edits are required for this step.

### Step 2: Classify Covered Structured Domains

Goal: verify that parser, sema, HIR, LinkNameId, BIR, and backend domains
covered by prior ideas no longer use rendered strings as ordinary authority.

Primary targets:
- source and symbol lookup paths migrated by ideas 158 through 162
- HIR rendered-registry surfaces audited by idea 166
- backend symbol and link-name transport paths

Actions:
- Inspect meaningful string lookup hits in each covered domain.
- Classify each as structured authority, compatibility bridge,
  display/output, diagnostic/debug, route-local identity, generated temporary
  name, or false positive.
- For compatibility bridges, record the explicit no-metadata, invalid-id, or
  compatibility-only boundary.
- Note any complete structured miss that can still fall back to rendered
  lookup.

Completion check:
- Covered structured domains have a recorded classification table in
  `todo.md`.
- Any remaining semantic-authority string path has owner, evidence, and a
  follow-up recommendation or tiny-fix rationale.

### Step 3: Classify Route-Local and Generated Names

Goal: separate legitimate route-local identity from source or link-visible
semantic authority.

Primary targets:
- LIR values, stack object names, block labels, and string-pool names
- BIR `Value::name`, local slots, block labels, phi incoming labels, and local
  memory-address base names
- prealloc, MIR-prep, register assignment, and temporary addressing helpers

Actions:
- Identify local strings used only for display, final rendering, or generated
  temporary names.
- Identify local strings used as lookup or validation authority across
  multiple structures.
- Record which route-local families are harmless and which deserve a typed
  local-id follow-up.
- Keep `TextId` and `LinkNameId` out of route-local temporary domains.

Completion check:
- Route-local string families are classified separately from source and
  link-visible identity.
- Follow-up candidates for idea 169 are concrete and evidence-backed.

### Step 4: Classify Compatibility Bridges and Retirement Candidates

Goal: identify retained rendered-string bridges that may be safe to retire or
narrow after the audit.

Primary targets:
- rendered-name overloads and fallback helpers
- `*_legacy`, `*_compatibility`, no-metadata, or invalid-id paths
- tests proving structured misses fail closed

Actions:
- List each retained compatibility bridge with owner, domain, caller class,
  boundary, and removal condition.
- Mark bridges that are production lookup paths versus tests, raw-input import,
  diagnostics, or compatibility-only helpers.
- Identify bridges that should feed idea 168.
- Flag any bridge that remains reachable after a complete structured miss.

Completion check:
- Compatibility bridge inventory is ready for bridge-retirement planning.
- Each retained bridge has owner, limitation, and removal condition.

### Step 5: Record Audit Artifact and Follow-Up Ideas

Goal: preserve the final whole-codebase classification and convert remaining
work into durable next actions.

Actions:
- Consolidate classifications, retained bridges, false positives, route-local
  families, and semantic-authority leftovers in `todo.md` or an appropriate
  durable audit artifact.
- Create or recommend follow-up ideas only for separate initiatives discovered
  by the audit.
- Keep ideas 168, 169, and 170 as the primary known follow-up lanes unless the
  audit proves a distinct initiative is needed.
- Confirm no source idea intent changed during activation or routine audit
  recording.

Completion check:
- The audit inventory is easy for a reviewer to inspect.
- Follow-up recommendations are concrete enough to activate as runbooks.
- No broad refactor or testcase weakening is hidden inside the audit.

### Step 6: Final Proof and Closure Readiness

Goal: prepare the audit idea for supervisor review and lifecycle closure.

Actions:
- Re-run the audit searches or scripts used for classification.
- Run focused build or test proof only for any tiny fixes made during the
  audit; otherwise document why the slice is lifecycle/audit-only.
- Verify known follow-up ideas still cover compatibility bridge retirement,
  route-local identity cleanup, and regression guarding.
- Record final proof commands, changed files, retained risks, and closure
  recommendation in `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or exact blockers are recorded
  for lifecycle decision.
- The supervisor can decide whether to close idea 167 or continue with a
  narrower follow-up runbook.

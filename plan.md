# Compatibility Bridge Retirement Runbook

Status: Active
Source Idea: ideas/open/168_compatibility_bridge_retirement.md

## Purpose

Retire or narrow rendered-string compatibility bridges that the final string
authority audit classified as no longer appropriate ordinary production lookup
paths.

## Goal

Convert safe retained bridges from broad fallback authority into deleted,
test-only, raw-input, no-metadata, display-only, or explicitly named
compatibility APIs while preserving structured lookup behavior and output
spelling.

## Core Rule

Use the closed idea 167 audit inventory as the source of truth. Do not claim
progress by reducing grep count, weakening tests, deleting diagnostics/output,
or moving route-local identity cleanup into this bridge-retirement runbook.

## Read First

- Source idea:
  `ideas/open/168_compatibility_bridge_retirement.md`
- Satisfied dependency and audit context:
  `ideas/closed/167_whole_codebase_string_authority_final_audit.md`
- Prior audit consolidation source:
  `git show 80c86e13d:todo.md`
- Parent cleanup ideas:
  `ideas/closed/158_sema_validate_string_authority_audit.md`
- Parent cleanup ideas:
  `ideas/closed/159_sema_consteval_domain_table_authority.md`
- Parent cleanup ideas:
  `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- Parent cleanup ideas:
  `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- Parent cleanup ideas:
  `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Current Scope

- Parser rendered record, typedef, const-int, and compatibility overloads.
- Sema/type-utils/consteval rendered enum, template, NTTP, and type-binding
  mirrors.
- HIR rendered declaration, record, template, specialization, import, and
  no-owner handoff bridges.
- LIR/HIR-to-LIR final-output scans, extern raw-name maps, legacy struct/type
  mirrors, and rendered reachability scans.
- BIR/backend user, extern, global, initializer, runtime, intrinsic, and raw
  symbol fallback boundaries.
- Tests or assertions proving complete structured misses fail closed where a
  bridge is retired or narrowed.

## Non-Goals

- Do not discover a new whole-codebase string-authority inventory; idea 167
  already owns the audit.
- Do not rewrite route-local SSA, label, slot, prepared value, register, or
  generated-name identity domains; idea 169 owns that work.
- Do not remove diagnostics, debug dumps, printer output, ABI spelling, final
  LLVM/assembly text, or display strings merely because they are strings.
- Do not weaken tests, downgrade supported cases, or remove behavior to make a
  bridge easier to retire.
- Do not build the regression guard or allowlist machinery; idea 170 owns that
  lane after bridge classifications stabilize.

## Working Model

- Structured source, semantic, and link-visible authority should use `TextId`,
  structured keys, `LinkNameId`, `StructNameId`, or concrete declaration ids.
- A retained bridge is acceptable only when its owner, domain, caller class,
  limitation, and removal condition are explicit.
- Complete structured metadata misses in covered domains should fail closed
  instead of recovering through rendered spelling.
- Compatibility paths that must remain should be named as compatibility,
  legacy, no-metadata, invalid-id, raw-import, display, or diagnostic
  boundaries.
- Route-local generated names may remain strings until idea 169 introduces or
  reuses typed local ids for those domains.

## Execution Rules

- Start each step by re-reading the idea 167 classification for the target
  owner and recording the exact bridge family being narrowed in `todo.md`.
- Prefer small behavior-preserving packets: remove one bridge family, fence one
  fallback surface, or add one closed-miss proof at a time.
- When deleting or narrowing an API, inspect all production callers before
  changing tests.
- If a bridge cannot be retired safely, rename or document it as an explicit
  compatibility boundary and record the removal condition.
- Preserve final emitted spelling and diagnostic text unless the source idea
  explicitly justifies a rendering change.
- Escalate to the supervisor or reviewer before accepting any packet whose main
  effect is expectation churn, helper renaming, or testcase-shaped matching.
- For code-changing steps, run a fresh build or focused test proof. Escalate to
  broader parser/HIR/backend regression checks when a packet crosses subsystem
  boundaries.

## Ordered Steps

### Step 1: Reconfirm Bridge Inventory and First Target

Goal: convert the idea 167 compatibility-bridge inventory into an actionable
first bridge-retirement packet.

Primary targets:
- `ideas/closed/167_whole_codebase_string_authority_final_audit.md`
- `git show 80c86e13d:todo.md`
- parser, sema, HIR, LIR, and BIR bridge candidate lists from the prior audit

Actions:
- Re-read the retained compatibility bridge summary and follow-up
  recommendations from the idea 167 audit artifact.
- Choose one narrow first bridge family with clear production callers and
  focused proof.
- Record in `todo.md` the bridge owner, current boundary, planned new
  boundary, nearby tests, and proof command.
- Do not edit implementation before this target packet is explicit.

Completion check:
- `todo.md` names the first target bridge family and proof subset.
- The chosen packet belongs to idea 168, not route-local idea 169 or guard
  idea 170.

### Step 2: Retire Parser Compatibility Overloads

Goal: remove or narrow parser rendered-name overloads once structured metadata
is available.

Primary targets:
- `resolve_record_type_spec`
- `eval_const_int`
- `resolve_typedef_chain`
- `types_compatible_p`
- rendered record, typedef, const-int, and record-tag compatibility maps

Actions:
- Inspect callers and classify each as structured metadata, no-metadata
  compatibility, test, diagnostic, or raw-input import.
- Remove production rendered overloads whose callers can pass `record_def`,
  owner keys, `TextId`, or typed HIR bindings.
- Rename or fence retained no-metadata APIs so callers opt into
  compatibility.
- Add or update focused tests proving complete `TextId` or record metadata
  misses do not recover through rendered maps.

Completion check:
- Parser production structured misses fail closed for the touched family.
- Any retained parser bridge is explicitly named and documented as
  compatibility or no-metadata.
- Focused parser tests or compile proof pass.

### Step 3: Narrow Sema and Consteval Rendered Mirrors

Goal: retire broad sema/template/static-eval rendered mirrors where structured
carriers now provide authority.

Primary targets:
- rendered enum const maps
- template, type, NTTP, and canonical symbol binding mirrors
- fallback canonical template names
- consteval local `by_name` only if it is a source/link compatibility bridge,
  not a route-local cleanup task

Actions:
- Identify complete scoped-carrier, binding-key, and `TextId` paths for each
  candidate mirror.
- Remove or fence rendered mirror lookups that duplicate complete structured
  authority.
- Keep ABI/display strings output-only and avoid changing final formatting.
- Add closed-miss tests for enum scoped-carrier and consteval/template binding
  metadata where a broad bridge is narrowed.

Completion check:
- Touched sema/consteval complete metadata misses fail closed or enter an
  explicitly named compatibility boundary.
- Display and ABI text remain output-only.
- Focused sema or HIR parser-support tests pass.

### Step 4: Retire HIR Rendered Declaration and Template Bridges

Goal: shrink HIR rendered indexes and specialization bridges without losing
valid import, dump, or incomplete-owner compatibility behavior.

Primary targets:
- `fn_index`
- `global_index`
- `struct_defs`
- `template_defs`
- rendered specialization canonical/display keys
- rendered qualified imports and no-owner handoffs

Actions:
- Separate ordinary production lookup from imports, dumps, diagnostics,
  incomplete owner metadata, absent owner-index handoffs, and display output.
- Move production lookup callers to `LinkNameId`, concrete ids,
  `ModuleDeclLookupKey`, `HirRecordOwnerKey`, owner indexes, or structured
  template binding keys.
- Rename or fence retained rendered bridges with explicit compatibility or
  no-owner wording.
- Add tests that a complete HIR declaration, record owner, template binding, or
  specialization miss does not recover through rendered spelling.

Completion check:
- HIR production lookup for the touched family no longer treats rendered text
  as ordinary authority after complete structured metadata misses.
- Retained rendered HIR bridges state their owner and limitation.
- Focused HIR lookup tests pass.

### Step 5: Retire LIR and HIR-to-LIR Final-Output Bridges

Goal: remove production scans that parse final or legacy rendered output back
  into semantic facts when typed carriers are available.

Primary targets:
- `extern_decl_name_map`
- `signature_text`
- `init_text`
- `type_decls`
- legacy signature and type parsing
- rendered discardable reachability scans

Actions:
- Distinguish final LLVM/output text and parity verifier diagnostics from
  production semantic lookup.
- Move production lookup and reachability callers to `LinkNameId`,
  `StructNameId`, typed signature/type mirrors, or initializer function ids.
- Retain printer output and verifier diagnostics as display/output, not
  semantic fallback.
- Add proof for `LinkNameId` or `StructNameId` mismatch failure when a rendered
  fallback is retired.

Completion check:
- The touched LIR/HIR-to-LIR bridge no longer recovers semantic facts from
  final rendered text when typed metadata exists.
- Output spelling is stable unless an intentional rendering change is
  documented.
- Focused LIR/HIR-to-LIR proof passes.

### Step 6: Retire BIR and Backend Raw Symbol Fallbacks

Goal: narrow raw BIR/backend symbol fallbacks to invalid-id, raw-producer,
runtime, intrinsic, or display boundaries.

Primary targets:
- raw function and global symbol maps
- initializer raw symbol parsing
- runtime and intrinsic invalid-id placeholders
- imported BIR name tables
- prepared backend link tables and raw-only handoff points

Actions:
- Separate user/extern/global raw symbol fallback from runtime/intrinsic
  display tokens and invalid-id producer boundaries.
- Move production call, global, initializer, and prepared-link lookup to
  carried link-name ids or explicit imported name tables.
- Rename or document retained raw fallbacks as invalid-id, raw-import,
  runtime/intrinsic, or compatibility boundaries.
- Add validation that present id mismatches fail instead of falling back to raw
  spelling.

Completion check:
- The touched BIR/backend family fails closed on present-id mismatch.
- Retained raw symbol paths are explicit compatibility or producer-boundary
  APIs.
- Focused BIR/backend validation or compile proof passes.

### Step 7: Consolidate Retained Bridges and Final Proof

Goal: make the remaining compatibility surface reviewable and prepare the idea
for closure or a narrowed follow-up.

Actions:
- Re-run searches for rendered, legacy, compatibility, fallback, no-metadata,
  invalid-id, raw symbol, and final-output scan vocabulary in touched domains.
- Record every retained bridge with owner, domain, caller class, limitation,
  and removal condition.
- Verify route-local leftovers are still assigned to idea 169 and guard work
  remains assigned to idea 170.
- Run the supervisor-approved focused regression subset, escalating to broader
  checks if multiple subsystems were changed.
- Record closure readiness, blockers, proof commands, and remaining risks in
  `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or exact blockers are recorded
  for lifecycle decision.
- Retained compatibility bridges are explicit and reviewable.
- Final proof is fresh and matches the changed surface.

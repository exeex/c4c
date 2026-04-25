# HIR Post Dual-Path Legacy Readiness Audit Runbook

Status: Active
Source Idea: ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md

## Purpose

Audit the HIR structured-identity migration after ideas 99 through 102 and
classify remaining legacy rendered-name lookup paths before any cleanup work.

## Goal

Produce `review/103_hir_post_dual_path_legacy_readiness_audit.md` with enough
precision for follow-up ideas 104 and 105 to avoid baseline drift and
testcase-overfit cleanup.

## Core Rule

This is a classification checkpoint. Do not perform implementation cleanup,
expectation rewrites, demotions, or API deletions while executing this plan.

## Read First

- `ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md`
- `ideas/closed/99_hir_module_symbol_structured_lookup_mirror.md`
- `ideas/closed/100_hir_compile_time_template_consteval_dual_lookup.md`
- `ideas/closed/101_hir_enum_const_int_dual_lookup.md`
- `ideas/closed/102_hir_struct_method_member_identity_dual_lookup.md`
- Existing review artifacts for ideas 97 through 99 under `review/`

## Current Targets

- HIR module function/global lookup and `find_*_by_name_legacy` callers
- Compile-time template, template-struct, specialization, and consteval registries
- Enum constant and const-int binding maps
- Struct definition owner lookup and `Module::struct_defs`
- Method, member, and static-member owner lookup helpers
- `TypeSpec::tag` semantic consumers
- HIR dump and diagnostic consumers
- HIR-to-LIR lowering seams under `src/codegen/lir/hir_to_lir/`
- Shared codegen helpers that consume HIR strings for struct tags, rendered
  type strings, raw extern names, and fallback symbol spellings

## Non-Goals

- Do not demote or delete legacy lookup paths.
- Do not change parser, sema, HIR, LIR, or codegen behavior.
- Do not rewrite tests or expectations.
- Do not decide that all rendered strings are semantic identity.
- Do not expand this audit into idea 104 or idea 105 implementation.

## Working Model

Classify each relevant remaining string or legacy path as exactly one primary
category, with notes where a path has mixed responsibilities:

- `safe-to-demote`
- `legacy-proof-only`
- `bridge-required`
- `diagnostic-only`
- `printer-only`
- `abi-link-spelling`
- `blocked-by-hir-to-lir`
- `needs-more-parity-proof`

## Execution Rules

- Prefer AST-backed symbol and caller queries where they reduce manual scanning.
- Record concrete file/function call sites in the review artifact.
- Separate semantic lookup authority from final spelling, diagnostics, dumps,
  ABI/link names, and printer output.
- Treat missing structured metadata as `bridge-required` or
  `needs-more-parity-proof`, not as cleanup permission.
- Flag any unsafe or ambiguous demotion candidate explicitly.
- Keep the report actionable for separate HIR-internal cleanup and downstream
  HIR-to-LIR bridge cleanup.

## Step 1: Inventory HIR Legacy Lookup APIs And Callers

Goal: list the remaining HIR legacy lookup APIs and direct callers after ideas
99 through 102.

Primary targets:

- Module function/global lookup helpers
- `find_function_by_name_legacy`
- `find_global_by_name_legacy`
- Compile-time template and consteval lookup helpers
- Enum, const-int, struct, method, member, and static-member lookup helpers

Actions:

- Inspect declarations, definitions, and direct callers.
- Identify whether each call site already has structured IDs, owner keys,
  `TextId`, or `LinkNameId` available.
- Mark fallback-only paths separately from authoritative string lookup.

Completion check:

- The review artifact has a table of HIR legacy lookup APIs, call sites, and
  provisional classification.

## Step 2: Classify String-Keyed HIR Maps And Fallbacks

Goal: classify remaining HIR string-keyed maps, rendered-name fallbacks, and
parity-only mirrors.

Actions:

- Inspect module, template, consteval, enum, const-int, struct, method, member,
  and static-member storage.
- Identify whether structured lookup is complete enough for direct use.
- Separate maps retained for parity proof from maps that still bridge missing
  downstream metadata.

Completion check:

- The review artifact classifies each string-keyed HIR map or fallback as
  `safe-to-demote`, `legacy-proof-only`, `bridge-required`, or
  `needs-more-parity-proof`, with justification.

## Step 3: Classify Diagnostics, Dumps, ABI, And Printer Spelling

Goal: prevent cleanup work from confusing semantic lookup paths with required
textual output.

Actions:

- Inspect HIR dump and diagnostic consumers.
- Inspect ABI/link-name and final textual spelling surfaces.
- Inspect `TypeSpec::tag` consumers and distinguish semantic owner lookup from
  output or debug text.

Completion check:

- The review artifact lists diagnostic-only, printer-only, and
  ABI/link-spelling surfaces separately from semantic lookup authority.

## Step 4: Inventory HIR-To-LIR String Identity Seams

Goal: identify downstream string handoff points that block HIR-only legacy
lookup deletion.

Primary targets:

- `src/codegen/lir/hir_to_lir/`
- `src/codegen/shared/fn_lowering_ctx.hpp`
- `src/codegen/shared/llvm_helpers.hpp`
- HIR fields consumed by LIR lowering, including `Function::name`,
  `Function::link_name_id`, `Function::name_text_id`, `GlobalVar::name`,
  `GlobalVar::link_name_id`, `GlobalVar::name_text_id`, `DeclRef::name`,
  `DeclRef::link_name_id`, `TypeSpec::tag`, `HirStructDef::tag`, and
  `HirStructField::name`

Actions:

- Trace string identity crossing from HIR into LIR or shared codegen helpers.
- Mark seams that should remain string payload or output text.
- Mark semantic identity handoff that should move to `TextId`, `LinkNameId`,
  typed IDs, or structured owner keys in idea 105.

Completion check:

- The review artifact includes an explicit HIR-to-LIR seam inventory with
  `blocked-by-hir-to-lir` recommendations where appropriate.

## Step 5: Write Follow-Up Recommendations And Proof Gaps

Goal: produce the final audit artifact and make the next cleanup boundary
clear.

Actions:

- Write `review/103_hir_post_dual_path_legacy_readiness_audit.md`.
- Recommend safe idea 104 cleanup scope.
- Recommend downstream idea 105 bridge scope.
- List proof gaps and focused validation subsets needed before demotion.

Completion check:

- The review artifact satisfies all required output from the source idea.
- It names safe demotion candidates separately from bridge-required,
  diagnostic-only, printer-only, ABI/link-spelling, and HIR-to-LIR blocked
  paths.
- No implementation files were changed.

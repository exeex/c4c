# HIR Struct Method Member Identity Dual Lookup Runbook

Status: Active
Source Idea: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Activated: 2026-04-25

## Purpose

Migrate HIR record, method, member, and static-member lookup toward structured owner identity while preserving rendered names for codegen, diagnostics, ABI/link output, HIR dumps, and legacy parity proof.

## Goal

Define and incrementally install durable HIR record-owner keys beside existing rendered-name paths, then prove structured lookup parity without expectation downgrades or testcase-shaped shortcuts.

## Core Rule

Keep rendered names as output spelling and legacy parity witnesses; use structured owner identity only where semantic lookup metadata is available and fallback/mismatch proof can remain in place.

## Read First

- `ideas/open/102_hir_struct_method_member_identity_dual_lookup.md`
- HIR data model declarations for `TypeSpec`, `Module::struct_defs`, `struct_def_order`, and record/method/member maps.
- HIR lowering paths that populate `struct_def_nodes_`, `template_struct_defs_`, `template_struct_specializations_`, `struct_methods_`, `struct_method_link_name_ids_`, `struct_method_ret_types_`, `struct_static_member_decls_`, and `struct_static_member_const_values_`.
- Downstream codegen and dump consumers that still require rendered struct/type/link names.

## Current Targets

- HIR-owned record identity lookup around struct definitions, declaration order, methods, members, static members, base-class lookup, and template specialization ownership.
- Bridge points where `TypeSpec::tag` or rendered owner names still feed diagnostics, dumps, mangling, ABI/link output, or LLVM type naming.

## Non-Goals

- Do not delete or broadly demote `TypeSpec::tag` in this runbook.
- Do not rewrite codegen type naming before HIR semantic lookup has structured parity.
- Do not rewrite parser or sema unless an audit proves a missing metadata handoff blocks HIR identity.
- Do not downgrade expectations, mark supported paths unsupported, or add testcase-shaped special cases.

## Working Model

- Structured record-owner keys should be durable HIR semantic identifiers that can coexist with rendered names.
- Candidate key material includes namespace context ID, global-qualified state, qualifier segment `TextId`s, unqualified struct `TextId`, template argument identity for specialization ownership, and a declaration pointer or declaration ID only as a bridge when durable source identity is incomplete.
- Legacy rendered-name maps remain available during migration for fallback reads, dual-write parity checks, diagnostics, dumps, mangled names, link names, and codegen-facing type output.

## Execution Rules

- Treat `step` as the implementation unit.
- Prefer narrow, behavior-preserving changes with explicit build or test proof after each code-changing step.
- Keep structured and legacy paths side by side until mismatch proof is stable.
- Escalate to broader HIR/backend validation when a step affects shared lookup, templates, method resolution, layout, or codegen handoff.
- Record temporary blockers and proof details in `todo.md`; edit this runbook only when the execution contract or ordering changes.

## Steps

### Step 1: Audit Record-Owner String Identity Uses

Goal: produce a concrete inventory of string-based record identity uses before adding new key types.

Primary target: HIR and downstream codegen handoff sites that read or write `TypeSpec::tag`, `Module::struct_defs`, `struct_def_order`, method maps, member maps, static-member maps, base-class owner lookup, and template specialization maps.

Actions:
- Inspect declarations and all direct consumers of the current rendered owner-name maps.
- Classify each use as semantic lookup, codegen spelling, diagnostic spelling, bridge-required, or legacy parity proof.
- Identify which call sites already have enough metadata to build a structured key and which require a bridge.
- Write audit findings into `todo.md` unless they change durable source intent.

Completion check:
- `todo.md` names the audited surfaces, classification results, and the narrow target for the first structured mirror.
- No implementation behavior changes are required for this step unless the audit discovers a trivial compile-only cleanup.

### Step 2: Define HIR Record-Owner Key Shape

Goal: add a documented structured owner key representation that can coexist with rendered names.

Primary target: HIR identity declarations and helper construction paths.

Actions:
- Define a record-owner key type using available namespace, qualifier, unqualified-name, and template-argument identity.
- Add comparison, hashing, and debug/render helper behavior needed by maps and mismatch messages.
- Add bridge construction helpers for sites that only have declaration identity or incomplete source metadata.
- Keep rendered spelling helpers separate from semantic key construction.

Completion check:
- The new key compiles, is covered by focused unit or integration proof where available, and does not change existing rendered output.

### Step 3: Add Structured Mirrors for Struct Definitions

Goal: dual-write and dual-read struct definition lookup by structured owner key beside rendered-name maps.

Primary target: `Module::struct_defs`, `struct_def_order`, lowerer `struct_def_nodes_`, `template_struct_defs_`, and `template_struct_specializations_`.

Actions:
- Add structured mirrors for struct definition ownership where key metadata is available.
- Preserve legacy rendered-name writes and fallback reads.
- Add mismatch assertions, diagnostics, or traceable proof paths that compare structured and legacy lookup results without changing output.
- Keep declaration order stable.

Completion check:
- Focused HIR proof passes for non-template and template struct definition lookup.
- Rendered dumps, diagnostics, and codegen-facing names remain unchanged.

### Step 4: Add Structured Mirrors for Method Lookup

Goal: route method ownership lookup through structured owner keys while keeping legacy fallback and link-name rendering intact.

Primary target: `struct_methods_`, `struct_method_link_name_ids_`, and `struct_method_ret_types_`.

Actions:
- Dual-write method maps by structured owner key where metadata is available.
- Add dual-read helpers that prefer structured lookup and fall back to rendered owner names with mismatch proof.
- Preserve existing method link-name, return-type, diagnostics, and dump spelling.
- Prove inherited/base and templated method lookup paths when they share the owner identity route.

Completion check:
- Focused method lookup tests pass without expectation rewrites.
- Link names and rendered method output are unchanged.

### Step 5: Add Structured Mirrors for Member and Static-Member Lookup

Goal: migrate member and static-member owner lookup to structured identity with legacy parity.

Primary target: base-class/member lookup paths, `struct_static_member_decls_`, and `struct_static_member_const_values_`.

Actions:
- Dual-write member and static-member lookup tables by structured owner key.
- Route semantic reads through structured helpers where possible.
- Preserve fallback reads and mismatch proof for rendered owner-name paths.
- Prove base-class, member, and static-member cases across namespace and template contexts covered by the audit.

Completion check:
- Focused member/static-member proof passes.
- Static value lookup, diagnostics, HIR dumps, and codegen output remain stable.

### Step 6: Run Parity and Baseline Validation

Goal: prove the structured-owner migration has not changed supported behavior or output spelling.

Primary target: focused HIR tests first, then broader repo-native validation selected by the supervisor.

Actions:
- Run the narrow proof commands established during the implementation steps.
- Run broader validation when multiple lookup families have changed or when a milestone is reached.
- Compare any structured/legacy mismatch reports and resolve real semantic gaps instead of suppressing proof.
- Keep legacy APIs available until a follow-up idea explicitly demotes or deletes them.

Completion check:
- Focused and supervisor-selected broader proof pass.
- `todo.md` records proof commands and results.
- Remaining legacy API demotion candidates are listed as follow-up work, not performed in this runbook unless separately authorized.

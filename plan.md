# HIR To LIR TextId Bridge Inventory And Cleanup Runbook

Status: Active
Source Idea: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Activated from: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Depends on:
- ideas/closed/103_hir_post_dual_path_legacy_readiness_audit.md
- ideas/closed/104_hir_safe_legacy_lookup_demotion.md

## Purpose

Inventory and start cleaning HIR-to-LIR boundary paths where semantic identity
still crosses as rendered strings instead of `TextId`, `LinkNameId`, typed IDs,
or structured references.

## Goal

Make the HIR-to-LIR string seam explicit, classify every relevant string use,
and land the first behavior-preserving identity cleanup that moves a low-risk
handoff toward text-table-backed or typed structured identity.

## Core Rule

Do not remove or weaken rendered string fallbacks until the exact seam has been
classified, the replacement identity source is explicit, and focused LIR/codegen
proof shows behavior parity.

## Read First

- `ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md`
- `ideas/closed/103_hir_post_dual_path_legacy_readiness_audit.md`
- `ideas/closed/104_hir_safe_legacy_lookup_demotion.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/operands.hpp`
- `src/codegen/lir/hir_to_lir/`
- `src/codegen/shared/fn_lowering_ctx.hpp`
- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/llvm/`
- HIR identity fields consumed by lowering:
  - `Function::name`, `Function::link_name_id`, `Function::name_text_id`
  - `GlobalVar::name`, `GlobalVar::link_name_id`, `GlobalVar::name_text_id`
  - `DeclRef::name`, `DeclRef::link_name_id`, structured decl metadata
  - `TypeSpec::tag`
  - `HirStructDef::tag` and record-owner structured keys
  - `HirStructField::name` and field text/member IDs

## Current Scope

- HIR-to-LIR call target, global, extern, and const-init identity handoffs.
- LIR symbol fields where `std::string name` acts as both identity and emitted
  spelling.
- Type identity handoffs where rendered LLVM type strings hide record or type
  identity.
- Field, designator, label, user label, and string-pool seams that must be
  separated into identity, payload, printer, or debug categories.
- Validation subsets for LIR lowering and LLVM/codegen stability.

## Non-Goals

- Do not remove all LIR strings in one pass.
- Do not rewrite final LLVM printer output away from text.
- Do not replace string literal bytes, inline asm text, or other literal
  payload strings.
- Do not change ABI or link-visible spelling behavior.
- Do not continue HIR-internal legacy lookup cleanup from idea 104.
- Do not downgrade tests, expectations, or supported-path contracts to claim
  bridge cleanup progress.

## Working Model

- `semantic-identity` strings should migrate toward `TextId`, `LinkNameId`,
  typed IDs, or structured keys.
- `type-identity` strings should migrate toward typed LIR type refs or HIR
  record-owner keys, with rendering delayed to a printer or backend boundary.
- `abi-link-spelling` keeps `LinkNameId` authoritative while preserving emitted
  text spelling.
- `printer-only` strings are final textual output and are not lookup authority.
- `diagnostic-debug` strings are debug or error text only.
- `literal-bytes` strings are payload, such as string literals or inline asm,
  and are not symbol identity.
- `temporary-rendered-lowering` strings remain rendered today but need a
  structured replacement path before deletion.

## Execution Rules

- Start with inventory. Do not jump directly into deletion or fallback removal.
- For every changed seam, record the classification and replacement authority in
  `todo.md`.
- Prefer low-risk `LinkNameId` and `TextId` handoffs for functions, globals,
  calls, extern declarations, and const-init lowering before type-system
  redesign.
- Preserve rendered fallback while migrating and prove parity before removing
  legacy string authority.
- Keep printer output, diagnostics, ABI/link spelling, literal bytes, and
  temporary rendered lowering separate in code comments or helper boundaries
  when a packet touches adjacent code.
- Escalate to plan review instead of broadening into a full backend type-system
  rewrite.

## Step 1: Build The HIR-To-LIR String Seam Inventory

Goal: create an explicit classification artifact for the relevant boundary
strings before implementation packets begin.

Primary targets:
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/operands.hpp`
- `src/codegen/lir/hir_to_lir/`
- `src/codegen/shared/fn_lowering_ctx.hpp`
- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/llvm/`

Actions:
- Inspect HIR-to-LIR string-producing and string-consuming paths.
- Classify each relevant string seam as `semantic-identity`,
  `type-identity`, `abi-link-spelling`, `printer-only`, `diagnostic-debug`,
  `literal-bytes`, or `temporary-rendered-lowering`.
- For identity-bearing seams, name the intended replacement authority:
  `TextId`, `LinkNameId`, typed ID, typed LIR type ref, or structured record
  owner key.
- Record the first low-risk cleanup candidate and the focused validation subset
  in `todo.md`.

Completion check:
- `todo.md` lists the inspected seam categories, replacement authority for each
  identity-bearing category, the first cleanup candidate, and the proof command
  the supervisor should delegate.

## Step 2: Prefer LinkNameId And TextId For Symbol Identity Handoffs

Goal: make function/global/call/const-init lowering prefer existing structured
identity where it is already available.

Primary targets:
- HIR-to-LIR call target lowering
- HIR-to-LIR global and extern lowering
- HIR-to-LIR constant initializer lowering
- LIR symbol fields where raw `std::string name` is semantic authority

Actions:
- Route identity through existing `LinkNameId` and `TextId` fields before
  falling back to rendered names.
- Add or extend explicit LIR identity fields only where the current string field
  conflates identity with output spelling.
- Keep ABI/link spelling behavior unchanged.
- Preserve rendered fallback and mismatch visibility until proof supports
  removal.

Completion check:
- Focused LIR/codegen proof covers functions, globals, externs, calls, and
  const initializers touched by the packet, with no emitted spelling drift.

## Step 3: Separate Type Identity From Rendered LLVM Type Text

Goal: identify and narrow paths where rendered type strings are used as type
identity rather than final output text.

Primary targets:
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/hir_to_lir/types.cpp`
- `src/codegen/shared/llvm_helpers.hpp`
- HIR record-owner and `TypeSpec::tag` consumers that feed LIR type lowering

Actions:
- Mark rendered LLVM type strings as printer/backend text unless they are still
  acting as lookup keys.
- Introduce or extend typed LIR type references only for a narrow seam with
  clear ownership and focused proof.
- Preserve `TypeSpec::tag` and emitted type spelling where they are still
  required for layout, ABI, printer, or backend compatibility.

Completion check:
- The packet either lands one narrow typed-reference cleanup or records the
  blocker in `todo.md` with nearby candidate paths and proof requirements.

## Step 4: Classify Payload, Printer, Diagnostic, Label, And Field Strings

Goal: prevent non-identity strings from being swept into identity cleanup.

Primary targets:
- field and designator names
- labels and user labels
- string pool names and raw bytes
- literal payload strings and inline asm text
- diagnostic and debug formatting
- final LIR/LLVM printer operands

Actions:
- Separate literal payload and printer-only text from semantic lookup keys.
- Keep string literal bytes and inline asm text as strings.
- Only propose field, designator, or label identity changes where a structured
  ID already exists or the seam has a clear owner.
- Record deferred or blocked cleanup as `temporary-rendered-lowering` when the
  current string is not safe to replace in this runbook.

Completion check:
- The inventory distinguishes identity strings from payload/output/debug text,
  and no packet changes final text-only behavior without a focused reason.

## Step 5: Land One Behavior-Preserving Low-Risk Cleanup

Goal: complete the first cleanup slice selected from the inventory without
expanding beyond the source idea.

Actions:
- Choose one low-risk identity handoff, preferably function/global/call or
  const-init identity where `LinkNameId` or `TextId` already exists.
- Implement the smallest helper or field change that separates identity from
  spelling.
- Preserve rendered fallback during migration unless the proof and classification
  make removal clearly safe.
- Update `todo.md` with changed seams, retained fallbacks, and validation
  results.

Completion check:
- The selected cleanup is behavior-preserving, the related seam classification
  is documented, and focused HIR/LIR/codegen proof is green.

## Step 6: Final Validation And Follow-Up Boundaries

Goal: leave the bridge inventory and first cleanup in a stable state for the
next execution route.

Actions:
- Run the supervisor-selected focused proof for the last packet.
- Escalate to broader regression guard if the cleanup touched multiple lowering
  families, shared helpers, or emitted-code behavior.
- Record remaining `temporary-rendered-lowering` seams and blocked typed-ID
  migrations in `todo.md`.
- Do not close the idea unless the source acceptance criteria are satisfied,
  not merely because this runbook is exhausted.

Completion check:
- HIR-to-LIR string seams are classified, semantic identity has a migration
  path toward text-table-backed or typed references, non-identity strings remain
  separated, and the first cleanup slice has focused proof.

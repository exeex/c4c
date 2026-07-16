# LIR Typed Reference Carriers And Collector Migration Runbook

Status: Active
Source Idea: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Activated after: ideas/closed/850_lir_cfg_phi_raw_bindings_evidence.md
Repaired after: eight accepted one-field migration packets through `LirBinOp.rhs`

## Purpose

Replace raw-text symbol scanning only where an exact typed LIR reference
carrier already exists, preserving compatibility and preventing rendered-text
recovery from becoming semantic authority.

## Goal

Migrate named call/global collectors and LIR-to-BIR preparation one source
field at a time from raw scanner recovery to native semantic reference
carriers.

## Core Rule

Do not perform a broad collector sweep. Each implementation packet must select
one exact source field with an existing semantic carrier, migrate only the
collector/preparation consumers for that field, and prove no missing or
spurious references versus compatibility.

## Read First

- `ideas/open/845_lir_typed_reference_carriers_collector_migration.md`
- `ideas/closed/839_lir_value_reference_carrier_foundation.md`
- `ideas/closed/843_lir_direct_hir_family_construction_array_composition.md`
- `ideas/closed/844_lir_global_extern_initializer_family_facts.md`
- Existing call/global collector and LIR-to-BIR preparation code.
- Compatibility tests around collector output and raw scanner behavior.

## Current Scope

- Call/global collectors and LIR-to-BIR preparation consumers.
- One exact producer-published semantic callee, argument, signature, or global
  reference carrier per packet.
- Compatibility parity for the replaced source field.
- Current repair target: `collect_inst_refs` migration for `LirCmpOp.lhs`
  from raw `S(op.lhs)` scanning to `collect_operand_ref(op.lhs, refs)`.
- Already accepted under this route: `LirStoreOp.val`, `LirStoreOp.ptr`,
  `LirLoadOp.ptr`, `LirGepOp.ptr`, `LirPhiOp.incoming[].value`, and
  `LirCastOp.operand`, `LirBinOp.lhs`, and `LirBinOp.rhs`.

## Non-Goals

- No reconstructing references from rendering.
- No ownership of residual non-type strings.
- No broad scanner deletion before every replaced field has a semantic carrier.
- No 734 receiver or type-model repair.
- No missing/spurious-reference tolerance.

## Working Model

Closed producer/schema routes have published some semantic carriers. This
runbook must first select an exact carrier-backed collector seam, then migrate
only that seam. Residual raw-text references remain queued to their existing
owners.

## Execution Rules

- Treat Step 1 as trace/selection and do not edit code there unless the exact
  first field is already obvious and bounded.
- For implementation steps, update `todo.md` before code edits.
- Preserve scanner compatibility until the exact replacement field is proven.
- Add or update nearby tests that compare missing/spurious reference behavior.
- Run a fresh build plus focused collector/LIR-to-BIR proof; broaden if shared
  collector infrastructure is changed.

## Ordered Steps

### Step 1 - Select One Carrier-Backed Collector Seam

Goal: identify the first exact source field that has a semantic reference
carrier and an existing raw scanner/collector consumer.

Actions:
- Inspect named call/global collector and LIR-to-BIR preparation code.
- Trace candidate source fields to their producer-published semantic carriers.
- Select `LirCmpOp.lhs` only unless inspection disproves its carrier readiness.
- Record the selected field, scanner consumer, proof target, and non-goals in
  `todo.md`.

Completion check:
- `todo.md` confirms `LirCmpOp.lhs` and `collect_inst_refs` as the one-field
  packet, or records a lifecycle blocker/no-ready-field decision with
  evidence.

### Step 2 - Migrate The Selected Field

Goal: replace scanner recovery for the selected field with its semantic
carrier in the named collector/preparation consumer.

Actions:
- Change only the selected consumer path: replace `S(op.lhs)` in the
  `LirCmpOp` arm with `collect_operand_ref(op.lhs, refs)`.
- Preserve compatibility fallback where unselected fields still require it.
- Leave compare, select, aggregate/vector ops, inline asm, and
  residual raw/global text on their current scanner paths.
- Avoid changing producer semantics unless Step 1 proves the source field
  already exists but is wired incorrectly.

Completion check:
- The selected field uses semantic carrier data; unrelated raw scanner paths
  remain unchanged.

### Step 3 - Prove Parity And Fail-Closed Behavior

Goal: prove the migration preserves references exactly and rejects malformed
carrier use.

Actions:
- Add/update nearby positive and malformed tests for the selected field.
- Run fresh build and focused collector/LIR-to-BIR proof.
- Run broader proof if shared collector infrastructure changed.
- Run `git diff --check`.

Completion check:
- Focused proof passes, no missing/spurious reference regression is observed,
  and the scanner is deleted only for the proven selected field.

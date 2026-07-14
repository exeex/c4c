# LIR PHI Producer Helper Result Identity Runbook

Status: Active
Source Idea: ideas/open/775_lir_phi_producer_helper_result_identity.md
Activated from: blocked 751 Step 1 ordinary helper-result first-loss audit.

## Purpose

Supply the native helper-result authority that 751 needs before it can publish
structured PHI incoming values.

## Goal

Make ternary, logical short-circuit, and vaarg helper results retain typed
current-function value identity before any display spelling is rendered.

## Core Rule

Helper result IDs are semantic authority. `%t` names, labels, printed LLVM,
instruction order, and testcase names are display or incidental data, never
recovery inputs.

## Read First

- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `ideas/closed/744_lir_remaining_ordinary_value_identity_publication.md`
- active ternary, logical, vaarg, and `emit_lir_op` producer paths

## Non-Goals

- no `LirPhiOp` carrier/verifier work; 751 owns that later consumer packet
- no Raw-BIR/importer work, CFG predecessor work, unrelated ordinary `LirInst`
  rework, local/object, memory/va, aggregate/vector, target-lowering, MIR, or
  emission expansion
- no parsing of value names, labels, printer text, or rendered LLVM

## Execution Rules

1. Start at the first shared helper-result seam evidenced by the ternary,
   logical, and vaarg chains; do not patch their PHI consumer.
2. Allocate or forward a valid current-function result ID before display text
   is rendered, then pass it structurally to the relevant `emit_lir_op` path.
3. Preserve fail-closed handling for missing, invalid, duplicate, and foreign
   result authority.
4. Prove a fresh build and focused positive/malformed producer coverage before the
   supervisor selects broader or full baseline acceptance.

## Ordered Steps

### Step 1 - Establish typed helper-result authority for PHI producers

Goal: establish the shared result-carrier seam that supplies native value IDs
to the three helper producer families before they reach the raw PHI seam.

Primary targets:

- ternary `emit_rval_id` / `coerce` result path
- logical short-circuit `fresh_tmp` result path
- vaarg helper chain and the corresponding `emit_lir_op` result publication
- nearby focused positive and malformed result-authority coverage

Actions:

- bind focused probes to the first shared helper-result carrier seam
- make each named chain allocate or forward its owning ID before rendering and
  publish it structurally through `emit_lir_op`
- retain display compatibility without making raw result text authoritative
- add nearby positive and malformed coverage across all named producer families

Completion check:

- fresh build and focused ternary/logical/vaarg producer proof pass with
  missing, invalid, duplicate, and foreign result authority still rejected;
  hand the bounded typed helper-result contract back to 751 Step 1.

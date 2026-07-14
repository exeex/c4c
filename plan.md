# Computed-Goto Label-Address Table Initialization Authority Decomposition Runbook

Status: Active
Source Idea: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Resumed from: 770 after accepted native direct-constant contract and ordinary
pointer-store consumer repair; continue at preserved Step 5

## Purpose

Replace the repeatedly moving carrier route with focused frontend-LIR producer
probes for label-address table initialization, representation, and rvalue
consumption seams.

## Goal

Identify the narrowest generic upstream producer/result authority seam before
any implementation packet, without consuming 764's downstream carrier work.

## Core Rule

The `IndirBrStmt` carrier already copies `emit_rval_operand(target).value_id()`
exactly. Focused probes must find authority before that consumer; external
cases remain integration evidence, never implementation selectors.

## Read First

- `ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
- `ideas/closed/770_lir_to_bir_native_label_address_constant_contract.md`
- direct frontend-LIR producers for label-address table initialization,
  representation, and rvalue consumption

## Non-Goals

- no `IndirBrStmt`/`LirIndirectBrOp.addr_value` publication or verifier change
- no Raw-BIR/importer, 734, backend/case ownership absent new evidence, or
  rework of accepted 765/766/767
- no text recovery, testcase-specific logic, synthetic bridge, or automatic
  local-table `DeclRef` decay implementation

## Execution Rules

1. Use the preserved five-case command for baseline/integration evidence only.
2. Use direct frontend-LIR production probes for ownership until evidence moves
   the fault downstream; do not make reduced external-test copies.
3. Record a positive and nearby malformed contract for every separated form.
4. Do not choose an implementation seam until producer maps show a shared
   generic owner or a precise separately scoped blocker.

## Ordered Steps

### Step 1 - Establish the blocked label-address table failure-family baseline

Completed: `b04832c62` accepted the 1/5 versus 4/5 baseline and preserved the
carrier evidence.

### Step 2 - Enumerate separated label-address table producer forms

Completed: `05386c45c` records the producer map.

### Step 3 - Extract direct frontend-LIR producer probes

Completed: the static structured-initializer contract is accepted in 769;
direct automatic-rvalue and table-decay probe work is retained in
`707062aeb` and `85ac8d42c`. The accepted 767 static-table decay control
remains untouched.

### Step 4 - Bind probes and select the narrowest generic producer seam

Completed: `34df925db` selects direct frontend-LIR `LabelAddrExpr` rvalue
production as the shared unresolved producer for automatic scalar
initializer/direct-rvalue consumption and automatic table element
initialization. Its result must retain valid current-function, target-label,
and produced-value identity. Automatic local-table `DeclRef` decay remains a
separate later authority contract, not Step 5 scope.

### Step 5 - Repair and prove native direct LabelAddrExpr rvalue production

Goal: implement only direct frontend-LIR `LabelAddrExpr` rvalue production as
a typed structured result and activate its focused proof.

Actions:

- make only selected producer/result and direct frontend-LIR test changes,
  yielding valid current-function, target-label, and produced-value identity
- use 770's function-owned native direct constant, including the accepted
  ordinary pointer-store consumer, without `select`, `gep`, `bitcast`, or any
  other synthetic materialization used only to manufacture an SSA identity
- add focused frontend-LIR positive coverage and nearby malformed verifier
  coverage for raw, invalid/foreign function or target, non-pointer, and
  missing/invalid/foreign value identity
- run a fresh build and the selected focused proof; retain external cases and
  automatic local-table `DeclRef` decay as later integration/dependency probes
- report accepted capability, proof, and exact return to 764 Step 1

Completion check:

- the selected producer capability and focused proof pass without carrier or
  verifier mutation, text recovery, testcase branching, reopening 767/769, or
  automatic-table decay implementation, and without `select`, `gep`,
  `bitcast`, or another synthetic label-address value bridge.

### Step 6 - Hand off the resolved producer capability to 764

Goal: preserve the accepted upstream authority seam so 764 can resume only its
downstream carrier/integration responsibility.

Completion check:

- lifecycle handoff names the accepted capability and exact return: 764 reruns
  all five consumers and publishes carrier authority only if necessary, then
  returns to 734 for plan-owner disposition.

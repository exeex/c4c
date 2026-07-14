# Computed-Goto Label-Address Table Initialization Authority Decomposition Runbook

Status: Active
Source Idea: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Resumed from: 769 after accepted structured global-initializer contract;
continue at preserved Step 3

## Purpose

Replace the repeatedly moving carrier route with focused frontend-LIR producer
probes for label-address table initialization, representation, and rvalue
consumption seams.

## Goal

Identify the narrowest generic upstream producer/result authority seam before
any implementation packet, without consuming 764's downstream carrier work.

## Core Rule

The `IndirBrStmt` carrier already copies `emit_rval_operand(target).value_id()`
exactly. Focused probes must therefore find authority before that consumer;
external cases remain integration evidence, never implementation selectors.

## Read First

- `ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/closed/769_lir_global_initializer_label_address_authority.md`
- `ideas/closed/767_lir_computed_goto_table_element_pointer_authority_decomposition.md`
- `ideas/closed/766_lir_ssa_indexed_gep_pointer_result_authority.md`
- `ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- direct frontend-LIR producers for label-address table initialization,
  representation, and rvalue consumption

## Non-Goals

- no `IndirBrStmt`/`LirIndirectBrOp.addr_value` publication or verifier change
- no Raw-BIR/importer, 734, backend/case ownership absent new evidence, or
  rework of accepted 765/766/767
- no text recovery, testcase-specific logic, synthetic bridge, or broad
  rvalue/pointer/table/CFG/PHI/local-object/memory/va/aggregate-vector redesign

## Execution Rules

1. Use the preserved five-case command for baseline/integration evidence only.
2. Use direct frontend-LIR production probes for ownership until evidence moves
   the fault downstream; do not make reduced external-test copies.
3. Record a positive and nearby malformed contract for every separated form,
   but do not accept raw behavior as a passing capability.
4. Do not choose an implementation seam until producer maps show a shared
   generic owner or a precise separately scoped blocker.

## Ordered Steps

### Step 1 - Establish the blocked label-address table failure-family baseline

Goal: retain the exact boundary after accepted 765/766/767 and prove why a
new upstream decomposition—not another carrier patch—is required.

Actions:

- run the preserved five-case command after a fresh build
- record the 1/5 versus 4/5 result and classify the four failures only by the
  shared missing target authority
- retain the carrier evidence that `IndirBrStmt` copies `addr.value_id()`
  exactly; do not patch the external cases or carrier

Completion check:

- the baseline records the same family boundary and no conclusion assigns
  producer ownership by testcase name or claims carrier progress.

### Step 2 - Enumerate separated label-address table producer forms

Goal: trace only far enough to distinguish generic label-address table
initialization, table representation, and rvalue-consumption forms before the
accepted table-element GEP/load result.

Actions:

- inventory static and local source forms only where their upstream producer
  contract differs
- name each direct frontend-LIR producer, structured input/result candidate,
  and first missing-authority boundary
- do not select a repair or reopen accepted prerequisite contracts

Completion check:

- a compact map names source-form producer/result contracts without external
  testcase numbers as ownership labels.

### Step 3 - Extract direct frontend-LIR producer probes

Goal: make each separated source form executable as a focused production-path
harness with one precise future positive and malformed contract.

Actions:

- add or extend directly relevant frontend-LIR harnesses, one primary producer
  contract per probe
- begin with the accepted static structured initializer representation from
  769, then cover the remaining automatic-table and direct-rvalue forms
- record the exact structured authority assertion and nearby malformed
  rejection each probe will require; do not mark raw behavior passing
- document why frontend-LIR owns the probe or record evidence that moves it to
  backend/case before creating a backend probe

Current Step 3 completion gap after `707062aeb` and `85ac8d42c`:

- 769's structured initializer positive/malformed verification completes the
  static-storage initializer row, and 767 remains the accepted static-table
  decay control. Do not reopen either contract.
- The `&&label` automatic-local fixture legitimately covers the automatic
  scalar-initializer and direct-rvalue rows because both share the immediate
  direct-rvalue-to-`LirStoreOp` route. Its remaining contract record must name
  a typed pointer producer with valid enclosing-function, target-label, and
  produced-value identity, and malformed raw, invalid/foreign owner or target,
  non-pointer, and missing/invalid/foreign-value rejection.
- The automatic-table decay fixture remains a separate representation row. Its
  remaining contract record must name a current-function local-slot base, two
  typed zero indices, and a valid GEP result identity, with malformed
  raw/non-pointer/invalid/foreign bases, raw or wrongly typed indices, and
  missing/invalid/foreign result rejection.
- These are future producer-contract requirements, not acceptance of the
  current raw/no-ID behavior and not a selection of an implementation seam.

Completion check:

- every form has a focused non-monolithic probe and explicit positive/malformed
  contract, including the two remaining automatic-form records above, without
  expectation downgrade or carrier assertion. Until those records are extracted,
  remain at Step 3 rather than selecting a Step 4 seam.

### Step 4 - Bind probes and select the narrowest generic producer seam

Goal: compare the focused producer/result maps and authorize one implementation
packet or identify a precise separately scoped blocker.

Actions:

- bind each probe to its direct generic producer/result contract
- select a shared or separate seam only when evidence is sufficient
- state the selected proof packet and exact post-acceptance return to 764

Completion check:

- the decomposition authorizes one narrow generic implementation/proof packet,
  or names a precise blocker; it does not publish the carrier or claim external
  integration success.

Selection record (complete):

- The one current implementation seam is direct frontend-LIR `LabelAddrExpr`
  rvalue production. It is the shared unresolved producer for automatic scalar
  initializer/direct-rvalue consumption and automatic table element
  initialization.
- The Step 5 result must be typed and structured, carrying valid current
  function, target label, and produced value identity. This extends neither
  769's accepted static structured initializer contract nor 767's accepted
  static decay contract.
- Step 5 must expose that label-address value through a native, direct
  label-address representation. It must not materialize the value through a
  synthetic identity bridge: `select`, `gep`, `bitcast`, or any comparable
  fabricated instruction/value route is not an acceptable implementation.
- Automatic local-table `DeclRef` decay through the local-slot/two-index
  `LirGepOp` route remains a separately unresolved authority contract. It is
  integration/dependency evidence for the later return to 764, not Step 5
  implementation scope.

Step 5 repair record (synthetic bridge rejected):

- The uncommitted `LirLabelAddrOp` implementation renders its SSA result as
  `select i1 true, blockaddress(...), blockaddress(...)`. Although its focused
  and broader frontend-LIR proofs are green, that is a synthetic identity
  bridge and fails this source idea's explicit no-synthetic-bridge gate.
- This remains an in-scope Step 5 repair: the accepted Step 4 selection is
  still direct frontend-LIR `LabelAddrExpr` rvalue production, and the needed
  correction is its native value representation/lowering rather than a
  carrier, 767, 769, or table-decay change.

### Step 5 - Implement and prove the selected producer seam

Goal: implement only direct frontend-LIR `LabelAddrExpr` rvalue production as
a typed structured result and activate its focused proof.

Actions:

- make only selected producer/result and direct frontend-LIR test changes,
  yielding valid current-function, target-label, and produced-value identity
- represent and lower the produced label address natively and directly; reject
  any `select`, `gep`, `bitcast`, or other synthetic materialization used only
  to manufacture an SSA identity
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

Actions:

- record the accepted producer/result contract, direct proof, and remaining
  integration obligation for lifecycle handoff
- request return to 764 Step 1 only after Step 5 acceptance

Completion check:

- lifecycle handoff names the accepted capability and exact return: 764 reruns
  all five consumers and publishes carrier authority only if necessary, then
  returns to 734 for plan-owner disposition.

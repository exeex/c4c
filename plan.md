# LIR Native Vaarg Operand Carrier Foundation Runbook

Status: Active
Source Idea: ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md
Supersedes: 783 Step 3 while its excluded carrier prerequisite is resolved.

## Purpose

Provide the smallest native current-function carrier foundation needed to make
the three already-inventoried vaarg seams structurally testable, then return
the focused probe work to 783.

## Goal

Accept a narrow operand/result carrier contract, including an explicit decision
on PHI incoming value transport, without entering PHI completion or lowering.

## Core Rule

Track native authority only. A PHI change is permitted solely if it transports
the already-published value carrier; predecessor/edge identity and verification
remain outside this runbook.

## Read First

- `ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md`
- `ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md`
- `ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- frontend-LIR carrier and verifier test inventory selected in Step 1

## Non-Goals

- Do not implement PHI verification, predecessor/edge identity, Raw-BIR/importer,
  backend, target lowering, MIR, emission, broad generic-expression redesign,
  or 782 helper-field publication.
- Do not recover authority from names, labels, text, output, instruction order,
  side tables, or result-name maps.

## Execution Rules

1. Start with carrier inventory and structural-probe feasibility; do not
   presume a PHI incoming representation change is necessary.
2. Keep one carrier contract across GP, FP/alignment, and AMD64 reg/stack;
   isolate any route that cannot share the contract as a blocker, not a special
   case.
3. If a PHI incoming value transport is required, prove it is transport-only
   and record the boundary that still belongs to 751.
4. Run the focused proof after each code-changing packet; preserve matching
   root regression logs through the supervisor-owned guard.

## Ordered Steps

### Step 1 - Inventory carrier surfaces and prove structural-probe feasibility

Goal: identify the smallest current-function operand/result carrier boundary
shared by the three vaarg seams.

Actions:

- trace the authoritative producer, string-only construction point, and
  immediate structural consumer for GP `gr_top` to `reg_addr`, FP/alignment,
  and AMD64 register/stack;
- inspect existing native carrier and frontend-LIR structural-test facilities;
  and
- record whether a probe reaches its consumer without a PHI incoming value
  transport.

Completion check:

- the shared carrier candidate and the PHI incoming-value necessity decision
  are supported by source-level evidence; no code or test changes occur.

### Step 2 - Bind the minimal generic carrier contract

Goal: publish only the native current-function operand/result carrier required
by all three seam boundaries.

Actions:

- implement the smallest carrier addition at the selected producer/consumer
  boundary;
- add a PHI incoming value transport only if Step 1 proved it indispensable,
  retaining string rendering compatibility and excluding verifier and CFG work;
  and
- maintain fail-closed ownership/validity behavior at the carrier's existing
  validation boundary.

Completion check:

- all three seams retain native structural authority at their immediate
  consumer boundaries with no text-recovery fallback or PHI-completion claim.

### Step 3 - Prove the carrier and publish the 783 handoff

Goal: establish focused structural evidence and an unambiguous parent return.

Actions:

- add focused frontend-LIR structural coverage for GP, FP/alignment, and AMD64
  register/stack carrier propagation plus relevant malformed authority cases;
- run the focused build/test command and coordinate the matching regression
  guard with the supervisor; and
- record the accepted carrier contract, PHI incoming value decision, and exact
  return point to 783 Step 3.

Completion check:

- 783 can resume its three focused probe bindings without reopening this
  foundation; any non-transport PHI requirement is named as a separate
  successor.

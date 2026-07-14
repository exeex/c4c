# LIR Typed Expression Result Carrier Decomposition Runbook

Status: Exhausted — close accepted pending supervisor lifecycle review
Source Idea: ideas/open/776_lir_typed_expression_result_carrier_decomposition.md
Activated from: 775's repeated first-loss boundary without failure-family reduction.

## Purpose

Replace the stalled helper-result route with independently observable
frontend-LIR expression-result seams.

## Goal

Identify the smallest typed expression-result carrier contract that can later
support a bounded producer repair for one family at a time.

## Core Rule

Expression-result IDs are semantic authority. `%t` names, labels, printed
LLVM, instruction order, and testcase names are display or incidental data,
never recovery inputs.

## Read First

- `ideas/open/776_lir_typed_expression_result_carrier_decomposition.md`
- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- active `emit_rval_payload`, `emit_rval_id`, `coerce`, ternary, logical, and
  vaarg frontend-LIR producer paths

## Non-Goals

- no generic all-expression migration or implementation claim from this
  decomposition alone
- no `LirPhiOp` carrier/verifier, Raw-BIR/importer, backend-case, side-table,
  CFG, target-lowering, MIR, or emission work
- no parsing of value names, labels, printer text, or rendered LLVM

## Execution Rules

1. Establish the carrier boundary before proposing any production migration.
2. Use focused frontend-LIR probes, not backend case files, to isolate the
   ternary, logical, and vaarg first-loss seams independently.
3. End by publishing only a precise typed PHI-consumer handoff; no PHI repair
   occurs here.

## Ordered Steps

### Step 1 - Establish the typed expression-result carrier boundary

Goal: identify the smallest allocation/ownership boundary across
`emit_rval_payload`, `emit_rval_id`, and `coerce` that can be probed without
committing to a broad migration.

Primary targets:

- typed expression-result carrier API and result allocation ownership
- focused frontend-LIR probe harnesses for later one-family observations

Actions:

- record the exact pre-render allocation/ownership boundary and whether a
  result can traverse it as a typed operand without PHI changes
- add or identify one frontend-LIR probe shape that observes this carrier only
- classify the ternary, logical, and vaarg routes as separate follow-on steps

Completion check:

- a precise carrier-boundary record and one focused frontend-LIR probe contract
  exist; no broad migration, PHI change, or backend proof is claimed.

### Step 2 - Isolate the ternary/coerce result path

Goal: bind one focused frontend-LIR probe to the ternary/coerce first-loss
seam and record its exact typed result requirement.

Completion check:

- the ternary/coerce route has an independent probe and does not rely on
  logical or vaarg facts.

### Step 3 - Isolate the logical short-circuit result path

Goal: bind one focused frontend-LIR probe to the `fresh_tmp` logical result
seam and record its exact typed result requirement.

Completion check:

- the logical route has an independent probe and does not rely on ternary or
  vaarg facts.

### Step 4 - Isolate the vaarg helper result path

Goal: bind one focused frontend-LIR probe to the vaarg helper / `emit_lir_op`
first-loss seam and record its exact typed result requirement.

Completion check:

- the vaarg route has an independent probe and does not rely on ternary or
  logical facts.

### Step 5 - Publish the PHI-carrier consumer handoff

Goal: state exactly which typed result facts are proven for a later 751 PHI
carrier packet and which require separate successors.

Completion check:

- the handoff names each family, typed field, proof, and 751 return point;
  it does not authorize PHI text recovery or Raw-BIR work.

Completed handoff:

- Shared boundary: `fresh_value(FnCtx&)` is the only evidenced pre-render
  allocation that returns an owning current-function `LirValueId` through
  `LirOperand::ssa`; `fresh_tmp` is display text only. `emit_rval_operand` is
  the typed expression-result boundary, while `emit_rval_id`, every string
  `emit_rval_payload` overload, and string `coerce` discard that authority.
- Ternary/coerce (`a1064821a`): the independent
  `test_ternary_coerce_result_authority_loss_boundary` probe proves native
  `LirCastOp` source/destination types, typed `i32` PHI, and later typed `i64`
  Add. The arm-coercion result, PHI result, and later Add input are raw at the
  first loss: `emit_rval_id` and string `coerce`, followed by `fresh_tmp` PHI
  construction. A successor would have to carry typed arm results through
  coercion and the expression consumer; this is not PHI text recovery.
- Logical (`ed1b2dab5`): the independent
  `test_logical_short_circuit_result_authority_loss_boundary` probe proves
  boolean conversion result/branch successor IDs, typed non-`i1` RHS `zext`,
  typed `i32` PHI, and later typed `i32` Add. The RHS conversion result first
  loses authority at `fresh_tmp`; the logical PHI and consumer remain raw.
  A successor would have to carry the RHS conversion result through the
  expression consumer without creating PHI authority from text.
- Vaarg (`0b4cdc019`): the independent
  `test_vaarg_helper_result_authority_loss_boundary` probe proves typed `i32`
  `LirVaArgOp`, its SSA va-list pointer operand, and a later typed `i32` Add.
  The vaarg result is first raw when `emit_rval_payload(VaArgExpr)` allocates a
  `fresh_tmp`, sends it to `emit_lir_op`, returns a string, and
  `emit_rval_expr` wraps it raw. A successor would have to carry the vaarg
  result through that helper/expression boundary.
- Focused proof: Step 1's existing scalar carrier contract and each Step 2–4
  probe passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`; the supervisor also
  accepted the regression guard for the committed decomposition sequence.
- Exact 751 return point: do not reactivate 751 yet. A distinct producer
  successor must first publish owning current-function `LirValueId` authority
  as native `LirOperand` results for the selected family at its first-loss
  boundary, with focused positive and malformed ownership proof. Only after
  that accepted handoff may 751 resume at Step 1 to add its PHI incoming
  value/predecessor carriers and fail-closed verification.
- Disposition: this documentation/decomposition runbook is complete and may
  close as intentionally concluded. It makes no producer capability claim;
  it requires a distinct successor before 751 can resume because the required
  typed expression-result propagation is outside this idea and cannot be
  recovered from raw PHI text. Existing 775 remains the open durable producer
  intent but is not reactivated by this handoff: its resumption record requires
  a new one-family typed expression-result publication successor if no bounded
  helper-only contract is executable.

Forbidden in any use of this handoff: PHI text recovery, PHI carrier or
verifier implementation in this idea, Raw-BIR/importer/backend scope, generic
expression migration, result-name maps, side tables, synthetic values, and
capability claims from these observations alone.

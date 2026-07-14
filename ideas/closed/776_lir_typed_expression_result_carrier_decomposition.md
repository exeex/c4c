# LIR Typed Expression Result Carrier Decomposition

Status: Closed — intentionally concluded documentation decomposition
Type: focused LIR expression-result authority decomposition
Blocked Consumer: `ideas/open/775_lir_phi_producer_helper_result_identity.md`
Downstream Consumer: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`

## Goal

Decompose the raw expression-result boundary into independently evidenced
typed-carrier seams so a later bounded producer repair can publish native
current-function `LirValueId` authority without a broad all-expression API
migration or a PHI text fallback.

## Why This Exists

751 found that PHI incoming values are raw pairs. 775 then established that
the supposed common helper seam is not independently repairable: the first
bad fact moves across `emit_rval_payload`, `emit_rval_id`, and `coerce`, and a
single claimed repair would either migrate generic expression APIs broadly or
change the PHI carrier/verifier outside 775. Neither packet implemented code,
ran after-proof, or reduced the failure family.

The observable seams are frontend LIR production, not backend receipt.
Focused frontend-LIR probes are therefore required; backend case files are not
appropriate evidence for this decomposition.

## In Scope

1. identify the typed expression-result carrier API boundary and its exact
   ownership/allocation contract;
2. isolate one ternary/coerce production probe and its first authority-loss
   seam;
3. isolate one logical short-circuit production probe and its first
   authority-loss seam;
4. isolate one vaarg helper production probe and its first authority-loss
   seam; and
5. publish an explicit PHI-carrier consumer handoff that states which typed
   facts are available, which are not, and what 751 may consume.

Each focused probe belongs in the frontend-LIR test surface and must expose one
producer contract. The original PHI route remains an integration consumer, not
the primary decomposition probe.

## Out Of Scope

- implementing a generic all-expression migration across
  `emit_rval_payload`, `emit_rval_id`, `coerce`, or unrelated expression APIs
- `LirPhiOp` carrier or verifier changes, including text-derived PHI recovery
- result-name maps, side tables, synthetic values, display-text parsing, or
  testcase-shaped identity branches
- Raw-BIR/importer, backend case files, target lowering, MIR, emission, and
  unrelated local/object, memory/va, aggregate/vector, or call-family work

## Acceptance Criteria

- The carrier API boundary is documented with the first allocation/ownership
  seam and a focused frontend-LIR probe contract, without changing code merely
  to classify it.
- Ternary/coerce, logical short-circuit, and vaarg helper probes each identify
  their own first loss and do not borrow facts from the others.
- The PHI handoff names a concrete typed-field contract or explicitly records
  that a separate successor is required; it never authorizes text recovery.
- Any implementation successor is one named seam/family with focused
  frontend-LIR positive and malformed proof. No generic migration or PHI/Raw-
  BIR scope is claimed by this decomposition itself.

## Completed Decomposition Handoff

Disposition: documentation/decomposition complete; close accepted pending
supervisor lifecycle review as intentionally concluded. This source makes no
producer capability claim and does not authorize a PHI repair.

The typed boundary is `fresh_value(FnCtx&)`, which publishes an owning
current-function `LirValueId` through `LirOperand::ssa`. `fresh_tmp` publishes
only display text. `emit_rval_operand` is the typed expression-result boundary;
`emit_rval_id`, string `emit_rval_payload` overloads, and string `coerce` lose
that authority.

| Family | Native typed fields proven | Focused proof | First loss | Required successor direction |
| --- | --- | --- | --- | --- |
| ternary/coerce | cast source/destination types; `i32` PHI type; later `i64` Add type | `test_ternary_coerce_result_authority_loss_boundary` (`a1064821a`) | `emit_rval_id` plus string `coerce`, then `fresh_tmp` PHI result | carry arm/coercion results as typed operands through the expression consumer |
| logical | boolean conversion/branch successor IDs; RHS `zext` types; `i32` PHI and later `i32` Add types | `test_logical_short_circuit_result_authority_loss_boundary` (`ed1b2dab5`) | `fresh_tmp` RHS conversion result, then `fresh_tmp` PHI result | carry the RHS result as a typed operand through the expression consumer |
| vaarg | `LirVaArgOp` `i32` type; SSA va-list pointer kind; later `i32` Add type | `test_vaarg_helper_result_authority_loss_boundary` (`0b4cdc019`) | `fresh_tmp` result passed to `emit_lir_op`, returned through string payload and wrapped raw | carry the vaarg helper result as a typed operand through helper and expression boundaries |

Every probe used the focused frontend-LIR build/test command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure
-R '^frontend_lir_call_type_ref$'`; the supervisor accepted the committed
sequence's regression guard.

Exact 751 return point: keep 751 blocked. A distinct producer-successor must
first select one documented family and publish an owning current-function
`LirValueId` as a native `LirOperand` result at its first-loss seam, including
focused positive and malformed ownership proof. Only after that accepted
handoff may 751 resume at Step 1 to introduce structured PHI incoming values
and predecessors with fail-closed verification.

Existing `ideas/open/775_lir_phi_producer_helper_result_identity.md` remains
the open durable producer intent but is not reactivated by this handoff: its
resumption record requires a new one-family typed expression-result publication
successor when no bounded helper-only contract is executable.

This handoff expressly forbids PHI text recovery; PHI carrier/verifier work in
this source; Raw-BIR/importer/backend work; generic expression migration;
result-name maps, side tables, synthetic values, or display-text parsing; and
any capability claim from these observations alone.

## Closure Record

Archived intentionally concluded: the independently evidenced decomposition
and PHI-consumer handoff are complete, but no producer capability was
implemented. The named remaining-intent route is open
`ideas/open/775_lir_phi_producer_helper_result_identity.md`; it records that a
distinct one-family native typed expression-result publication successor is
required before its bounded producer work can resume. Keep
`ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md` blocked:
its exact return point is after that successor has accepted focused positive
and malformed ownership proof, when 751 may resume at Step 1 for structured
PHI incoming value/predecessor carriers and fail-closed verification.

## Reviewer Reject Signals

- Reject a single broad `emit_rval_*` / `coerce` rewrite claimed as progress
  without separate evidence and probe contracts for the three families.
- Reject backend case files or a monolithic external testcase as the primary
  proof when the observed authority loss is in frontend LIR production.
- Reject parsing `%t` spellings, labels, printer/LLVM text, instruction order,
  or testcase names to create expression or PHI authority.
- Reject PHI carrier/verifier, Raw-BIR, side-table, synthetic-value, or
  result-name-map changes folded into this decomposition.
- Reject classification-only edits, malformed-contract weakening, expectation
  downgrades, or baseline edits presented as capability progress.

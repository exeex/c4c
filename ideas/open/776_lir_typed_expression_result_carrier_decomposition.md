# LIR Typed Expression Result Carrier Decomposition

Status: Open
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

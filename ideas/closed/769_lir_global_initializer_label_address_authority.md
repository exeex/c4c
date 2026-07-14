# LIR Global-Initializer Label-Address Authority

Status: Closed (capability complete; resumed
`ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
at Step 3)
Type: focused global/static initializer representation and verifier contract
Predecessor: 768 Step 3 static-storage initializer probe

## Goal

Give a global/static initializer a structured label-address element that
retains its enclosing-function `LinkNameId` and target `BlockLabelId` (or
equivalent stable target identity), publish it through the constant initializer
and global-lowering route, and validate it with a matching LIR verifier
contract.

## Why This Exists

The 768 Step 3 no-edit probes separated the static-storage source form from
the direct/local `LirOperand` route. `ConstInitEmitter` currently serializes
`blockaddress(...)`, global lowering retains only initializer function IDs, and
`LirGlobal` plus its verifier expose no structured target-label initializer
element. A static table cannot assert or preserve label-target authority until
that distinct global-initializer contract exists.

The accepted 767 static-table-decay control remains valid and untouched. This
idea supplies only the missing static initializer authority needed before 768
can resume its static probe; it does not select or publish the computed-goto
carrier result.

## In Scope

- Define one structured global/static initializer label-address element with
  enclosing-function `LinkNameId` and target `BlockLabelId`/equivalent identity.
- Publish that element from `ConstInitEmitter` through global lowering into
  `LirGlobal` (or its structured initializer model).
- Add matching verifier validation for both enclosing function and target label
  identity, including a nearby malformed rejection.
- Add one direct focused frontend-LIR positive proof and one nearby malformed
  proof for the structured global-initializer contract.
- Determine whether this representation can reach existing downstream use
  without a distinct Raw-BIR/importer contract. If it cannot, record that exact
  boundary as a separately scoped downstream blocker; do not absorb it.

## Out Of Scope

- `LirOperand` direct-rvalue authority, automatic/local initialization, local
  table decay, or automatic-table probes.
- `IndirBrStmt`, `LirIndirectBrOp.addr_value`, carrier publication, backend or
  case work, and all raw-text recovery.
- Any rework of 765, 766, or the accepted 767 static-table-decay control.
- Raw-BIR/importer changes unless a later separately scoped blocker is created
  from evidence; no backend/MIR/case workaround or broad initializer redesign.

## Acceptance Criteria

- A structured initializer element preserves both function and target-label
  identity without relying on rendered `blockaddress(...)` text.
- `ConstInitEmitter` and global lowering publish that structure, and verifier
  checks reject a malformed enclosing-function or target-label reference.
- Direct focused frontend-LIR positive and malformed tests prove the generic
  contract; no reduced external testcase is used as the primary proof.
- The result states whether a Raw-BIR/importer contract is unchanged. If it is
  required, the result names a separate open downstream blocker rather than
  implementing it here.
- 768 can resume exactly at Step 3 with the accepted static representation;
  765/766/767 and carrier behavior remain unchanged.

## Reviewer Reject Signals

- Reject a string, printed `blockaddress(...)`, raw-text parse, or synthetic
  bridge used as label-target authority instead of structured function and label
  identities.
- Reject a test-name-shaped or external-monolith-only proof, expectation
  downgrade, verifier relaxation, helper rename, or classification-only change
  claimed as capability progress.
- Reject any direct-rvalue, local/automatic initialization, table-decay,
  `IndirBrStmt`/`addr_value`, backend/case, or 765/766/767 change in this
  initiative.
- Reject a Raw-BIR/importer change folded into this patch. If the structured
  LIR element cannot cross that boundary, require a named separate downstream
  blocker with the exact missing importer contract.
- Reject a broad global initializer rewrite or an element that retains the old
  function-ID-only / no-target-label failure mode under a new name.

## Closure Record — 2026-07-14

Disposition: capability complete; return to
`ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
Step 3.

- Step 1 selected the structured global-initializer representation boundary.
  Step 2 was accepted in `56d86556a`: global lowering publishes one structured
  initializer element with enclosing-function and target-label identities, and
  verifier validation rejects malformed function/label references. The focused
  `frontend_lir` proof passed 5/5.
- The baseline candidate was rejected and is not rolled forward. A fresh build
  followed by the exact four-case reproduction still fails all four only at
  missing `LirIndirectBrOp.addr_value` authority. That failure family was
  already recorded after accepted 767 (`403e86afd`) in 768 and 764, so it is
  not caused by the structured publication in `56d86556a`.
- No Raw-BIR/importer contract is selected or required by this evidence. The
  remaining work is 768's paused upstream label-address table producer probe;
  it must resume at Step 3 and must not be folded into this closed global
  initializer contract.

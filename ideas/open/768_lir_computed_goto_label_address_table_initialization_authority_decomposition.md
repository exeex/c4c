# LIR Computed-Goto Label-Address Table Initialization Authority Decomposition

Status: Open (parked behind
`ideas/open/769_lir_global_initializer_label_address_authority.md`, active
blocker for `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`)
Type: decomposition of upstream computed-goto label-address table producer seams
Predecessor: 764 Step 1, after accepted 765, 766, and 767 prerequisites

## Goal

Identify and prove the narrowest generic upstream producer contract that gives
computed-goto label-address table targets valid pointer authority before their
rvalue reaches the already-correct `IndirBrStmt` carrier seam.

## Why This Exists

764 Step 1 has moved its first bad fact repeatedly without shrinking the same
four-failure family. 765 supplied member/bitfield RHS identity, 766 supplied
SSA indexed-GEP pointer results, and 767 (`403e86afd`) supplied static-local
and local table-element GEP/load pointer results. A fresh build followed by the
preserved five-consumer command still yields 1/5 passing: `comp-goto-1` passes,
while `20040302-1`, `20041214-1`, `920501-4`, and `920501-5` fail only because
`LirIndirectBrOp.addr_value` is absent.

The direct carrier evidence rules out another carrier repair:
`StmtEmitter::emit_control_flow_stmt(IndirBrStmt)` obtains
`addr = emit_rval_operand(target)` and copies `addr.value_id()` exactly into
`addr_value`. The target arrives without an ID. The remaining work is therefore
an upstream label-address table initialization/representation and rvalue
consumption decomposition, not a monolithic integration or carrier route.

## In Scope

- Re-establish the preserved five-consumer baseline as integration evidence and
  classify only the shared missing-authority stop.
- Enumerate separable generic source/producer forms before the accepted
  table-element GEP/load result, including label-address table initialization,
  table representation, and rvalue consumption.
- Extract directly relevant frontend-LIR production probes, with one primary
  positive/malformed producer contract per source form; use backend/case only
  if evidence later moves ownership downstream.
- Bind each focused probe to its direct generic producer/result contract and
  select an implementation seam only after evidence supports it.

## Out Of Scope

- Changing `IndirBrStmt`, publishing `LirIndirectBrOp.addr_value`, or changing
  carrier verifier requirements; those remain 764 after this decomposition.
- Reopening or reworking accepted 765, 766, or 767 contracts.
- Raw-BIR/importer, 734 Step 7.24, backend/case ownership absent new evidence,
  text recovery, testcase-name branching, expectation downgrade, or a
  synthetic identity bridge.
- Broad rvalue, pointer, table, CFG, PHI, local/object, memory/va,
  aggregate/vector, target-lowering, MIR, or emission-family redesign.

## Acceptance Criteria

- The exact five-consumer command is recorded as 1 pass / 4 failures with the
  four failures classified at the same missing carrier authority, without a
  testcase-specific conclusion.
- Each separated label-address table initialization/representation or rvalue
  consumption form has a direct frontend-LIR probe and one explicit generic
  positive/malformed producer contract; probes are not reduced external cases.
- The mapping selects one narrow generic upstream producer seam or records a
  precise separately scoped blocker. It does not claim carrier publication or
  external integration success.
- The handoff names the selected seam, focused proof, and exact return: resume
  764 Step 1, rerun all five consumers, and publish carrier authority only if
  necessary, then return to 734 for plan-owner disposition.

## Reviewer Reject Signals

- Reject another `IndirBrStmt`/`addr_value` patch when the carrier already
  copies the target operand ID exactly, or any verifier relaxation used to
  manufacture authority.
- Reject test-name-shaped branching, smaller copies of external monoliths, or
  probes that do not bind one primary generic producer/result contract.
- Reject Raw-BIR/importer, 734, backend/case ownership without evidence, or
  rework of accepted 765/766/767 presented as progress.
- Reject text/printed-label recovery, synthetic bridges, expectation
  downgrades, or broad pointer/table/rvalue redesign before direct frontend-LIR
  probes identify the producer seam.

## Resumption Record — 2026-07-14 Static-Storage Initializer Blocker

- Last accepted progress: Step 1 baseline is accepted and committed as
  `b04832c62`; after a fresh build the preserved five consumers remain 1 pass /
  4 failures at missing `LirIndirectBrOp.addr_value` authority. Step 2 producer
  mapping is accepted and committed as `05386c45c`; its AST-backed queries and
  `git diff --check` are recorded in
  `docs/lir_computed_goto_label_address_authority/step2_producer_map.md`.
- Completed steps: Step 1 — Establish the blocked label-address table
  failure-family baseline; Step 2 — Enumerate separated label-address table
  producer forms.
- Interrupted step: Step 3 — Extract direct frontend-LIR producer probes.
- Blocker and scope boundary: the static-storage table initializer is not a
  direct/local `LirOperand` rvalue route. `ConstInitEmitter` serializes
  `blockaddress(...)`; global lowering retains only initializer function IDs;
  `LirGlobal` and its verifier have no structured initializer element that
  carries the enclosing-function `LinkNameId` and target `BlockLabelId` (or
  equivalent identity). Publication and validation of that initializer contract
  are a separately scoped global-initializer initiative, now
  `ideas/open/769_lir_global_initializer_label_address_authority.md`.
- Exact return point: after 769 accepts its representation, producer
  publication, verifier validation, and direct focused frontend-LIR
  positive/malformed proof, resume this idea at Step 3. Extract the static
  initializer positive/malformed frontend-LIR probe using that accepted
  representation, then complete the remaining automatic-table and direct-rvalue
  probes and Step 4 seam selection. Do not reopen 765/766/767 or change
  `IndirBrStmt`/carrier behavior.
- Remaining work: finish Step 3 for all non-static forms after the accepted
  static contract, complete Step 4 seam selection, then only the selected
  upstream producer packet and handoff to 764. The accepted 767 static-table
  decay control remains untouched.

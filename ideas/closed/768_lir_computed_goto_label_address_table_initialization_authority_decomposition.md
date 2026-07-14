# LIR Computed-Goto Label-Address Table Initialization Authority Decomposition

Status: Closed capability complete; archived after handoff to
`ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
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

## Resumption Update — 2026-07-14 Global-Initializer Contract Accepted

- 769 completed its bounded structured global/static initializer contract in
  `56d86556a`: `LirGlobal::initializer_elements` carries the enclosing
  `LinkNameId` and target `LirBlockId`, and the verifier rejects malformed
  owner/target links. Its direct `frontend_lir` proof passed 5/5.
- The subsequent fresh build and exact four-case reproduction failed all four
  cases only because `LirIndirectBrOp.addr_value` is still absent. This is not
  a regression caused by 769: the same family and missing-authority stop were
  already preserved after accepted 767 (`403e86afd`) in this idea and in 764.
  It is an upstream producer investigation for this idea's Step 3, not a
  Raw-BIR/importer boundary and not a reason to reopen 769.
- Exact return point: resume Step 3 — Extract direct frontend-LIR producer
  probes. First use the accepted static structured representation to extract
  its positive/malformed production probe, then complete the automatic-table
  and direct-rvalue forms before Step 4 selects any generic producer seam.
  Do not alter `IndirBrStmt`, `LirIndirectBrOp.addr_value`, Raw-BIR/importer,
  765/766/767, or derive authority from rendered text.

## Resumption Record — 2026-07-14 Native Label-Address Constant Blocker

- Last accepted progress: Steps 1--4 remain complete. Step 1 baseline is
  `b04832c62`; Step 2 producer map is `05386c45c`; the direct automatic
  rvalue and table-decay probe work is retained in `707062aeb` and
  `85ac8d42c`; `34df925db` records the Step 4 seam selection. The most recent
  lifecycle record is `a89f5f4c6`, which rejects—not accepts—the synthetic
  bridge. These references and the accepted 769 (`56d86556a`) / 767
  (`403e86afd`) prerequisites remain evidence; none authorizes reopening
  those boundaries.
- Completed steps: Step 1 — Establish the blocked label-address table
  failure-family baseline; Step 2 — Enumerate separated label-address table
  producer forms; Step 3 — Extract direct frontend-LIR producer probes; Step
  4 — Bind probes and select the narrowest generic producer seam.
- Interrupted step: Step 5 — Repair and prove native direct `LabelAddrExpr`
  rvalue production.
- Blocker and scope boundary: LLVM 19 permits `blockaddress(@f, %target)` as
  a direct pointer constant but rejects `%x = blockaddress(@f, %target)` as
  an instruction. Current LIR operand authority has only value/global/integer
  alternatives; scalar LIR-to-BIR pointer lowering accepts SSA/null/global;
  indirect-jump lowering requires source SSA for `addr_value`; and
  `IndirectJumpTerm` carries only `ValueId`. A direct identity-bearing native
  constant needs Raw-BIR/importer/backend-facing representation/lowering work,
  which is outside 768. The rejected `select` bridge cannot be repaired in
  this source because it violates the explicit no-synthetic-bridge gate.
- Successor: `ideas/open/770_lir_to_bir_native_label_address_constant_contract.md`
  owns only the native LIR-to-BIR label-address constant representation and
  lowering/indirect-jump consumption contract, with direct focused
  positive/malformed proof. It explicitly excludes this source's producer
  recovery, carrier publication, external integrations, table decay, 767/769,
  and broad backend work.
- Exact return point: after 770 accepts a native direct constant contract that
  retains produced pointer identity through the required indirect-jump
  consumption boundary, resume this idea at **Step 5 — Repair and prove native
  direct `LabelAddrExpr` rvalue production** (the interrupted current title).
  Replace the rejected synthetic bridge only in direct frontend-LIR
  `LabelAddrExpr` rvalue production, retain typed
  current-function/target-label/produced-value authority, run its selected
  focused positive/malformed proof, then perform Step 6 handoff to 764. Do not
  redo Steps 1--4, change carrier behavior, implement automatic-table decay,
  or reopen 767/769.

## Resumption Update — 2026-07-14 Native Constant Contract Complete

- 770 is capability-complete and archived at
  `ideas/closed/770_lir_to_bir_native_label_address_constant_contract.md`.
  Its implementation commit is `e8a0f70b4`; proof-state commit `6803f8c25`
  records the accepted direct constant-to-Raw-BIR-source-value-to-
  `IndirectJumpTerm` fixture and its malformed rejection coverage. A fresh
  `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`
  passed.
- Exact return: Steps 1--4 stay complete. Resume at Step 5 — Repair and
  prove native direct `LabelAddrExpr` rvalue production. Replace only the
  rejected synthetic bridge using 770's completed native direct-constant
  contract; retain the focused frontend-LIR positive/malformed proof
  obligation. Do not claim carrier publication, external integration, or
  automatic-table `DeclRef` decay.

## Resumption Record — 2026-07-14 Native Constant Store-Consumer Repair

- Last accepted progress: Steps 1--4 remain complete, with the same commit
  references and accepted prerequisite boundaries recorded above. Step 5 made
  no implementation or test edits after 770's first closure.
- Interrupted step: Step 5 — Repair and prove native direct `LabelAddrExpr`
  rvalue production.
- Blocker and scope boundary: the resumed producer emits its native
  `LirOperandKind::DirectConstant` into the ordinary `LirStoreOp` pointer
  initializer consumer. The first 770 contract proved that constant only in
  its direct Raw-BIR source-value to `IndirectJumpTerm` route. In the ordinary
  store route, `LirStoreOp` printer operand admission excludes
  `DirectConstant`, and function value-use verification accepts only an
  instruction definition for the constant's `LirValueId`. This is not a 768
  producer repair: legal direct-constant consumption and the corresponding
  validation belong to the native-constant contract.
- Successor/repair route: 770 is reopened under
  `ideas/open/770_lir_to_bir_native_label_address_constant_contract.md` for
  one bounded repair: permit the structured native direct label-address
  constant in the ordinary pointer-store value operand, render it as a legal
  direct `blockaddress(...)` constant, and validate its function-owned direct
  definition rather than requiring an instruction definition. It must prove
  this store-consumer route and malformed direct-constant authority/use forms;
  it must not change producer recovery, carrier publication, table decay, or
  broad lowering.
- Exact return point: after the reopened 770 repair accepts native direct
  constant use in the pointer-store consumer, resume this idea at **Step 5 —
  Repair and prove native direct `LabelAddrExpr` rvalue production**. Retry
  that producer/test packet without redoing Steps 1--4. Preserve the typed
  current-function, target-label, and produced-value authority; then run the
  selected frontend-LIR positive/malformed proof. Do not alter carrier
  behavior, automatic-table `DeclRef` decay, or accepted 767/769 contracts.

## Resumption Update — 2026-07-14 Store-Consumer Repair Accepted

- 770's reopened in-scope repair is complete: `0a2778f0d` makes the structured
  native direct constant legal as the ordinary pointer-store value operand,
  with focused positive/malformed coverage. Together with `e8a0f70b4` and
  `6803f8c25`, it is accepted after a fresh build and the passing focused
  `^backend_lir_to_bir_interface$` proof retained in the matching root logs.
- Exact active return: Steps 1--4 stay accepted. Resume **Step 5 — Repair and
  prove native direct `LabelAddrExpr` rvalue production**. Use the completed
  native constant without a synthetic bridge, then run the selected focused
  frontend-LIR positive/malformed proof. Do not claim producer recovery,
  carrier publication, external integration, automatic-table decay, or any
  change to 767/769 before that packet earns its own acceptance.

## Resumption Record — 2026-07-14 Automatic Local Table Initializer Blocker

- Last accepted progress: Steps 1--4 remain accepted: `b04832c62` records the
  baseline, `05386c45c` maps producers, `707062aeb` and `85ac8d42c` retain the
  direct automatic-rvalue/table-decay probes, and `34df925db` records the
  Step 4 seam. 770's native direct-constant contract and ordinary pointer-store
  repair are closed and accepted at `023a6660b` (implementation `e8a0f70b4`
  and `0a2778f0d`; proof state `6803f8c25`). Accepted 767 (`403e86afd`) and
  769 (`56d86556a`) remain closed prerequisites.
- Interrupted step: **Step 5 — Repair and prove native direct `LabelAddrExpr`
  rvalue production.** Its local code and focused probe are deliberately
  uncommitted and are not accepted.
- First bad fact: that direct-rvalue packet passes a fresh build plus
  `^frontend_lir_label_address_rvalue_probe$`, but the mandatory fresh broader
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_'` guard
  regresses `frontend_lir_call_type_ref` (6/7) with
  `LirStoreOp.val: must not be empty`. Reverting the local-declaration
  `stmt.cpp` authority change leaves the same failure. Nested automatic
  `void *table[] = { &&first, &&second };` initializer-list/coordinator
  lowering loses operand identity after direct `LabelAddrExpr` creates its
  function-owned native direct constant.
- Blocker and scope boundary: preserving that structured native direct-constant
  authority through automatic local table initializer emission is a separate
  initializer route, not Step 5 direct-rvalue producer repair and not
  carrier/verifier/backend/Raw-BIR work. It also does not reopen 767 or 769.
  `ideas/open/771_lir_automatic_local_label_address_table_initializer_authority.md`
  owns only this route, its focused positive/malformed proof, and any generic
  direct consumer indispensably shared by that route.
- Exact return point: after 771 accepts automatic local initializer emission
  that retains the structured direct-constant operand identity, resume this
  idea at **Step 5 — Repair and prove native direct `LabelAddrExpr` rvalue
  production**. Re-run the direct-rvalue producer packet and focused
  positive/malformed proof without redoing Steps 1--4, then assess Step 5 and
  continue to Step 6's 764 handoff.
- Remaining action and guardrails: do not accept or commit the current Step 5
  code; do not implement automatic-table `DeclRef` decay-to-GEP, carrier
  publication, external integrations, 764 work, or broad rvalue/table redesign
  here. Reject synthetic bridges, raw-text recovery, testcase routing, and
  expectation downgrades.

## Resumption Update — 2026-07-14 Automatic Local Table Initializer Accepted

- 771 is capability-complete and archived at
  `ideas/closed/771_lir_automatic_local_label_address_table_initializer_authority.md`.
  Its accepted generic initializer-consumer implementation is `9cb82f9cb`.
  A fresh build, focused `^frontend_lir_label_address_rvalue_probe$`, and
  `^frontend_lir_` guard passed 7/7; the focused stacked-worktree proof
  verifies both automatic local table element stores retain the structured
  direct-constant identity. Regression comparison against the matching earlier
  frontend-LIR baseline is non-regressive 7/7 with
  `--allow-non-decreasing-passed`; direct before/after attribution is
  unavailable because the unaccepted Step 5 stack changed between captures.
- Exact active return: Steps 1--4 stay accepted. Resume **Step 5 — Repair and
  prove native direct `LabelAddrExpr` rvalue production**. The current
  uncommitted Step 5 producer files and focused-probe hunk remain unaccepted;
  rerun and assess that packet on its own merits. Do not redo Steps 1--4 or
  reopen 771, 767, 769, carrier behavior, or automatic-table `DeclRef` decay.

## Completion Record — 2026-07-14 Native Producer Accepted; External Handoff

- Accepted progress: Steps 1--4 retain their accepted records and commit
  references above. Step 5 is now accepted in `628b55b9`: native direct
  `LabelAddrExpr` rvalue production emits the typed current-function,
  target-label, pointer, and produced-value constant without a synthetic
  bridge. The accepted prerequisite initializer-consumer repair is
  `9cb82f9cb`. A fresh build, focused
  `^frontend_lir_label_address_rvalue_probe$`, and `^frontend_lir_` guard
  passed 7/7.
- Step 6 is complete: 764 is resumed at **Step 1 — Publish and prove
  production computed-goto address carrier authority**. The exact return is to
  rerun its five preserved computed-goto consumers and repair/publish
  `LirIndirectBrOp.addr_value` only from verified pointer authority if the
  four shared external failures still stop there.
- The hook full-suite candidate is rejected, not producer rejection evidence:
  it regressed from 0/3034 to 5/3037 failures. Four cases (`20040302-1`,
  `20041214-1`, `920501-4`, and `920501-5`) fail at missing
  `LirIndirectBrOp.addr_value`, which is outside this source's explicit
  non-goals and belongs to 764. `pr70460` fails instead at
  `LirGepOp.ptr: must not be empty`; it is outside both the accepted producer
  capability and 764's carrier-family scope and requires a separately scoped
  open GEP-pointer-authority blocker before a future full-suite candidate.
- No return to 768 is planned. Do not reopen its accepted Steps 1--5 for
  carrier publication, external integration, automatic-table `DeclRef` decay,
  or the GEP failure. If future evidence identifies a new unproven native
  direct-label producer contract, create a separate scoped initiative rather
  than weakening this completion boundary.

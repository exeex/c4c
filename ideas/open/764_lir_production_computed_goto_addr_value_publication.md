# Production LIR Computed-Goto Address Value Publication

Status: Open (active plan; blocker for
`ideas/open/734_lir_to_new_bir_container_completeness.md`)
Type: bounded production LIR computed-goto authority repair
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`, after accepted Step 7.24

## Goal

Make the active production computed-goto lowering publish a verifier-valid,
current-function pointer `LirValueId` in `LirIndirectBrOp.addr_value`, so the
already accepted Raw-BIR receiver can import its typed address without a text
fallback.

## Why This Exists

The bounded receiver in 734 Step 7.24 is accepted, but the fresh full suite
reproduces `llvm_gcc_c_torture_src_comp_goto_1_c` failing before Raw-BIR import:
`LirIndirectBrOp.addr_value: must carry current-function pointer LirValueId`.
The failure is production LIR from `tests/c/external/gcc_torture/src/comp-goto-1.c`,
not the receiver.

Closed idea 757 introduced the field, verifier, and a focused HIR lowering
handoff. Its `StmtEmitter::emit_control_flow_stmt(IndirBrStmt)` publication
copies `emit_rval_operand(...).value_id()`. The production route reaches that
owner with no ID, so the 757 fixture contract did not prove this distinct
producer route. This blocker starts at that first actual authority-loss owner;
it does not reopen 757 or the 734 receiver.

## In Scope

- reproduce and trace the production `comp-goto-1.c` lowering only far enough
  to identify why the `IndirBrStmt` target operand has no current-function
  `LirValueId`
- make that active producer path publish the existing pointer value identity
  through `LirIndirectBrOp.addr_value` without parsing operand, label, printer,
  LLVM, or testcase text
- retain and exercise existing verifier requirements for presence, validity,
  current-function ownership, pointer suitability, and display-mirror
  agreement
- add focused production-path coverage and a precise handoff proving valid
  `addr_value` reaches the computed-goto carrier for `comp-goto-1.c`

## Out Of Scope

- Raw-BIR containers, importer, receiver validation, or re-execution of 734
  Step 7.24
- recovering authority from display strings, labels, rendered output, or
  testcase identity
- successor/CFG publication already completed by 750, or a broad rewrite of
  HIR, parser, sema, rvalue, pointer, local/object, memory/va, PHI,
  aggregate/vector, parameter, call, target-lowering, MIR, or emission families
- unrelated failure families outside the same `LirIndirectBrOp.addr_value`
  downstream consumer family

## Acceptance Criteria

- The five currently affected consumers (`comp-goto-1`, `20040302-1`,
  `20041214-1`, `920501-4`, and `920501-5`) no longer fail because
  `LirIndirectBrOp.addr_value` is absent.
- The production carrier has a valid current-function pointer `LirValueId` and
  the existing verifier rejects missing, invalid, foreign, non-pointer, or
  display-mismatched authority without text fallback.
- Focused coverage proves the actual production computed-goto route, not only
  a hand-built LIR fixture, publishes the authority and preserves fail-closed
  malformed cases; rerunning the five affected consumers shows no remaining
  carrier failure before a future full baseline candidate is considered.
- The accepted handoff names the producer seam, typed field, proof, and exact
  return action: resume 734 after Step 7.24 for plan-owner disposition; do not
  re-execute the receiver packet.

## Reviewer Reject Signals

- Reject parsing `addr`, labels, rendered LLVM/printer output, or testcase text
  to derive or repair the value ID.
- Reject a named-case-only workaround, expectation downgrade, verifier
  relaxation, or a change that leaves the production carrier without the same
  typed current-function pointer authority.
- Reject Raw-BIR/importer changes, receiver rework, or changes that claim
  ownership of the other four full-suite failures.
- Reject broad rvalue, CFG, PHI, local/object, memory/va, aggregate/vector, or
  target-lowering rewrites instead of a minimal repair at the first evidenced
  production authority-loss owner.

## Resumption Record: member/bitfield rvalue identity blocker

Paused at `Step 1 - Trace and publish production computed-goto address
authority`; no implementation from this idea was accepted. A fresh clean build
preceded the investigation, and the focused baseline is preserved in
`test_before.log`:

`ctest --test-dir build -V -R '^llvm_gcc_c_torture_src_comp_goto_1_c$'`

The exact first bad fact is that the test fails before Raw-BIR import because
`LirIndirectBrOp.addr_value` is absent. The trace established that
`IndirBrStmt` correctly copies `emit_rval_operand(...).value_id()`. In
`comp-goto-1.c`, the target is `base_addr + insn.f1.offset`; binary lowering
routes that expression through `emit_indexed_gep`, whose result remains
string-only because the RHS member/bitfield rvalue has no ID. The exploratory
patch was fully reverted. Its build passed, while focused
`frontend_lir_call_type_ref` failed at the intentional raw-index guard:
`verify_authoritative_gep` requires fully authoritative inputs. Thus a
GEP-only ID publication, text recovery, or verifier weakening is rejected.

## Resumption Record: accepted 765 producer handoff

765 Step 1 is concluded and archived at
`ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`.
Accepted commit `1e24e2081` repaired the confirmed first production producer:
`emit_member_rval_operand` now returns the final `emit_bitfield_load` valid
current-function `LirValueId` for `insn.f1.offset`. Its fresh build,
`frontend_lir_call_type_ref` proof, matching subset guard, and broad backend
guard are accepted.

The full baseline candidate was rejected, not accepted: against baseline
`c8a205218` (3034/3034), five tests fail with the same downstream
`LirIndirectBrOp.addr_value: must carry current-function pointer LirValueId`
check: `comp-goto-1`, `20040302-1`, `20041214-1`, `920501-4`, and `920501-5`.
They are one consumer failure family for this route, not separately accepted
out-of-scope debt. Resume this idea at Step 1: publish the authoritative
address GEP pointer identity into the computed-goto carrier, preserve all
fail-closed checks, rerun all five consumers, and require a later full
candidate to show no new baseline failures. Do not resume 734's receiver; its
accepted Step 7.24 remains paused as recorded.

## Resumption Record: SSA-based indexed-GEP pointer-result authority blocker

Paused at `Step 1 - Publish and prove production computed-goto address carrier
authority`; no implementation from 764 was accepted. The accepted predecessor
progress remains 765 Step 1 in `1e24e2081`: `emit_member_rval_operand` now
supplies the valid current-function RHS `LirValueId` for
`insn.f1.offset`, and its fresh build, focused
`frontend_lir_call_type_ref` proof, matching subset guard, and broad backend
guard remain the accepted producer proof references.

The exact five-consumer reproduction command was:

`ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`

All five stop at the same missing `LirIndirectBrOp.addr_value` authority
check. The first newly established blocking fact is upstream of the carrier:
`src/codegen/lir/hir_to_lir/lvalue.cpp::emit_indexed_gep` emits a raw-result
`LirGepOp` and returns string-only. Although the statement seam correctly
copies `addr.value_id()`, the production address GEP uses an SSA base and the
current authoritative-GEP contract only permits a global `LinkNameId` base.
A stmt-only pointer-bitcast bridge was rejected: a fresh
`cmake --build --preset default` passed, but
`ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
failed at `verify_cast_op_authority` because authoritative casts require
integer endpoints. No production or test edits remain from that rejected
route, and no `test_after.log` was written.

This contract/producer repair is outside 764's bounded downstream
carrier-publication scope. The separately active blocker
`ideas/open/766_lir_ssa_indexed_gep_pointer_result_authority.md` owns only
current-function pointer-result identities for SSA-based indexed GEPs and its
direct nearby coverage. It must not change Raw-BIR/importer, revive 734 Step
7.24, weaken the verifier, use display/text recovery, or absorb this idea's
`LirIndirectBrOp.addr_value` publication. After that blocker is accepted,
resume 764 at Step 1: publish the verified GEP pointer `LirValueId` into
`LirIndirectBrOp.addr_value`, rerun all five consumers above, then return to
734 for plan-owner disposition. Do not substitute a synthetic alloca/load,
phi, select, or text-derived ID at the statement seam.

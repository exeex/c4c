# Current Packet

Status: Active
Source Idea Path: ideas/open/765_lir_member_bitfield_rvalue_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish the production member/bitfield RHS value identity

## Just Finished

- Plan Step 1: confirmed `StmtEmitter::emit_rval_expr` routed `MemberExpr`
  through the string-only `emit_rval_payload` path, whose final
  `emit_bitfield_load` temporary was the first production owner dropping the
  RHS identity for `insn.f1.offset`. `emit_member_rval_operand` now returns
  the bitfield load's valid current-function `LirValueId`; its load,
  shift/mask/sign-extension, and final value use structured operands so the
  ID remains verifier-valid. `frontend_lir_call_type_ref` proves that exact
  final ID reaches the member rvalue consumer.

## Suggested Next

- Return to 764 Step 1 and reattempt only computed-goto address publication
  using the repaired `insn.f1.offset` producer; do not resume 734 directly.

## Watchouts

- Do not change Raw-BIR/importer, 734, `IndirBrStmt`, or the computed-goto
  address carrier. Do not publish a GEP result from a raw/partial RHS index.
- Keep `verify_authoritative_gep` fail-closed and never derive identity from
  rendered operands, labels, LLVM/printer text, or testcase identity.
- The requested external case still stops at the unchanged downstream
  `LirIndirectBrOp.addr_value` authority failure. This packet intentionally
  does not make the local-base GEP authoritative or relax its verifier.

## Proof

- Preserved baseline: `ctest --test-dir build -V -R '^llvm_gcc_c_torture_src_comp_goto_1_c$'`
  (recorded in `test_before.log`).
- Fresh `cmake --build --preset default` succeeded. The delegated
  `ctest --test-dir build -j --output-on-failure -R
  '^(backend_lir_to_bir_interface|llvm_gcc_c_torture_src_comp_goto_1_c)$'`
  retained the expected downstream comp-goto failure and passed
  `backend_lir_to_bir_interface`, including its malformed/raw-index
  fail-closed coverage. Additional producer proof (not selected by that
  regex): `ctest --test-dir build --output-on-failure -R
  '^frontend_lir_call_type_ref$'` passed. No canonical regression log was
  written; the supervisor owns those logs.

# Current Packet

Status: Active
Source Idea Path: ideas/open/765_lir_member_bitfield_rvalue_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish the production member/bitfield RHS value identity

## Just Finished

- None; this upstream blocker was activated after 764 proved that the
  computed-goto carrier and GEP verifier are behaving correctly and the RHS
  member/bitfield rvalue is the first missing-authority seam.

## Suggested Next

- Trace `insn.f1.offset` in `comp-goto-1.c` to the first production
  member/bitfield rvalue owner that drops its `LirValueId`, then repair only
  that owner.

## Watchouts

- Do not change Raw-BIR/importer, 734, `IndirBrStmt`, or the computed-goto
  address carrier. Do not publish a GEP result from a raw/partial RHS index.
- Keep `verify_authoritative_gep` fail-closed and never derive identity from
  rendered operands, labels, LLVM/printer text, or testcase identity.

## Proof

- Preserved baseline: `ctest --test-dir build -V -R '^llvm_gcc_c_torture_src_comp_goto_1_c$'`
  (recorded in `test_before.log`).
- Before handoff: fresh `cmake --build --preset default` plus focused producer
  and malformed-index proof selected during Step 1. The supervisor owns
  broader acceptance and canonical regression logs.

Status: Active
Source Idea Path: ideas/open/855_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish And Verify The Selected Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the selected
`LirBinOp.scalar_lhs_parameter_authority` row: the existing carrier/emitter
already published the native current-function DirectScalar LHS tuple for
floating `fmul`, and the verifier now fails closed unless that floating LHS
authority is consumed by the selected `fmul` relation with a nonselected scalar
RHS. Added focused frontend coverage for the positive `double x * 2.0` row and
malformed authority/consumer cases covering omitted, invalid, duplicate,
foreign, owner/index/type/ABI/role, non-`fmul`, LHS mismatch, type mismatch,
and RHS consumer incoherence.

## Suggested Next

Supervisor should choose the next packet. A coherent next slice is the
downstream handoff for only this selected binary-`fmul` LHS row if the runbook
intends receiver work next; otherwise keep later rows separate.

## Watchouts

`ir.hpp` and `hir_to_lir/expr/binary.cpp` were intentionally left unchanged:
the existing scalar-LHS authority carrier and emitter already support the
selected row. Raw-BIR receiver files, backend tests, `plan.md`, source ideas,
and `test_before.log` remain untouched. Later or nonselected candidates remain
out of scope for this packet.

## Proof

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$' ) > test_after.log 2>&1 && git diff --check`

Passed. Proof log: `test_after.log`.

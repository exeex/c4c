Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.44
Current Step Title: Receive DirectScalar Binary-Fmul LHS Parameter Authority
你該做code review了

# Current Packet

## Just Finished

Completed `plan.md` Step 7.44 by receiving the closed-855
`LirBinOp.scalar_lhs_parameter_authority` tuple into typed Raw BIR for a
current-function `DirectScalar` floating parameter used as binary `fmul` LHS.
The receiver preserves LirValueId 91, owner
`direct_scalar_fmul_lhs_parameter_owner`, parameter index 0, `float` type,
`DirectScalar` ABI, explicit `Lhs` role, opcode `fmul`, LHS identity, matching
Raw-BIR `F32` result type, and coherent nonselected scalar RHS value 92.

## Suggested Next

Select the next bounded 734 receiver row after Step 7.44; do not extend this
packet into adjacent DirectScalar parameter families.

## Watchouts

Step 7.44 deliberately admits only the closed-855 binary-`fmul` LHS parameter
authority row. Missing, invalid, duplicate, foreign, owner/index/type/ABI/role,
non-`fmul`, LHS mismatch, result type mismatch, RHS incoherence, and
duplicate-consumer cases reject transactionally before publication.

## Proof

Passed:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_after.log 2>&1 && git diff --check`

Proof log: `test_after.log`.

Status: Active
Source Idea Path: ideas/open/635_prepared_branch_stack_clobber_safety_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Reclassify Residual Branch Stack-Source Rows

# Current Packet

## Just Finished

Step 6 re-ran the seven representative branch stack-source rows after the Step
5 clobber-safety publication and classified each remaining failure by current
first owner:

| Row | Current first owner | Recommendation |
| --- | --- | --- |
| `src/20001017-1.c` | `unsupported_call_abi` | Split/leave to ABI object-route work. |
| `src/loop-2e.c` | `unsupported_terminator_fragment` | Split/leave to RV64 terminator lowering. |
| `src/pr39100.c` | `unsupported_terminator_fragment` | Split/leave to RV64 terminator lowering. |
| `src/20000314-3.c` | `unsupported_terminator_fragment` | Split/leave to RV64 terminator lowering. |
| `src/20140828-1.c` | `unsupported_terminator_fragment` | Split/leave to RV64 terminator lowering. |
| `src/20080519-1.c` | `unsupported_terminator_fragment` | Split/leave to RV64 terminator lowering. |
| `src/20050125-1.c` | `unsupported_terminator_fragment` | Split/leave to RV64 terminator lowering. |

No representative row now reports `missing_stack_clobber_safety`; the prepared
branch clobber-safety authority is no longer the first owner for this family.

## Suggested Next

Supervisor should treat idea `635` as close-ready from the executor's
classification perspective. Residual failures should be split or left to their
own owner routes: one ABI object-route row and six RV64 terminator-lowering
rows. No further clobber-safety packet is justified by this seven-row probe.

## Watchouts

This was classification-only; no code, expectation, unsupported-marker,
allowlist, or source-idea changes were made. The probe still exits nonzero
because all seven representative rows remain unsupported, but their current
owners are ABI or terminator lowering rather than branch stack clobber-safety
authority.

## Proof

`cmake --build --preset default && ALLOWLIST=build/agent_state/635_step1_branch_clobber_safety.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: build passed, probe exited nonzero with `total=7 passed=0 failed=7`.
For this Step 6 classification packet, the nonzero probe result is expected
because the rows remain unsupported; `test_after.log` and the per-row
`build/rv64_gcc_c_torture_backend/*/case.log` files provide the owner evidence.

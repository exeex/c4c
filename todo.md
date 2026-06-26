Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rerun src/20000112-1.c

# Current Packet

## Just Finished

Step 3 reran the `src/20000112-1.c` representative after commit `2a61ebb6`.
The representative still fails at runtime: clang exits 0, while the c4c RV64
object binary segfaults. Minimal classification shows this remains the same
prior-preservation owner, not a new owner. Current c4c `special_format` saves
and restores `s1`/`s2` and emits later `s1 -> a0` reloads, but it still does not
populate `s1` from the original `a0` before the first `strchr`; the first
observed `s1` population is after the first `strchr` has returned null in `a0`,
so subsequent preserved/reloaded arguments use null.

## Suggested Next

Repair the remaining prior-preservation ordering gap so the initial
preservation home is populated before the first clobbering call, then rerun the
same `src/20000112-1.c` representative.

## Watchouts

Do not broaden into a new owner: the Step 3 evidence still points at the
prior-preservation path. The saved callee register frame support is present in
the current object code; the remaining miss is the timing/source of the first
home population before the initial `strchr`.

## Proof

Ran the exact Step 3 representative command and preserved output in
`test_after.log`:

`bash -o pipefail -c 'mkdir -p build/agent_state && printf "%s\n" "src/20000112-1.c" > build/agent_state/380_residual_step3_20000112.allowlist && ALLOWLIST=build/agent_state/380_residual_step3_20000112.allowlist STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh | tee test_after.log'`

Result: failed, `src/20000112-1.c` remains blocked under the same
prior-preservation owner. Concise artifacts:
`build/agent_state/380_residual_step3_20000112.allowlist`,
`build/agent_state/380_residual_step3_20000112.case.log`,
`build/agent_state/380_residual_step3_20000112.case_tail.txt`,
`build/agent_state/380_residual_step3_20000112.c4c_objdump.txt`,
`build/agent_state/380_residual_step3_20000112.clang_objdump.txt`,
`build/agent_state/380_residual_step3_20000112.special_format_disasm.txt`,
`build/agent_state/380_residual_step3_20000112.clang_special_format_disasm.txt`,
`build/agent_state/380_residual_step3_20000112.c4c_qemu_strace.err`,
`build/agent_state/380_residual_step3_20000112.qemu_rc.txt`, and
`build/agent_state/380_residual_step3_20000112.classification.txt`.

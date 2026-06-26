Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Residual Semantic Repair

# Current Packet

## Just Finished

Step 2 follow-up repaired and covered the remaining prior-preservation ordering
gap in prepared call-boundary planning. Later supported callee-saved GPR
preservation facts now seed earlier calls that pass the same original value
from the same register source, so the first clobbering call carries the needed
`PreservationHomePopulation` effect instead of waiting for a later callsite.
Focused `backend_call_boundary_effect_plan` coverage now proves the matching
register-source seed, the resulting preservation-home population effect, and a
nearby unsupported/non-register preservation source that stays unseeded. The
`src/20000112-1.c` representative remains advanced past the original owner:
`special_format` emits `mv s1,a0` before the first `strchr`, then aborts later
on a short-circuit/select join path that reads `s2` on skip edges where the RHS
call result was never produced.

## Suggested Next

Do not broaden this Step 2 slice into select/join emission. The next owner
should inspect short-circuit select/join materialization in the object route:
prepared BIR has phi-edge immediate materialization bundles for skip edges, but
the emitted object code still recomputes from the RHS result home (`s2`) after
the join.

## Watchouts

Keep the prior-preservation repair scoped to prepared call/value-home facts.
The testable seeding helper is declared in `call_plans.hpp` so focused tests can
exercise the rule without named-source matching. The remaining abort is
classified as an advanced/new owner because the original incoming argument is
now populated into `s1` before the first clobbering call. Relevant artifacts are
under `build/agent_state/380_residual_followup_20000112.*`, especially
`380_residual_followup_20000112.special_format_disasm.txt`,
`380_residual_followup_20000112.prepared_bir.txt`, and
`380_residual_followup_20000112.classification.txt`.

## Proof

Ran the supervisor-selected Step 2 follow-up proof command and preserved output in
`test_after.log`:

`bash -o pipefail -c 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_codegen_route_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_rv64_runtime_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_(obj_)?runtime_rv64_(two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call))$" && mkdir -p build/agent_state && printf "%s\n" "src/20000112-1.c" > build/agent_state/380_residual_followup_20000112.allowlist && ALLOWLIST=build/agent_state/380_residual_followup_20000112.allowlist STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh; } 2>&1 | tee test_after.log'`

Result: build passed and the focused 13-test ctest subset passed, including the
new call-boundary effect coverage. The `src/20000112-1.c` representative failed
with `clang_exit=0` and `c4c_exit=Subprocess aborted`, so the full delegated
proof command failed. Classification: the representative advanced past the
prior-preservation owner and is now blocked by short-circuit select/join
materialization in the object route.

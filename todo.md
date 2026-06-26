Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Residual Prior-Preservation Gap

# Current Packet

## Just Finished

Step 1 audit classified the residual `src/20000112-1.c` failure as a
call-argument prior-preservation publication gap, still owned by idea 380. The
semantic call operands are all `strchr(ptr %p.fmt, ...)`, and the current
prepared metadata recognizes later `strchr` calls as preserving `%p.fmt`
`value_id=0` through `callee_saved_register:s1`. The RV64 object emission still
passes the later arg0 from `register:a0` and emits no home-population
`mv s1,a0` before the first clobbering call and no reload/republication
`mv a0,s1` before the later prepared calls, so the first failed `strchr`
return in call-result home `a0` becomes the next call argument.

## Suggested Next

Implement the smallest semantic lowering rule that makes RV64 object emission
consume prepared `PreservationHomePopulation` and `PreservationRepublication`
effects, or otherwise makes `select_prepared_call_argument_source` mark the
later `%p.fmt` call argument as `PriorPreservation` only when the preserved
home is already populated on every reaching path.

## Watchouts

Keep adjacent shapes fail-closed: do not synthesize a shortcut for
`special_format`, `strchr`, or named block numbers; do not treat every
`register:a0` call argument with a `preserved_values` entry as reloadable unless
the preserved home is populated; do not use the call-result home (`a0` after
`strchr`) as evidence for the original `%p.fmt`; and do not admit stack-slot,
multi-register, FPR, byval, local-frame-address, or unknown-route preservation
through this GPR callee-saved repair unless those routes have their own
prepared selection/effect support. Current object code also does not save the
prepared `s1`/`s2` callee-saved frame-plan homes in `special_format`, so a
repair that starts emitting `s1`/`s2` uses may need matching frame-plan save and
restore support rather than only an argument reload.

## Proof

Audit/classification only; no implementation proof and no root proof logs were
run. Used the existing Step 4 artifacts
`build/agent_state/380_step4_20000112.classification.txt`,
`build/agent_state/380_step4_20000112.c4c_trace_tail.txt`,
`build/agent_state/380_step4_20000112.c4c_qemu_L_strace.err`,
`build/agent_state/380_step4_20000112.clang_qemu_L_strace.err`,
`build/agent_state/380_step4_20000112.c4c_o_objdump.txt`,
`build/agent_state/380_step4_20000112.clang_bin_objdump.txt`,
`build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`, plus fresh
diagnostic dumps
`build/agent_state/380_step1_current_20000112.prepared_bir.txt` and
`build/agent_state/380_step1_current_20000112.semantic_bir.txt`. AST-backed
lookup with `c4c-clang-tool-ccdb` identified the relevant surfaces in
`src/backend/prealloc/call_plans.cpp`,
`src/backend/mir/riscv/codegen/prepared_call_emit.cpp`, and
`src/backend/mir/riscv/codegen/object_emission.cpp`.

Status: Active
Source Idea Path: ideas/open/387_rv64_object_route_same_module_sret_calls.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Or Narrowly Route Sret Calls

# Current Packet

## Just Finished

Step 4 added the narrow prepared same-module RV64 `memory_return`/sret call
route.

The object-emission call route now accepts a prepared same-module call with a
frame-slot `memory_return` whose `sret_arg_index` names an aggregate-address
argument backed by a matching `LocalFrameAddressMaterialization`. For that
shape it materializes the hidden sret pointer with `addi a0, sp, <prepared
frame-slot offset>` before normal call emission. Ordinary scalar argument
emission remains on the existing prepared argument path; the focused fixture
proves the immediate ordinary argument still lands in `a1`.

The route is intentionally fail-closed. It rejects non-same-module memory
returns, non-frame-slot memory returns, missing or mismatched sret argument
indices, missing source-selection facts, mismatched source/materialized
frame-slot offsets, mismatched materialization frame-slot IDs, and dynamic fixed
stack frames through the existing unsupported-instruction diagnostic.

Focused backend coverage added and tightened:
- `builds_prepared_same_module_sret_call_object`
- `rejects_prepared_same_module_sret_call_fail_closed_shapes`

The fail-closed coverage now explicitly proves rejection when `memory_return`
is present on a non-`SameModule` wrapper kind and when the `memory_return` slot
id does not match the sret argument source/materialization facts.

Additional representative evidence:
`build/agent_state/387_step4_920908-1.run.log` shows the `920908-1.c` RV64
object runner now compiles and links the c4c object, then fails at runtime with
`clang_exit=0 c4c_exit=Subprocess aborted`.

`build/agent_state/387_step4_920908-1.c4c-disasm.log` shows the same-module
call in `main` now uses the prepared sret object address and preserves ordinary
arguments:
`addi a0,sp,16`, `li a1,2`, `mv a2,t0`, `mv a3,s2`, followed by an
`R_RISCV_CALL_PLT f` relocation. This means the prior same-module
`memory_return` object-emission admission boundary is gone; the remaining
representative abort is a later runtime/semantic boundary.

## Suggested Next

Run the next packet against the post-call runtime mismatch for `920908-1.c`.
Start from the Step 4 disassembly and compare callee `f` variadic aggregate
consumption against the caller-published variadic aggregate argument payloads.

## Watchouts

- Do not broaden this route beyond prepared same-module frame-slot
  `memory_return` facts without new coverage.
- The sret address is sourced from the explicit prepared call-plan/object facts,
  not from callee names, source filenames, or literal testcase offsets.
- The representative now reaches runtime, so further work should not reclassify
  this as the old `unsupported_instruction_fragment` call gate unless fresh
  evidence contradicts the Step 4 disassembly.

## Proof

Delegated proof command run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed
out of 326`.

Latest focused coverage-tightening proof used the delegated command above and
passed.

Prior focused pre-proof also passed:
`cmake --build --preset default --target backend_riscv_object_emission_test && build/tests/backend/mir/backend_riscv_object_emission_test`.

Representative evidence logs:
- `build/agent_state/387_step4_920908-1.run.log`
- `build/agent_state/387_step4_920908-1.case.log`
- `build/agent_state/387_step4_920908-1.compile.log`
- `build/agent_state/387_step4_920908-1.c4c-disasm.log`

Status: Active
Source Idea Path: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun `va-arg-13.c` And Route The Next Boundary

# Current Packet

## Just Finished

Step 5 reran the `va-arg-13.c` RV64 gcc-torture object-route representative
after the Step 4 prepared-call publication/copy implementation.

The representative still fails at runtime:

- runner diagnostic: `[RV64_BACKEND_RUNTIME_MISMATCH]`
- source: `tests/c/external/gcc_torture/src/va-arg-13.c`
- comparison: `clang_exit=0 c4c_exit=Subprocess aborted`
- runner output: no stdout/stderr payload from either binary

The observable CMake runner symptom did not advance past `Subprocess aborted`,
so idea 390 is not completion-proven by this representative.

The Step 5 disassembly evidence points at a later boundary than the
prepared-call frame-slot-address publication itself. The c4c `test` body now
passes a va-list pointer object into `dummy`, but the pointed-to save area does
not contain the variadic `a1=1234` payload that clang saves in its variadic
function prologue/register-save area before `va_start` consumption. In c4c,
`dummy` loads the va-list pointer and then reads `lw 0(pointer)`; that path can
still reach `abort()` because `test` has not established the expected variadic
argument save payload at that address.

Likely next owner: RV64 variadic function prologue / va-start backing
register-save-area publication for the incoming variadic argument payload, not
another address-materialization-only prepared-call change.

## Suggested Next

Supervisor should route the remaining `va-arg-13.c` abort as a new/later
boundary around RV64 variadic argument save-area setup, or ask the plan owner
whether the current Step 5 evidence exhausts this runbook slice and needs a
follow-on plan packet.

## Watchouts

- Keep idea 389 `va_start` destination-address materialization closed.
- Keep idea 388 `va_end`, idea 386 ordinary frame-slot-address GPR call
  arguments, and idea 387 sret out of scope.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets, or the abort branch.
- Do not broaden this packet into a full `va_arg`, aggregate, or call ABI rewrite.
- Address materialization alone is not proof that the initialized `va_list`
  payload reached `dummy`; the implementation must require a source-to-object
  publication/copy fact and fail closed when the fact is absent or ambiguous.
- The Step 4 route intentionally consumes prepared store-source publication
  facts; do not weaken this to address-materialization-only acceptance.
- Step 5 did not prove idea 390 complete; it exposed that the next abort route
  is likely before `dummy` can read the expected `1234` from the va-list save
  area.
- The runner did not identify which of the two `dummy(state*.ap)` calls aborts.
  The disassembly-level owner classification is therefore a route note, not a
  full root-cause fix.
- A raw direct `qemu-riscv64` attempt outside the CMake runner lacked the
  configured RISC-V dynamic loader path and should not be used as the runtime
  verdict; the CMake runner output in `test_after.log` is authoritative.

## Proof

Command run:

`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Step 5 va-arg-13 RV64 object runner for idea 390'; cmake --build --preset default --target c4cll >/dev/null; case_log=build/agent_state/390_step5_va-arg-13.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; exit 0; } > test_after.log 2>&1`

Result: representative failed as expected for routing evidence; the wrapper
exited 0 and recorded `case_exit=1` from the CMake case runner.

Log paths:

- `test_after.log`
- `build/agent_state/390_step5_va-arg-13.case.log`
- `build/agent_state/390_step5_va-arg-13.c4c_disasm.log`
- `build/agent_state/390_step5_va-arg-13.clang_disasm.log`
- `build/agent_state/390_step5_va-arg-13.qemu_strace.log`

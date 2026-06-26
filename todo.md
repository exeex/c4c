Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun `va-arg-13.c` And Route The Next Boundary

# Current Packet

## Just Finished

Step 5 reran the `va-arg-13.c` RV64 object-route representative after Step 4.
The representative still fails with:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0 c4c_exit=Subprocess aborted
```

The prior abort boundary is not gone, so idea 392 is not completion-proven.
Disassembly evidence shows the RV64 variadic save area is still populated
(`a1` through `a7` saved beginning at `sp+0x48`), but the first and second
`dummy` call argument objects are still overwritten with the `va_list` storage
address (`s1 == sp+0x80`) immediately before passing their object addresses
(`sp+0x18` and `sp+0x20`) to `dummy`. The same Step 392 value-publication
boundary remains; this run did not expose a clearly later boundary.

## Suggested Next

Route back to supervisor/plan review for Step 392. The next coherent packet is
to inspect why the explicit call-argument value-publication route still emits
the `va_list` storage address as payload for the representative despite the
focused object-emission tests passing.

## Watchouts

- Do not solve any remaining representative failure by hard-coding
  `va-arg-13.c`, `test`, `dummy`, offset `0x80`, offset `0x48`, or the abort
  branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer stored into the backing save area.
- Do not treat `missing_frame_slot_arg_publication=yes` or a generic
  `StoreLocalPublication` record alone as enough authority. The implemented
  Step 4 route requires the explicit prepared call-argument value-publication
  fact and its referenced store-source record to agree.
- Preserve the distinction between the argument object's address, the local
  `va_list` storage address, and the initialized save-area pointer payload.
- Generic `StoreLocalPublication` records are useful input facts but are not
  Step 392 callsite authority by themselves. Keep later work on the explicit
  prepared fact surface.
- The failing representative still shows `sd t1,24(sp)` after `mv t1,s1` and
  later `sd t1,32(sp)` after `mv t1,s1`, overwriting the argument objects with
  the storage address rather than the initialized save-area pointer payload.

## Proof

Delegated representative rerun:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Step 5 va-arg-13 RV64 object runner for idea 392'; cmake --build --preset default --target c4cll >/dev/null; case_log=build/agent_state/392_step5_va-arg-13.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; exit 0; } > test_after.log 2>&1`

Result: representative failed, `case_exit=1`, with prior
`Subprocess aborted` runtime mismatch still present. Proof log:
`test_after.log`. Evidence logs:
`build/agent_state/392_step5_va-arg-13.case.log`,
`build/agent_state/392_step5_va-arg-13.c4c-disasm.log`,
`build/agent_state/392_step5_va-arg-13.clang-disasm.log`,
`build/agent_state/392_step5_va-arg-13.qemu-strace.log`.

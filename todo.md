Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` classified the first remaining generic
`expected prepared RV64 object path to reject` in the focused
`backend_riscv_object_emission` failure after commit `37d2020f1`.

GDB evidence in
`build/agent_state/664_step4_generic_rejection_classification/gdb_first_generic_rejection_bt.txt`
stops on the first `fail()` call. The stack reaches
`expect_prepared_rejection_diagnostic(...)`,
`expect_byval_pointer_access_rejection(...)`, and the byval stack-slot
pointer-access fail-closed row. The source row is
`rejects_byval_stack_slot_pointer_access_fail_closed_shapes()` in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`.

The row remains in idea 664 scope. The owner boundary is RISC-V object-emission
infrastructure for prepared local-memory admission and fragment emission:
`src/backend/mir/riscv/codegen/object_emission.cpp` rejects or admits the
prepared local-memory access before object publication, while
`src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp` chooses the
concrete byval/frame-slot/pointer-value load path. The first generic line means
a malformed byval pointer-access shape builds an object module instead of
fail-closing before text publication.

No implementation, tests, expectations, unsupported markers, allowlists,
timeout/runtime policy, baseline accounting, `plan.md`, or source idea files
were edited.

## Suggested Next

Suggested next packet: repair the byval stack-slot pointer-access fail-closed
admission path so malformed prepared pointer-value access facts cannot fall
through to a broader local slot/frame-slot load path and publish object text.
Keep the change general to prepared local-memory admission/emission; do not
rewrite expectations or key on the test row name.

## Watchouts

- The optimized test binary may fold identical helper names in backtraces, but
  the first generic failure resolves to the byval stack-slot pointer-access
  fail-closed row through `expect_byval_pointer_access_rejection(...)`.
- The first focused log line is the ownership target for the next packet. Later
  failures remain in the focused log, including diagnostic exactness,
  pointer-value F64 local, sret stack-homed stores, call-argument publication
  ordering, string/direct-global diagnostics, and the second generic rejection.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, or baseline accounting for this slice.

## Proof

`cmake --build --preset default --target backend_riscv_object_emission_test -j 2 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still fails on
`backend_riscv_object_emission`, starting with the classified byval
stack-slot pointer-access generic rejection. `test_after.log` is the canonical
proof log. Evidence summary:
`build/agent_state/664_step4_generic_rejection_classification/summary.md`.

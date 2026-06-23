Status: Active
Source Idea Path: ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Prepared Pointer Post-Increment Publication

# Current Packet

## Just Finished

Completed Step 3 semantic/prepared repair for loop-carried pointer
post-increment publication.

`lower_memory_gep_inst` now has a narrow loop-carried pointer-publication
compatibility path: the GEP result must be stored back into a local pointer
slot, use a literal constant GEP index, and live in a block with a backedge to
that block. That keeps one-shot pointer-step and indexed pointer access routes
on their existing lowering while fixing the repeated-loop publication boundary.

The focused dump coverage for
`backend_dump_riscv64_loop_carried_pointer_postincrement` was flipped from
expected-repair to positive. It now proves the loop body publishes next pointer
values from the loaded current pointer locals:
`%t29 = bir.add ptr %t28, 2` and `%t32 = bir.add ptr %t31, 2`, followed by
stores back to `%lv.from` and `%lv.to`. It also forbids the old fixed array-base
rematerialization (`%lv.source.0 + 2` / `%lv.sink.0 + 2`).

`backend_codegen_route_riscv64_loop_carried_pointer_postincrement` remains an
expected-repair emission test because RV64 body emission still truncates before
a valid return path for this newly semantic pointer-value loop body. That is a
prepared/RV64 consumption residual, not a semantic publication residual.

## Suggested Next

Repair RV64 prepared consumption/body emission for the loop-carried
pointer-value load/computed-pointer store path, then flip the focused codegen
route to positive and add/enable runtime qemu coverage when the emitted function
has a valid loop body and return path.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block layout, `%lv.from`,
  `%lv.to`, `%t29`, fixed stack offsets, block names, or fixed array sizes.
- Keep the Step 3 boundary at loop-carried local pointer publication. A broader
  attempt to claim all addressed-pointer GEPs or one-shot pointer steps
  regressed closed RV64 routes; those paths should stay on existing lowering
  unless separately planned.
- Do not reopen empty-exit successor, i16 halfword local-array, stack-homed
  fused compare, or one-shot local-array pointer-step routes unless fresh
  evidence proves a regression.
- Keep runtime proof on the supported path; do not mark the candidate
  unsupported or skip qemu to claim progress.

## Proof

Focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(loop_carried_pointer_postincrement|prepared_local_array_pointer_step|empty_loop_exit_successor|i16_local_array_select_store|stack_homed_fused_compare_missing_false_label)'`

Result: passed, 14/14 tests.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 283/284 tests
passing. The sole failure recorded in `test_after.log` remains the existing
`backend_riscv_prepared_edge_publication` contract:
`RISC-V prepared module should emit a register edge move`. The focused
loop-carried pointer dump repair and closed RV64 neighbors from ideas 319, 321,
and 322 passed in both focused and backend runs.

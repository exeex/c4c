Status: Active
Source Idea Path: ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Loop-Carried Pointer Coverage

# Current Packet

## Just Finished

Completed Step 2 focused expected-repair coverage for loop-carried pointer
post-increment publication/lowering.

Added `tests/backend/case/riscv64_loop_carried_pointer_postincrement.c`, a
small halfword-array loop that repeats `*to++ = *from++` and then checks the
copied values plus final pointer positions. The case is independent of
`src/00143.c`, Duff's-device spelling, fixed stack offsets, candidate block
names, candidate SSA names, and candidate array sizes.

Added backend CTest coverage:

- `backend_dump_riscv64_loop_carried_pointer_postincrement`
- `backend_codegen_route_riscv64_loop_carried_pointer_postincrement`

The dump test captures the current expected-repair fact: the loop body loads
the current pointer locals, but the published next pointer values are still
materialized from fixed array bases (`%lv.source.0 + 2` and `%lv.sink.0 + 2`)
instead of from the loaded loop-carried pointer local plus element stride. The
codegen test captures the current expected-repair RV64 truncation at the loop
body before a valid return path is emitted.

## Suggested Next

Repair the loop-carried pointer post-increment lowering so the next pointer
publication is derived from the current pointer local plus stride, then flip the
new expected-repair dump/codegen coverage to positive contracts and add/enable
runtime qemu coverage once RV64 emits a valid body and return path.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block layout, `%lv.from`,
  `%lv.to`, `%t29`, fixed stack offsets, block names, or fixed array sizes.
- Do not avoid repeated fallthrough loop iterations in the focused fixture just
  to make proof pass.
- Do not reopen empty-exit successor, i16 halfword local-array, stack-homed
  fused compare, or nested store-source routes unless fresh evidence proves a
  regression.
- Keep runtime proof on the supported path; do not mark the candidate
  unsupported or skip qemu to claim progress.
- Runtime expected-repair coverage was not kept in this packet because the RV64
  runtime helper reports signal exits as the string `Illegal instruction`, not
  a numeric expected run code. The dump/codegen expected-repair checks are the
  encodable proof for the current gap; runtime should become positive after the
  repair removes truncation.
- Do not satisfy the repair by rematerializing one observed array base or by
  matching the new fixture's local names. The semantic boundary is current
  pointer local plus stride in a repeated loop body.

## Proof

Focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(loop_carried_pointer_postincrement|empty_loop_exit_successor|i16_local_array_select_store|stack_homed_fused_compare_missing_false_label)'`

Result: passed, 11/11 tests.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 281/282 tests
passing before this packet and 283/284 after adding the two expected-repair
tests. The sole failure recorded in `test_after.log` remains the existing
`backend_riscv_prepared_edge_publication` contract:
`RISC-V prepared module should emit a register edge move`. The focused RV64
neighbors from ideas 319, 321, and 322 passed in both focused and backend runs.

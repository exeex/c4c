Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Representative Executable Progress

# Current Packet

## Just Finished

Step 5 added semantic RV64 object-emission support for prepared select
materialization consumers, including the `src/20000113-1.c` block 4
instruction 4 shape in `logic.end.40`:
`%t48 = bir.select ne i32 %t33, 1, i32 1, %t44`. The RV64 object route now uses
`classify_prepared_object_select_consumer` on shared traversal instruction
events, emits scalar compare/branch materialization for ordinary and prepared
join-transfer selects, supports register or stack-slot select result homes, and
returns shared prepared-consumer diagnostics for malformed select carriers.

Focused coverage now builds a prepared join-transfer select materialization
object and checks the generated local branch/jump relocations. The existing
stack/local object-emission support remains intact.

The representative allowlist did not improve yet. `src/20000113-1.c` now gets
past the previous compile-time unsupported prepared select shape and reaches a
runtime mismatch: clang exits 0, while the c4c object binary aborts. No next
unsupported `20000113` object-emission shape was exposed by this proof.

## Suggested Next

Diagnose the new `src/20000113-1.c` runtime mismatch in the RV64 object route,
starting from the linked c4c binary path that reaches `abort` after the
select-materialized short-circuit value.

## Watchouts

- Do not claim allowlist pass-count progress from this packet: current proof remains
  2 passed (`src/20000205-1.c`, `src/20010119-1.c`) and 2 failed
  (`src/20000113-1.c`, `src/20030216-1.c`).
- `src/20000113-1.c` has changed failure mode from compile-time unsupported
  prepared select materialization to runtime abort. The select sequence at
  object offsets `0x3f8-0x408` correctly branches to the generated
  `.Lfoobar_logic_end_40_select_4_true/end` labels for the prepared BIR shape;
  the next investigation should compare the preceding bitfield/scalar value
  feeding the false arm.
- `src/20030216-1.c` remains a separate compile-time unsupported prepared
  module shape and was not diagnosed or claimed here.
- Keep the route shared-traversal-first; do not scan
  `control_flow.parallel_copy_bundles`, do not use
  `prepared_object_parallel_copy_event_kind` from RV64, and do not add
  filename/test-name shortcuts, expectation weakening, rendered-text probes, or
  target-local CFG reconstruction.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true`

Result: build completed; allowlist remained 2/4 passed and 2/4 failed.
`src/20000113-1.c` now fails with `[RV64_BACKEND_RUNTIME_MISMATCH]`
(`clang_exit=0`, `c4c_exit=Subprocess aborted`), while `src/20030216-1.c`
still fails at compile time with `RISC-V backend object route unsupported
prepared module shape`. Proof log: `test_after.log`.

Additional focused validation: `ctest --test-dir build --output-on-failure -R
'^backend_riscv_object_emission$'` passed.

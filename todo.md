Status: Active
Source Idea Path: ideas/open/547_bir_local_memory_call_metadata_boundary_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Local-Memory Evidence

# Current Packet

## Just Finished

Step 1 reconstructed the available `unsupported_local_memory_access` evidence
from existing RV64 gcc torture backend artifacts without creating new logs.

Evidence sources:
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` currently lists
  only `src/20001026-1.c` as failing, and that case reports
  `ambiguous_non_parallel_multi_source_stack_destination`, not
  `unsupported_local_memory_access`.
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` likewise contains
  only `src/20001026-1.c`.
- Existing per-case logs under `build/rv64_gcc_c_torture_backend/*/case.log`
  still contain 41 `unsupported_local_memory_access` rows from the prior
  RV64 work-root sweep. These rows are current on disk, but they are not
  indexed by the latest summary file because the summary appears to have been
  overwritten by the later single-case run.

Row count from retained work-root case logs: 41.

Diagnostic group retained for Step 1:
- `prepared_local_address_base_plus_offset_missing`: 41 rows. Diagnostic:
  `unsupported_local_memory_access: RV64 object route requires prepared
  frame-slot or pointer-value base-plus-offset local memory addressing`.
  First bad fact shape: RV64 reached a prepared local-memory access but did not
  have a consumer-acceptable prepared frame-slot or pointer-value
  base-plus-offset addressing fact. Initial category is `prepared publication /
  address provenance / RV64 consumer evidence gap` until Step 3 proves whether
  the producer failed to publish a valid fact or the RV64 consumer is rejecting
  an available one.

Rows in that group:
- `src/20000519-1.c`
- `src/20000722-1.c`
- `src/20010123-1.c`
- `src/20011109-2.c`
- `src/20020215-1.c`
- `src/20021204-1.c`
- `src/20030606-1.c`
- `src/20030920-1.c`
- `src/20031204-1.c`
- `src/20041019-1.c`
- `src/20060929-1.c`
- `src/20070212-3.c`
- `src/20071213-1.c`
- `src/20080519-1.c`
- `src/20120808-1.c`
- `src/20140828-1.c`
- `src/920429-1.c`
- `src/920506-1.c`
- `src/921117-1.c`
- `src/930429-1.c`
- `src/941110-1.c`
- `src/950628-1.c`
- `src/950929-1.c`
- `src/complex-7.c`
- `src/ipa-sra-2.c`
- `src/pr20466-1.c`
- `src/pr30185.c`
- `src/pr35800.c`
- `src/pr38051.c`
- `src/pr38969.c`
- `src/pr43835.c`
- `src/pr58984.c`
- `src/pr60017.c`
- `src/pr60822.c`
- `src/pr65369.c`
- `src/pr68185.c`
- `src/pr68250.c`
- `src/pr68321.c`
- `src/pr70005.c`
- `src/pr78675.c`
- `src/pr88739.c`

Representative case logs:
- `build/rv64_gcc_c_torture_backend/src_20000519-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20070212-3.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr35800.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr65369.c/case.log`

Smallest reproduction command available from the existing runner shape:

```sh
cmake --build build --target c4cll &&
cmake \
  -DCOMPILER=/workspaces/c4c/build/c4cll \
  -DCLANG="$(command -v clang)" \
  -DQEMU_RISCV64="$(command -v qemu-riscv64)" \
  -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000519-1.c \
  -DROOT=/workspaces/c4c/tests/c/external/gcc_torture \
  -DTARGET_TRIPLE=riscv64-linux-gnu \
  -DSYSROOT=/usr/riscv64-linux-gnu \
  -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000519-1.c/clang.bin \
  -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000519-1.c/c4c.o \
  -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000519-1.c/c4c.bin \
  -DCASE_TIMEOUT_SEC=20 \
  -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake
```

Unresolved owner questions for Step 3:
- Is the 41-row work-root set still the intended canonical row set even though
  the latest summary/failed files now describe only the later `src/20001026-1.c`
  single-case run?
- For representative retained rows, does prepared BIR contain a frame-slot or
  pointer-value base-plus-offset fact that RV64 fails to consume, or is the
  prepared producer failing to publish that fact?
- Are any retained rows stale because their per-case logs predate a relevant
  later implementation change, or should they remain active evidence until a
  fresh full RV64 torture scan replaces the work-root case logs?

## Suggested Next

Execute Step 2 by reconstructing current call-metadata evidence from existing
RV64 gcc torture backend artifacts, keeping summary-file rows separate from
retained work-root case logs if they disagree.

## Watchouts

- Do not implement RV64 lowering in this review runbook.
- Do not treat call-metadata suspicions as implementation-ready before row-level
  evidence exists.
- Do not weaken tests, unsupported markers, expected output, or pass/fail
  accounting.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.

## Proof

Step 1 evidence-recording validation:

```sh
git diff --check -- todo.md
```

Result: passed with no output.

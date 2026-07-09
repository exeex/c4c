Status: Active
Source Idea Path: ideas/open/642_rv64_global_residual_runtime_mismatch_research.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Candidate Owners

# Current Packet

## Just Finished

Completed plan Step 2, `Trace Candidate Owners`, for
`src/pr79737-2.c`.

Strongest first owner: packed bitfield/global-object layout and bitfield
access lowering, not true runtime support. The source declares two packed
file-scope instances of a 72-bit bitfield struct:
`18 + 1 + 24 + 15 + 14 = 72` bits, so the clang control lays out `i` and `j`
as 9-byte globals. C4C lays them out as 12-byte globals and emits 32-bit
word-lane accesses at offsets `0`, `4`, and `8`; the different assignment
orders in `foo` and `bar` then feed the final self-check and reach
`__builtin_abort()`.

Concrete owner evidence:
- Step 1 runtime boundary:
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/case.log` reports
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
  `c4c_exit=Subprocess aborted`.
- Symbol/layout evidence:
  `build/agent_state/642_step2_symbols.txt` records C4C `c4c.o` symbols
  `i` size `12`, `j` size `12`, with linked addresses `i=0x2019`,
  `j=0x2025`. The same file records clang linked symbols `i` size `9`,
  `j` size `9`, with addresses `i=0x2019`, `j=0x2022`.
- C4C access evidence:
  `build/agent_state/642_step2_c4c_o_disasm.txt` and
  `build/agent_state/642_step2_c4c_bin_disasm.txt` show C4C accessing `i`
  and `j` through `lw`/`sw` word lanes at object offsets `0`, `4`, and `8`.
- Clang control evidence:
  `build/agent_state/642_step2_clang_bin_disasm.txt` shows byte-oriented
  packed accesses with `lbu`/`sb` over the 9-byte objects, including base
  addresses `i=0x2019` and `j=0x2022`.

Candidate-owner classification:
- global/local memory lowering: ruled out as first owner. Global symbol
  address materialization works far enough to compile, link, and execute, and
  the direct global symbols resolve to concrete `i`/`j` addresses. The first
  proven bad fact is the packed bitfield object size/lane model consumed by
  those memory accesses.
- object relocation: ruled out. `build/agent_state/642_step2_relocations.txt`
  shows the linked C4C and clang binaries both use normal
  `R_RISCV_RELATIVE`, `R_RISCV_64`, and `R_RISCV_JUMP_SLOT` entries, with
  matching jump-slot relocations for `__libc_start_main` and `abort`; there is
  no loader relocation assertion like the `src/990106-1.c` call-lowering row.
- stack layout: ruled out. The checked values live in file-scope globals
  `i` and `j`; stack traffic in the C4C `main` disassembly is temporary
  comparison scratch, not the storage home for the packed objects.
- ABI/call setup: ruled out as first owner. `foo()` and `bar()` have no
  parameters or return value, and `main` reaches both direct calls before the
  final predicate. The only external call involved in the failure is `abort`,
  which is reached after the generated self-check decides the globals differ.
- branch/control-flow: ruled out as first owner. The branch to `abort@plt`
  appears after comparisons over loaded bitfield values in
  `build/agent_state/642_step2_c4c_bin_disasm.txt`; there is no timeout,
  bad target, or unreachable-control evidence independent of the bad data.
- true runtime support: ruled out. The clang control binary exits `0` under
  the same harness, while the C4C binary calls libc `abort` after its own
  source-level predicate fails. That is a miscompile reaching a test abort,
  not missing qemu/libc support.

Follow-up proof surface: publish the research artifact with this
classification, then open a separate implementation idea only if the
supervisor wants to pursue packed bitfield layout/global access support. That
follow-up should prove C4C reports 9-byte packed struct globals for this shape,
uses packed byte-lane bitfield stores/loads instead of 12-byte word-lane
storage, and reruns `src/pr79737-2.c` without weakening expectations.

## Suggested Next

Start Step 3, `Publish The Research Artifact`, by adding
`docs/runtime_mismatch_ownership/04_global_residual_runtime_mismatch.md` and
linking it from `docs/runtime_mismatch_ownership/index.md`. Include the Step 1
rerun command, the Step 2 candidate-owner table above, the strongest
packed-bitfield/global-layout classification, and the separate follow-up proof
surface.

## Watchouts

- Do not implement code changes inside this research idea.
- Do not treat the runtime mismatch symptom alone as proof of true runtime
  support ownership; this row is now classified by object layout, symbols, and
  disassembly.
- Keep any implementation recommendation separate from idea 642. A fix belongs
  to a packed bitfield/global-layout follow-up, not to this research packet.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime comparison behavior, or accounting.

## Proof

No new root proof log was required for this evidence-only packet. Preserved the
existing Step 1 `test_after.log` and inspected the existing row artifacts under
`build/rv64_gcc_c_torture_backend/src_pr79737-2.c/`.

Inspection artifacts generated under `build/agent_state/`:
- `build/agent_state/642_step2_symbols.txt`
- `build/agent_state/642_step2_relocations.txt`
- `build/agent_state/642_step2_c4c_o_disasm.txt`
- `build/agent_state/642_step2_c4c_bin_disasm.txt`
- `build/agent_state/642_step2_clang_bin_disasm.txt`

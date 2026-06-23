Status: Active
Source Idea Path: ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Loop-Carried Pointer Evidence

# Current Packet

## Just Finished

Completed Step 1 evidence normalization for
`tests/c/external/c-testsuite/src/00143.c`.

Artifacts are under
`build/rv64_c_testsuite_probe_latest/triage_323_step1/`:
`probe_results.tsv`, `summary.md`, BIR/prepared-BIR dumps, RV64 assembly,
linked binary metadata, c4cll qemu logs, and clang-reference qemu logs.

Fresh status:

| case | BIR | prepared BIR | emit | clang link | c4cll qemu | clang ref qemu | classification |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `src/00143.c` | 0 | 0 | 0 | 0 | 1 | 0 | `bir_fixed_array_base_postincrement_residual` |

The first bad mechanism is already visible before RV64 emission: pointer
post-increment values are derived from fixed array bases instead of the current
loop-carried pointer locals. In `block_6`, BIR loads `%lv.from` / `%lv.to` but
stores `%lv.a.0 + 2` and `%lv.b.0 + 2`; later blocks repeat fixed base offsets
such as `%lv.a.0 + 16` and `%lv.b.0 + 16`. The emitted RV64 follows those facts
by storing `sp + 2`, `sp + 80`, `sp + 16`, and `sp + 94` into the pointer homes.
Because the Duff-style body repeats through the loop, those next pointer values
need to be based on the current pointer local plus stride.

This is distinct from prior routes: stack-homed fused compare branch labels are
present, semantic i16 `lh`/`sh` body emission is present, and the empty loop-exit
successor now reaches a normal return path. The remaining failure is a normal
qemu exit status 1 for the c4cll binary while the clang reference exits 0.

## Suggested Next

Add focused expected-repair backend coverage for loop-carried pointer
post-increment publication/lowering: a pointer local updated in a repeated loop
body should publish the next value from the current pointer local plus element
stride, not rematerialize from the fixed array base.

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
- The first bad fact appears in BIR/prepared facts, so Step 2 coverage should
  assert the semantic loop-carried pointer update rather than only matching
  emitted RV64 text.

## Proof

Probe commands run for `src/00143.c`: dump BIR, dump prepared BIR, RV64 asm
emit, clang link with `--target=riscv64-linux-gnu --gcc-toolchain=/usr`, qemu
with `-L /usr/riscv64-linux-gnu`, and clang-reference qemu.

Delegated proof still to run:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 281/282 tests
passing. The sole failure recorded in `test_after.log` is the existing
`backend_riscv_prepared_edge_publication` contract:
`RISC-V prepared module should emit a register edge move`. The focused RV64
neighbors for fused compare, i16 local-array halfword emission, and empty
loop-exit successor passed in this backend run.

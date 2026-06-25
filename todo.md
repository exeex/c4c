Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit the Multi-Block Rejection Boundary

# Current Packet

## Just Finished

Step 1 audit completed. The narrow representative scan still fails all four
cases with the coarse object-route diagnostic:

`error: --codegen obj failed: RISC-V backend object route unsupported prepared module shape`

Boundary table:

| Case | Primary layer | Evidence | Notes |
| --- | --- | --- | --- |
| `src/20000113-1.c` | object emission single-block gate | `--dump-prepared-bir` succeeds and shows `foobar` with 7 BIR blocks plus prepared branch conditions and edge-copy obligations; object route rejects in `prepared_function_to_object_function` before instruction/call inspection because `function->blocks.size() != 1` at `src/backend/mir/riscv/codegen/object_emission.cpp:936`-`940`. | There are later external `abort`/`exit` blockers in the `.s` path, but the first object-route blocker for the defined function is the single-block gate. |
| `src/20000205-1.c` | object emission single-block gate | `--dump-prepared-bir` succeeds and shows `f` with 8 BIR blocks and `main` with 3 BIR blocks; prepared control-flow records fused branch conditions. Object route rejects on the same `function->blocks.size() != 1` gate at `object_emission.cpp:936`-`940`. | This is the cleanest representative for the 356 repair because BIR/prepared control flow is present and the first object-route gate is structural, not source parsing. |
| `src/20010119-1.c` | unrelated globals/strings/ABI blocker | `--dump-prepared-bir` succeeds and both defined functions are single-block, but prepared control flow also contains declaration `exit`; `build_rv64_prepared_text_object_module` iterates every prepared control-flow entry and rejects when `find_defined_bir_function` returns null for declarations at `object_emission.cpp:936`-`940` via the loop at `object_emission.cpp:1641`-`1644`. | Not a multi-block case after current frontend/prepared lowering; treat as declaration/external-call object-route handling, likely outside the first 356 repair. |
| `src/20030216-1.c` | unrelated globals/strings/ABI blocker | `--dump-prepared-bir` succeeds and `main` is single-block, but object route rejects modules with any globals or string constants before function conversion at `object_emission.cpp:1633`-`1637`; `--codegen asm` reports unsupported prepared global storage layout. | This belongs to idea 357 data/global-section support, not 356 multi-block routing. |

Layer conclusion: the first real 356 repair target is not BIR CFG ownership and
not MIR/pseudo lowering. Prepared BIR already records CFG, edge transfers,
branch conditions, and regalloc summaries. The first true 356 blocker is the
RV64 object emission handoff: it currently converts only one-block defined
functions into a linear `RiscvObjectFunction` fragment stream, while the
prepared `.s` emitter already iterates BIR blocks and emits labels/branches in
`src/backend/mir/riscv/codegen/prepared_function_emit.cpp:391`-`401` and
`595`-`690`.

## Suggested Next

Define the Step 2 object-emission handoff contract. The next implementation
packet should not try to "fix CFG" in MIR/assembler. It should define what
target-block/label/instruction stream object emission consumes after prepared
BIR has already preserved CFG, then decide how to replace the one-block
`RiscvObjectFunction` fragment contract without duplicating high-level CFG
lowering.

## Watchouts

- Do not edit backend implementation during Step 1.
- Do not move CFG semantics into MIR, assembler, or object emission.
- Do not ask the assembler to understand loops, if/else, switch, liveness, or
  inline-asm operand relationships.
- If a representative case is blocked by globals/strings/data sections, mark it
  as an idea 357 blocker instead of forcing it into 356.
- The `.s` path is not a clean oracle for these four cases yet: the first three
  hit unsupported runtime external symbols (`exit`/`abort`) and `20030216-1.c`
  hits global storage layout. Use the prepared-BIR dump and object-route code
  to identify the first object-route gate.
- `20010119-1.c` is no longer a multi-block representative after current
  lowering. It is useful as a declaration/external-call object-route blocker,
  but should not drive the first multi-block repair.

## Proof

Proof log: `test_after.log`

Narrow audit command:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Result: `total=4 passed=0 failed=4`, expected for audit. Per-case logs are
under `build/rv64_gcc_c_torture_backend/*/case.log`.

Additional inspected commands:

```sh
build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir \
  --target riscv64-linux-gnu tests/c/external/gcc_torture/src/<case>.c

build/c4cll -I tests/c/external/gcc_torture --codegen asm \
  --target riscv64-linux-gnu tests/c/external/gcc_torture/src/<case>.c \
  -o /tmp/rv64-multiblock-audit/<case>.s
```

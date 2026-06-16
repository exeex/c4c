# RV64 Runtime Scalar And Local Foundation

## Goal

Extend the qemu-backed rv64 backend runtime path beyond immediate branch
smokes so it can execute a small foundation of scalar arithmetic, return-value,
and local-slot backend cases from `tests/backend/case/`.

## Starting Point

Idea 295 established the first rv64 runtime route:

- `c4c --codegen asm --target riscv64-linux-gnu`;
- runner-side rejection of BIR/LLVM fallback text;
- clang rv64 cross compile/link;
- `qemu-riscv64 -L /usr/riscv64-linux-gnu`;
- 10 passing `branch_if_*` cases.

That route currently proves simple immediate integer compare branches and
immediate integer returns. The next useful capability step is to make the same
public route handle basic scalar values and local storage without broad ABI
work.

## In Scope

- Add rv64 runtime tests, using the helper from idea 295, for a small ordered
  set of existing cases such as:
  - `return_zero.c`;
  - `return_add.c`;
  - `return_add_sub_chain.c`;
  - `local_temp.c`;
  - `local_array.c` or another tiny local-slot case if the first local case is
    too broad.
- Repair the rv64 prepared-BIR emitter semantically enough to support:
  - integer binary ops used by the selected cases;
  - named temporary values;
  - returning a named scalar value;
  - the smallest necessary local-slot load/store path.
- Keep the qemu runner as the acceptance boundary.
- Keep existing semantic-BIR observation tests intact.

## Out of Scope

- Full rv64 ABI completion.
- Function-call lowering, aggregate returns, varargs, global memory, or pointer
  provenance beyond what a chosen tiny local case strictly needs.
- Moving backend case files into target-specific directories.
- Rewriting the rv64 backend around a new architecture.
- Testcase-name or exact-file-shape shortcuts.

## Suggested Execution Order

1. Register `return_zero.c` and verify it passes without broad emitter changes.
2. Register `return_add.c`; implement named scalar value materialization if it
   fails.
3. Register `return_add_sub_chain.c`; generalize binary-op value tracking
   instead of adding one-off instruction sequences.
4. Try one tiny local-slot case. If every local case immediately pulls in broad
   stack/addressing work, stop and open a narrower local-slot idea instead of
   stretching this one.
5. Run the rv64 runtime subset and a focused backend route subset.

## Acceptance Criteria

- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with the new scalar/local cases included.
- The runner still rejects `bir.func` and LLVM IR fallback before linking.
- At least one non-branch scalar arithmetic case executes under qemu.
- If a local-slot case is included, it executes under qemu without weakening
  existing tests.
- Existing RISC-V prepared/semantic observation tests still pass:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  shared backend routing or prepared emitter behavior changes materially.

## Reviewer Reject Signals

- The route passes by accepting BIR or LLVM text as if it were RISC-V assembly.
- Compiler behavior is keyed to a testcase filename or a single exact source
  shape.
- Local-slot work expands into broad memory/aggregate/global support without a
  new source idea.
- Existing AArch64/backend route tests are weakened or moved as part of this
  semantic rv64 capability slice.
- The qemu proof is replaced by host-native execution.

## Notes For The Next Agent

- Start from the helper in `tests/backend/CMakeLists.txt`:
  `c4c_add_backend_rv64_runtime_case(...)`.
- The initial rv64 prepared emitter core lives in
  `src/backend/mir/riscv/codegen/emit.cpp`.
- Prefer a tiny value environment for named scalar temporaries before touching
  stack/local-slot machinery.
- Treat target-specific case directory cleanup as a separate mechanical idea
  unless the runtime expansion directly exposes a concrete directory ownership
  problem.

## Completion Note

Closed after Step 5 validation. The rv64 runtime subset passes 14/14 with
`return_zero.c`, `return_add.c`, `return_add_sub_chain.c`, and `local_temp.c`
registered under qemu. The focused RISC-V/backend route validation is monotonic
against the accepted baseline: `backend_riscv_prepared_edge_publication`
remains the single known pre-existing failure, with no new failures.

# RV64 Backend QEMU Runtime Bringup

## Goal

Bring up a small, repeatable RISC-V 64 backend runtime test path that compiles
existing backend cases through `c4cll --codegen asm --target riscv64-linux-gnu`,
assembles/links them with LLVM/clang cross compilation, and executes them under
`qemu-riscv64`.

## Why This Exists

The environment now has the required rv64 tools:

- `qemu-riscv64` and `qemu-system-riscv64` are installed.
- LLVM advertises `riscv32` and `riscv64` targets.
- `riscv64-linux-gnu-ld` and `/usr/riscv64-linux-gnu` sysroot are available.

The current public backend route for a simple case such as
`tests/backend/case/branch_if_eq.c` does not yet prove rv64 execution. Running
`c4cll --codegen asm --target riscv64-linux-gnu` currently exposes that the
public C-file LIR handoff can still fall back to backend BIR text instead of
terminal RISC-V assembly. The first bringup should therefore make the route
honest: either emit assemblable rv64 assembly or fail loudly before qemu.

## In Scope

- Add a backend CTest helper for rv64 runtime cases in
  `tests/backend/CMakeLists.txt`.
- Add a CMake runner under `tests/backend/cmake/` that performs:
  - `c4cll --codegen asm --target riscv64-linux-gnu <case> -o <out.s>`;
  - rejection of BIR/LLVM fallback text;
  - `clang --target=riscv64-linux-gnu --gcc-toolchain=/usr <out.s> -o <bin>`;
  - `qemu-riscv64 -L /usr/riscv64-linux-gnu <bin>`.
- Start with tiny existing `tests/backend/case/` cases such as
  `branch_if_eq.c` and nearby branch/scalar cases whose expected result is
  program exit 0.
- Repair the rv64 public LIR handoff only enough to make the basic cases emit
  real RISC-V assembly.
- Keep the proof semantic: do not add testcase-shaped shortcuts for
  `branch_if_eq.c`.
- Decide how to keep target-specific cases discoverable. If directory
  organization changes are needed, prefer a follow-up step that moves obvious
  AArch64-only cases under an AArch64-specific subdirectory and updates CMake
  references mechanically.

## Out of Scope

- Broad RISC-V ABI completion.
- Moving every backend case before a first rv64 runtime proof exists.
- Downgrading existing AArch64/backend route tests.
- Treating BIR text emission as a passing rv64 runtime route.
- Adding special-case compiler logic keyed to a testcase filename or exact
  branch shape.

## Acceptance Criteria

- At least one rv64 backend runtime CTest compiles an existing backend case via
  c4cll-generated RISC-V assembly, links it with clang's rv64 target, and runs
  successfully under qemu.
- The rv64 runner rejects fallback text such as `bir.func` or `define ... @`
  before attempting to link.
- The helper can add additional rv64 runtime cases without repeating the full
  command sequence.
- Existing backend route tests still pass for the touched area.
- Any target-specific case directory reshuffle is mechanical, scoped, and
  validated by CTest names that referenced the moved files.

## Reviewer Reject Signals

- A test passes by accepting BIR/LLVM text instead of real RISC-V assembly.
- The implementation hard-codes behavior for `branch_if_eq.c` or a single
  exact control-flow shape.
- A large case-directory move is mixed with semantic backend repair in one
  hard-to-review commit.
- The rv64 CTest depends on host-native execution instead of qemu.
- Existing AArch64 route references are broken by directory cleanup.

## Closure Note

Closed after establishing the first qemu-backed rv64 backend runtime route.

Implemented:

- Added reusable rv64 runtime CTest plumbing in `tests/backend/CMakeLists.txt`
  and `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`.
- The runner rejects BIR fallback text (`bir.func`) and LLVM IR fallback text
  before attempting to link.
- The runner compiles c4cll-generated RISC-V assembly with
  `clang --target=riscv64-linux-gnu --gcc-toolchain=/usr`, then executes the
  linked binary with `qemu-riscv64 -L /usr/riscv64-linux-gnu`.
- Repaired the public rv64 LIR handoff in `src/backend/backend.cpp` so
  `--codegen asm --target riscv64-linux-gnu` routes through the prepared rv64
  emitter instead of returning prepared BIR text.
- Added a small semantic rv64 prepared-BIR emitter core for simple immediate
  integer compare branches and immediate integer returns.
- Registered all existing `tests/backend/case/branch_if_*.c` cases as
  qemu-backed rv64 runtime tests.

Validation:

- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passed 10/10.
- `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  passed 3/3.
- `ctest --test-dir build -R '^backend_' --output-on-failure` passed 190/190.

Directory organization decision:

- No case-directory reshuffle was needed for this bringup. The rv64 helper can
  consume shared `tests/backend/case/` files directly.
- Moving AArch64-only cases under a target-specific subdirectory should remain
  a separate mechanical cleanup only if future rv64 coverage makes the shared
  case directory hard to scan. It should not be mixed with semantic rv64 backend
  repair.

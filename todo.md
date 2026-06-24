Status: Active
Source Idea Path: ideas/open/329_native_object_emission_umbrella.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inspect Runtime And Backend Test Wiring

# Current Packet

## Just Finished

Completed Step 2 research: inspected runtime, backend route, and c-testsuite
wiring for current asm-output routes and recorded where future object-route
tests can sit beside them without touching implementation files, tests, CLI
runtime wiring, or child idea files.

Inspected commands/files:

- `git status --short`
- `rg --files scripts tests cmake src | rg '(c_testsuite|backend|runtime|codegen|CMakeLists|\\.cmake$)'`
- `find . -maxdepth 3 -type f \\( -name 'CMakeLists.txt' -o -name '*.cmake' \\) | sort`
- Targeted `rg` searches for:
  - `clang`, `.s`, `runtime`, `link`, `c_testsuite`, `--codegen`, backend route labels
  - backend route helper registrations and output variables
  - `--codegen asm`, `--codegen obj`, `object_emitted`, and public assemble APIs
- Targeted reads:
  - `src/apps/c4cll.cpp`
  - `src/backend/backend.hpp`
  - `src/backend/backend.cpp`
  - `src/codegen/llvm/llvm_codegen.hpp`
  - `src/codegen/llvm/llvm_codegen.cpp`
  - `tests/CMakeLists.txt`
  - `tests/backend/CMakeLists.txt`
  - `tests/backend/cmake/run_backend_codegen_route_case.cmake`
  - `tests/backend/cmake/run_backend_dual_codegen_route_case.cmake`
  - `tests/backend/cmake/run_backend_codegen_failure_case.cmake`
  - `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`
  - `tests/backend/cmake/run_backend_aarch64_external_smoke.cmake`
  - `tests/backend/cmake/run_backend_aarch64_selected_indirect_call_case.cmake`
  - `tests/c/external/CMakeLists.txt`
  - `tests/c/external/c-testsuite/RunCase.cmake`
  - `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`
  - `tests/backend/bir/backend_x86_handoff_boundary_lir_test.cpp`
  - `src/backend/mir/aarch64/BINARY_UTILS_CONTRACT.md`

Current `.s` to toolchain/runtime flow:

- `src/apps/c4cll.cpp` accepts only `--codegen llvm|asm|compare`; there is no
  CLI `--codegen obj` today. `--codegen asm` maps to `CodegenPath::Lir`, routes
  through `llvm_backend::emit_module_native(...)`, and writes the returned text
  to `-o` or stdout. It explicitly rejects LLVM-IR fallback when the user asked
  for asm-like output. The app does not assemble, link, or run backend output.
- `src/codegen/llvm/llvm_codegen.cpp` lowers HIR to LIR, then backend-enabled
  `CodegenPath::Lir` calls `backend::emit_module(...)`.
- `src/backend/backend.cpp` routes target-local asm output through
  `emit_aarch64_lir_module_entry(...)` or `emit_riscv_lir_module_entry(...)`
  for AArch64/RV64 targets. The public assemble surface
  `assemble_target_lir_module(...)` currently stages emitted text, records the
  requested object path, and reports `object_emitted = false` with
  `backend bootstrap mode does not assemble objects yet`.
- `tests/backend/cmake/run_backend_codegen_route_case.cmake` is the main asm
  route harness: it runs
  `c4cll --codegen asm --target <triple> <src> -o <out>.s`, then validates text
  snippets only. This is the right sibling for future object-route emission
  contract tests that should compare output form without executing binaries.
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake` is the RV64
  execution harness: it emits `.s` with `--codegen asm`, compiles it with
  `clang --target=riscv64-linux-gnu --gcc-toolchain=/usr <out>.s -o <bin>`,
  then runs `qemu-riscv64 -L /usr/riscv64-linux-gnu <bin>`. This is where RV64
  direct-object runtime tests can sit after direct `.o` exists, replacing only
  the `.s -> clang` input form while preserving the qemu/runtime comparison.
- `tests/backend/cmake/run_backend_aarch64_external_smoke.cmake` emits AArch64
  `.s`, validates snippets, assembles with `aarch64-linux-gnu-as -o <out>.o`,
  links with `clang <out>.s -o <bin>`, and only runs on AArch64 hosts. This is
  the existing AArch64 external assembler/link smoke sibling for early object
  writer validation.
- `tests/c/external/c-testsuite/RunCase.cmake` uses two modes:
  - `frontend`: pipes compiler LLVM IR into `clang -x ir - -o <bin> -lm`.
  - `backend-*`: runs `c4cll --codegen asm --target <triple> <src> -o <OUT_LL>`,
    verifies the output starts like asm, then runs
    `clang --target=<triple> -x assembler <OUT_LL> -o <bin> -lm`.
    The variable name `OUT_LL` is historical here; in backend mode it holds asm
    text, not LLVM IR.
- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake` is the explicit
  AArch64 backend scan runner. It also emits `.s` via `--codegen asm`, compiles
  with `clang --target=aarch64-unknown-linux-gnu -x assembler <OUT_LL> -o
  <bin> -lm`, then runs either directly on AArch64/ARM64 hosts or through
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.

How backend tests distinguish route, target, and output form:

- Test names encode route and target today, e.g.
  `backend_codegen_route_*`, `backend_rv64_runtime_*`,
  `c_testsuite_aarch64_backend_*`, and `c_testsuite_x86_backend_*`.
- `tests/backend/CMakeLists.txt` centralizes labels with
  `c4c_set_backend_test_labels(...)`, always adding `internal;backend` plus
  route labels such as `backend_route`, `backend_runtime`, `backend_dump`,
  `rv64`, `qemu`, `aarch64`, `emission`, `output_contract`,
  `expected_failure`, and feature labels.
- `c4c_add_backend_codegen_route_test(...)` registers asm text-output checks
  through `run_backend_codegen_route_case.cmake`. The output form is implicit
  in `--codegen asm` and `.s` output paths.
- `c4c_add_backend_codegen_failure_test(...)` verifies asm-route diagnostic
  rejection and asserts no output file is produced.
- `c4c_add_backend_dump_flag_test(...)` is observational BIR/MIR text and is
  not an object-output proof surface.
- `c4c_add_backend_rv64_runtime_case(...)` labels runtime cases
  `backend_runtime;rv64;qemu;case;...` and stores products under
  `build/backend_rv64_runtime/<case>.s` and `.bin`.
- c-testsuite backend cases are opt-in through
  `ENABLE_C_TESTSUITE_BACKEND_TESTS` for host-selected x86/AArch64, or
  `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN` for explicit AArch64. They label
  tests as `c_testsuite;<target>_backend`, plus
  `aarch64_backend_smoke` for the first three smoke cases.
- Existing C++ API tests in
  `tests/backend/bir/backend_x86_handoff_boundary_lir_test.cpp` already pin
  the bootstrap assemble contract: staged text is produced, requested output
  path is preserved, `object_emitted` remains false, and the error string says
  objects are not assembled yet. Direct object-route child ideas should update
  or add sibling tests for the new object result contract instead of weakening
  this coverage accidentally.

Where future object-route tests can sit:

- Add a sibling backend helper near `c4c_add_backend_codegen_route_test(...)`,
  likely `c4c_add_backend_object_route_test(...)`, that invokes the future
  explicit object output mode, writes `OUT_OBJ`, verifies the file exists, and
  inspects object structure with repo-native or external tools. Use labels like
  `backend_object_route`, target labels (`rv64`, `aarch64`), and feature labels.
- Add CMake runner siblings under `tests/backend/cmake/`, rather than changing
  the existing asm-route runners in place. Keep `.s` route tests green as the
  compatibility baseline.
- For RV64 execution, add an object-output sibling beside
  `run_backend_rv64_runtime_case.cmake` that compiles/links the generated
  object into a binary and keeps the same `qemu-riscv64 -L
  /usr/riscv64-linux-gnu` runtime check. The first proof labels should mirror
  existing `backend_runtime;rv64;qemu;case` and add an object-specific label.
- For AArch64, add object-contract tests beside
  `run_backend_aarch64_external_smoke.cmake` and existing binary-utils contract
  expectations. `src/backend/mir/aarch64/BINARY_UTILS_CONTRACT.md` already
  names useful object fixtures: minimal return, global-load relocations, and an
  external-call relocation fixture.
- For c-testsuite, add a distinct backend object mode rather than overloading
  `CODEGEN_MODE=backend-*`. A future mode such as `backend-aarch64-obj` or a
  separate runner should generate an object path, link it with `clang
  --target=<triple> <OUT_OBJ> -o <bin> -lm`, then reuse the same expected-output
  runtime comparison. Avoid continuing the misleading `OUT_LL` naming for new
  object paths.

Likely proof commands and labels for child ideas:

- Narrow backend asm compatibility after route work:
  `ctest --test-dir build -R '^backend_codegen_route_' --output-on-failure`
  or a target-specific subset such as
  `ctest --test-dir build -L 'backend_route' -R 'riscv64|aarch64' --output-on-failure`.
- RV64 runtime object-route proof once implemented:
  `ctest --test-dir build -R '^backend_rv64_runtime_.*object' --output-on-failure`
  with labels `backend_runtime;rv64;qemu;object_route`.
- AArch64 object contract proof once implemented:
  `ctest --test-dir build -R '^backend_contract_aarch64_.*object|^backend_object_route_aarch64_' --output-on-failure`
  with labels `backend_object_route;aarch64;object_contract`.
- c-testsuite object scan smoke proof once implemented:
  `ctest --test-dir build -L 'c_testsuite' -R 'c_testsuite_aarch64_backend_.*object|c_testsuite_rv64_backend_.*object' --output-on-failure`
  with labels `c_testsuite;<target>_backend;object_route` and optional smoke
  labels mirroring `aarch64_backend_smoke`.

## Suggested Next

Execute Step 3 by inspecting the existing RV64 assembler/object port under
`src/backend/mir/riscv/assembler/`: public API shape, encoded instruction and
relocation records, section/symbol handling, known gaps, and whether pieces are
reusable for compiler direct `.o` output without text round-tripping.

## Watchouts

- Existing backend route tests are asm-first by construction; future object
  tests should be additive siblings, not rewrites of `--codegen asm` runners.
- `tests/c/external/c-testsuite/RunCase.cmake` calls backend asm output
  `OUT_LL` in backend mode. New object-route wiring should avoid copying that
  name because it obscures output form.
- There is no accepted `--codegen obj` option today. Child ideas need a CLI/API
  contract before tests can invoke direct object output.
- The current public assemble API is a bootstrap staging contract with
  `object_emitted = false`; changing it is a route/API child, not just a test
  update.
- Keep c-testsuite default behavior on LLVM/frontend or existing asm backend
  modes until direct object-route parity is intentionally proven.

## Proof

Research-only packet; no build or test validation required and no
`test_after.log` was written. Proof is the inspected file/command list recorded
above in `todo.md`.

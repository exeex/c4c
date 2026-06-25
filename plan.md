# c-testsuite RV64 asm/objdump Roundtrip Scan Target

Status: Active
Source Idea: ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md

## Purpose

Add an opt-in, large-scale RV64 assembly/object roundtrip scan that feeds
vendored c-testsuite C sources through `c4cll --codegen asm`, `c4c-as`, and
`c4c-objdump`.

Goal: provide a manual or nightly validation route that finds assembler,
disassembler, and object-stability gaps without slowing ordinary full CTest.

## Core Rule

Do not make the scan part of the default full CTest path. Unsupported generated
assembly must be reported with the failing case and stage; it must not be
silently skipped or rewritten.

## Read First

- `ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md`
- `tests/c/external/c-testsuite/`
- `tests/c/external/c-testsuite/allowlist.txt`
- `tests/c/external/c-testsuite/RunCase.cmake`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_rv64_roundtrip_contract.cmake`
- `tests/backend/rv64/rv64i_ev64_roundtrip_contract.md`

## Current Targets

- Create a runner, likely under `tests/backend/cmake/` or `scripts/`, for the
  c-testsuite RV64 asm/objdump roundtrip scan.
- Add an explicit CMake target or option-gated CTest integration that is off by
  default.
- Use stable output directories under the build tree.
- Start with an explicit c-testsuite allowlist and known unsupported-family
  reporting.

## Non-Goals

- Do not require the scan in ordinary
  `ctest --test-dir build -j --output-on-failure`.
- Do not use external `as` or `objdump` as the source of truth.
- Do not execute generated binaries.
- Do not prove broad C semantic correctness.
- Do not weaken existing tests or mark supported paths unsupported to make the
  scan green.

## Working Model

For each selected c-testsuite `.c` file:

```text
case.c
  -> c4cll --codegen asm --target riscv64-linux-gnu -> case.pass0.s
  -> c4c-as case.pass0.s -> case.pass1.o
  -> c4c-objdump case.pass1.o -> case.pass1.s
  -> c4c-as case.pass1.s -> case.pass2.o
  -> c4c-objdump case.pass2.o -> case.pass2.s
  -> c4c-as case.pass2.s -> case.pass3.o
```

The stable comparison must check both:

- `case.pass1.s == case.pass2.s`
- `case.pass2.o == case.pass3.o`

Failure stage names should match the source idea:

- `c4cll-asm`
- `c4c-as-pass1`
- `objdump-pass1`
- `c4c-as-pass2`
- `objdump-pass2`
- `c4c-as-pass3`
- `comparison`

## Execution Rules

- Keep normal test behavior unchanged unless the user explicitly enables the
  scan target or option.
- Prefer structured per-case diagnostics over one large opaque failure.
- Use per-case timeouts.
- Keep initial allowlist and unsupported-family classification explicit in
  repository files, not implicit in runner logic.
- Do not add named-case shortcuts or testcase-shaped matching.
- For code-changing steps, prove with build/configure plus the narrow scan
  target or script command, then use broader CTest only if the integration
  touches shared test infrastructure.

## Ordered Steps

### Step 1: Inspect Existing Test Surfaces

Goal: identify the smallest integration route that reuses existing c-testsuite
and RV64 roundtrip infrastructure.

Primary targets:

- `tests/c/external/c-testsuite/`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_rv64_roundtrip_contract.cmake`
- top-level and test CMake files that own custom targets or disabled tests

Actions:

- Locate existing allowlist patterns, CMake helper conventions, binary output
  directories, and timeout handling.
- Decide whether the first implementation should be a CMake script plus custom
  target, a standalone script plus target wrapper, or an option-gated CTest
  group.
- Record the chosen narrow proof command in `todo.md`.

Completion check:

- `todo.md` names the selected integration surface, owned files for Step 2, and
  the exact proof command the executor should run.

### Step 2: Add The Per-Case Roundtrip Runner

Goal: implement the scan runner for one or more selected c-testsuite inputs.

Primary targets:

- New or existing runner under `tests/backend/cmake/` or `scripts/`
- Build-tree output directory for per-case artifacts

Actions:

- Run `c4cll --codegen asm --target riscv64-linux-gnu`.
- Run the three `c4c-as` stages and two `c4c-objdump` stages.
- Compare text stability and object stability.
- Report the failing case path and exact stage on failure.
- Apply per-case timeout handling.

Completion check:

- The runner can be invoked directly against a small explicit allowlist and
  fails closed with clear diagnostics when any required tool stage fails.

### Step 3: Add Opt-In Build Integration

Goal: expose the runner through an explicit command without changing default
CTest behavior.

Primary targets:

- `tests/backend/CMakeLists.txt`
- possibly `tests/CMakeLists.txt` or a backend-specific helper file

Actions:

- Add a CMake custom target such as
  `rv64_c_testsuite_asm_roundtrip_scan`, or an equivalent disabled-by-default
  CTest group.
- Wire paths to `$<TARGET_FILE:c4cll>`, `$<TARGET_FILE:c4c-as>`, and
  `$<TARGET_FILE:c4c-objdump>`.
- Keep output under the backend build tree.
- Ensure ordinary full CTest does not run the large scan by default.

Completion check:

- `cmake --build build --target <scan-target>` or the documented opt-in CTest
  invocation runs the scan, while default CTest membership is unchanged.

### Step 4: Make The Allowlist And Unsupported Families Explicit

Goal: keep scan scope and current gaps reviewable.

Primary targets:

- A new allowlist or manifest near `tests/c/external/c-testsuite/` or
  `tests/backend/rv64/`
- Optional documentation near the runner or RV64 contract

Actions:

- Select a small initial c-testsuite allowlist.
- Record known unsupported assembly families as diagnostics or manifest data.
- Avoid silently skipping unsupported lines.

Completion check:

- Reviewers can see which c-testsuite cases are in scope and which unsupported
  families are expected to be reported by the scan.

### Step 5: Prove And Document The Opt-In Scan

Goal: validate the scan route and leave a concise command for future manual or
nightly use.

Primary targets:

- Runner integration
- Any nearby README, contract, or CMake comments needed to discover the command

Actions:

- Run a fresh build/configure if required by new CMake wiring.
- Run the opt-in scan command.
- Run default full CTest or a targeted CTest query when needed to prove the scan
  is not always-on.
- Document the command in the narrowest appropriate place.

Completion check:

- The opt-in command is discoverable, the scan checks both text and object
  stability, default CTest remains unchanged, and validation evidence is
  recorded in `todo.md`.

## Acceptance

The source idea is complete only when all of these are true:

- An explicit command or build target runs the large RV64 asm roundtrip scan.
- Ordinary full CTest does not run the large scan by default.
- c-testsuite `.c` inputs flow through `c4cll --codegen asm`, `c4c-as`, and
  `c4c-objdump`.
- The runner checks both canonical text stability and object stability after
  two passes.
- Failures report case file and stage.
- The initial allowlist and known unsupported families are explicit.

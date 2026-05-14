Status: Active
Source Idea Path: ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Enable Public `return_add.c` Smoke

# Current Packet

## Just Finished

Step 5 public smoke wiring completed for `plan.md` Step 5, `Enable Public return_add.c Smoke`.

Wired `tests/backend/case/return_add.c` into the existing AArch64 external assembler/compiler smoke model:
- Added `backend_cli_aarch64_asm_external_return_add_smoke` under the existing `CLANG_EXECUTABLE AND AARCH64_AS_EXECUTABLE` gate.
- Reused `run_backend_aarch64_external_smoke.cmake` with configurable required assembly snippets and expected runtime rc.
- Preserved existing return_zero defaults: `.globl main`, `main:`, `mov w0, #0`, `ret`, expected run rc `0`.
- Added return_add requirements for `.globl main`, `main:`, `mov w0, #2`, `add w0, w0, #3`, and `ret`, with expected run rc `5` when running on an AArch64 host.
- Kept external assemble and clang link proof intact for both public AArch64 asm smokes.

Kept out of scope as required: no edits to compiler/printer implementation, focused C++ tests, `tests/backend/case/return_add.c`, `plan.md`, or the source idea.

## Suggested Next

Delegate the next coherent packet to either run supervisor-selected broader validation for the completed Step 5 slice or ask plan-owner/reviewer whether this runbook step is complete.

## Watchouts

- The public `return_add.c` AArch64 asm smoke is now enabled under the external toolchain gate; keep broader scalar ALU cases deferred unless explicitly delegated.
- Reject named-case matching, fixture assembly text, expectation downgrades, or unsupported marking as progress.
- The external smoke script now defaults to return_zero snippets and rc `0`; future public smokes should pass explicit snippets and expected rc rather than weakening defaults.
- The current scalar printer supports only the immediate subset proved earlier; do not use public smoke wiring to expand scalar ALU capability.

## Proof

Ran the supervisor-delegated Step 5 public-smoke proof exactly:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_cli_aarch64_asm_"' > test_after.log 2>&1
```

Result: passed; 3/3 AArch64 asm CLI tests passed, including `backend_cli_aarch64_asm_external_return_add_smoke`. The public-smoke proof was captured in `test_after.log` before the broader validation below replaced it.

Supervisor-side regression guard compared the matching `^backend_cli_aarch64_asm_` before/after logs before the broader validation below replaced `test_after.log`:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed; before 2/2, after 3/3, no new failing tests.

Supervisor-side broader validation now preserved in `test_after.log`:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_"' > test_after.log 2>&1
```

Result: passed; 24/24 `backend_aarch64_` tests passed.

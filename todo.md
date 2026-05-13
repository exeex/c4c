Status: Active
Source Idea Path: ideas/open/217_c4cll_debug_flags_document_aarch64_asm_output.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm the Landed CLI Contract

# Current Packet

## Just Finished

Step 1 confirmed the landed AArch64 ASM CLI contract before skill edits.

Exact command shape:

```bash
build/c4cll --codegen asm --target aarch64-linux-gnu <input.c> -o <out.s>
```

Target triple handling:

- `--target <triple>` overrides the default host target profile.
- The landed AArch64 tests use `aarch64-linux-gnu`.
- `--codegen asm` selects the backend-native machine-node printer route, not
  LLVM text assembly generation.
- `--backend-bir-stage prepared|semantic` is valid only with `--codegen asm`;
  prepared is the default, while semantic is for bounded observation before
  prepare.

Output-path convention:

- Use `-o <path>` for file output.
- For AArch64 ASM proof, use a `.s` output path.
- If `--codegen asm` is asked to write stdout or a `.s`/`.S` file and the
  backend returns LLVM IR or no asm, the CLI exits with an error instead of
  falling back to LLVM-generated assembly.

External toolchain workflow and helpers:

- CMake registers `backend_cli_aarch64_asm_no_machine_nodes_fails` through
  `tests/backend/cmake/run_backend_codegen_failure_case.cmake`.
- CMake conditionally registers
  `backend_cli_aarch64_asm_external_return_zero_smoke` when both `clang` and
  `aarch64-linux-gnu-as` are found.
- The external smoke helper is
  `tests/backend/cmake/run_backend_aarch64_external_smoke.cmake`.
- The helper runs `c4cll --codegen asm --target "${TARGET_TRIPLE}" "${SRC}" -o
  "${OUT_ASM}"`, checks snippets including `.globl main`, `main:`,
  `mov w0, #0`, and `ret`, assembles with `${AS}`, links with `${CLANG}`, and
  only executes the produced binary when the host processor is `aarch64` or
  `arm64`.

Environment and scope limits:

- AArch64 ASM output is printer output from selected machine nodes.
- `c4cll` does not parse the `.s` text back, encode objects, or link
  executables.
- External object/link proof depends on local `clang` and `aarch64-linux-gnu-as`
  availability; the CTest smoke is skipped at configure time if either is
  missing.

## Suggested Next

Edit `.codex/skills/c4cll-debug-flags/SKILL.md` to document the confirmed
AArch64 ASM command shape and external proof boundary.

## Watchouts

- Do not imply assembly text is parsed back into backend semantics.
- Keep the external proof distinction clear: `c4cll` emits `.s`; the CMake
  helper owns assembler/link/run checks.
- Leave `review/aarch64_allocation_record_step2_review.md` untouched.

## Proof

No build required for this inspection packet.

Ran:

```bash
build/c4cll --help
ctest --test-dir build -N -R 'backend_cli_aarch64_asm'
```

Results:

- `build/c4cll --help` exited 0 and documents
  `--codegen asm --target aarch64-linux-gnu test.c -o out.s`.
- CTest discovery listed `backend_cli_aarch64_asm_no_machine_nodes_fails` and
  `backend_cli_aarch64_asm_external_return_zero_smoke`.
- No `test_after.log` was written because this packet was an inspection-only
  no-build proof and `todo.md` was the only owned file.

# c4cll Debug Flags AArch64 ASM Output Runbook

Status: Active
Source Idea: ideas/open/217_c4cll_debug_flags_document_aarch64_asm_output.md
Activated: 2026-05-13

## Purpose

Document the real `c4cll` AArch64 machine-node assembly output route in
`.codex/skills/c4cll-debug-flags/SKILL.md` now that the AArch64 ASM printer
work has landed.

## Goal

Make the debug-flags skill an accurate operator reference for emitting AArch64
`.s`, selecting the target, writing an output path, and validating the assembly
with an external toolchain.

## Core Rule

Document only command shapes implemented by `src/apps/c4cll.cpp` and validated
by existing post-216 workflows; do not guess flag names or describe an internal
assembler, encoder, linker, ELF writer, or `.s` parser path.

## Read First

- Source idea: `ideas/open/217_c4cll_debug_flags_document_aarch64_asm_output.md`
- CLI surface: `src/apps/c4cll.cpp`
- Skill target: `.codex/skills/c4cll-debug-flags/SKILL.md`
- Relevant post-216 tests or helpers for AArch64 assembly output and external
  smoke validation

## Scope

- Update only the c4cll debug-flags skill unless a documentation-driven CLI
  correction is explicitly required by the supervisor.
- Preserve existing HIR, LIR, BIR, MIR, parser, and semantic-debug guidance.
- Add recipes for producing `.s`, using the target triple, choosing an output
  path, compiling the emitted assembly with external `clang` or `as`, and
  running a smoke executable when the host supports AArch64 execution or
  emulation.

## Non-Goals

- Do not implement or change the AArch64 ASM printer.
- Do not add backend feature tests beyond minimal documentation verification
  if the supervisor asks for it.
- Do not present external toolchain validation as internal assembler or encoder
  support.
- Do not rewrite unrelated frontend or IR debug guidance.

## Working Model

- `--dump-bir`, `--dump-prepared-bir`, and `--dump-mir` are inspection dumps.
- Machine-node `.s` output is final/debug printer output from structured
  machine nodes.
- LLVM-route output is a separate backend path and must remain distinguished
  from the machine-node printer.
- Assembly text is not parsed back into backend semantics by the normal debug
  flow.

## Execution Rules

- Start by confirming the exact CLI contract in `src/apps/c4cll.cpp`.
- Cross-check tests or helper scripts that prove the post-216 command route.
- Keep edits narrow and recipe-oriented.
- Use concrete commands with placeholders for source file, target triple, and
  output path.
- For each documentation change, verify the documented flags exist in the CLI
  and that the recipe matches the test/helper workflow.

## Ordered Steps

### Step 1: Confirm the Landed CLI Contract

Goal: identify the exact AArch64 assembly-output command accepted by `c4cll`.

Primary target: `src/apps/c4cll.cpp`

Actions:

- Inspect the implemented flag or mode for machine-node `.s` output.
- Confirm how the target triple is selected.
- Confirm how output-file paths are specified.
- Locate any post-216 CTest, script, or fixture that exercises external
  assembly smoke validation.

Completion check:

- The executor can name the exact command form to document, including target
  and output-path handling, and has identified the matching proof workflow.

### Step 2: Update the Debug-Flags Skill Recipes

Goal: teach the new AArch64 `.s` route without disturbing existing debug
guidance.

Primary target: `.codex/skills/c4cll-debug-flags/SKILL.md`

Actions:

- Add or update the AArch64 assembly-output recipe with the exact `c4cll`
  command form.
- Include an end-to-end recipe that emits `.s` from `.c`, compiles it with an
  external `clang` or `as`, and runs the smoke executable when the environment
  supports the target.
- Clarify the distinction between BIR dumps, prepared BIR dumps, MIR dumps,
  machine-node `.s` printer output, and LLVM-route output.
- State explicitly that machine-node `.s` output is final/debug printer output
  and not the semantic bridge into an internal assembler or encoder.

Completion check:

- The skill documents the exact implemented command, includes a complete
  external-toolchain smoke recipe, and preserves unrelated debug guidance.

### Step 3: Verify the Documentation Against the Repo

Goal: prove the skill is accurate and did not drift from the implementation.

Primary targets:

- `.codex/skills/c4cll-debug-flags/SKILL.md`
- `src/apps/c4cll.cpp`
- Relevant post-216 AArch64 assembly-output tests or helpers

Actions:

- Check every documented flag against the CLI implementation.
- Run the narrow documentation/proof command chosen by the supervisor.
- If the proof command exercises an external AArch64 toolchain, record host
  limitations clearly when the toolchain or emulator is unavailable.

Completion check:

- Proof results are recorded in `todo.md`, and the documentation can be traced
  directly to the implemented CLI route and existing smoke workflow.

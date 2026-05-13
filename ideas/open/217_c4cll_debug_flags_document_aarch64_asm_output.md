# c4cll Debug Flags Document AArch64 ASM Output

Status: Open
Created: 2026-05-13

Depends On:
- `ideas/closed/216_aarch64_machine_node_asm_printer_external_smoke.md`

## Goal

Update `.codex/skills/c4cll-debug-flags` after the AArch64 machine-node `.s`
printer route lands, so the skill documents the real c4cll flag usage for
emitting AArch64 assembly and validating it with an external toolchain.

## Why This Idea Exists

Idea 216 will add or stabilize a user-facing `c4cll` route in
`src/apps/c4cll.cpp` for printing `.s` from structured AArch64 machine nodes.
The debug-flags skill is the operator-facing reference for choosing frontend
and backend inspection commands. Once the flag exists, the skill must teach the
new route instead of leaving users to infer it from tests or source code.

The skill currently mentions backend-native assembly in broad terms. This idea
is for updating it with the exact post-216 command shape, target requirement,
output-file convention, and smoke-test workflow.

## In Scope

- Update `.codex/skills/c4cll-debug-flags/SKILL.md` with the actual AArch64
  `.s` output flag or mode implemented by idea 216.
- Keep the documented command aligned with `src/apps/c4cll.cpp`; do not invent
  a flag name that the CLI does not accept.
- Add command recipes for:
  - emitting AArch64 `.s` from a `.c` file
  - selecting the target triple
  - writing the assembly to an output path
  - compiling the printed `.s` with external `clang` / `as`
  - running the smoke executable when the environment supports AArch64
    execution or emulation
- Clarify the debugging distinction between:
  - `--dump-bir`
  - `--dump-prepared-bir`
  - `--dump-mir`
  - machine-node `.s` printer output
  - LLVM route output
- Document that the `.s` printer is an output consumer of structured machine
  nodes, not an internal assembler/encoder input path.
- If 216 chooses a backend-specific CTest helper or directory for smoke tests,
  reference that workflow in the skill at recipe level.

## Out Of Scope

- Implementing or changing the AArch64 `.s` printer.
- Changing `src/apps/c4cll.cpp` beyond documentation-driven corrections if the
  flag is already implemented by 216.
- Adding new backend tests beyond minimal skill-doc verification if needed.
- Documenting an internal assembler, encoder, ELF writer, linker, or `.s`
  parser as part of the normal debug path.
- Rewriting unrelated frontend parser, HIR, LIR, or BIR flag documentation.

## Acceptance Criteria

- `.codex/skills/c4cll-debug-flags/SKILL.md` documents the exact AArch64
  assembly-output command accepted by `c4cll`.
- The skill includes at least one end-to-end recipe that emits `.s` from `.c`
  and compiles it with an external toolchain, matching the validation route
  established by idea 216.
- The skill clearly says `.s` output is final/debug printer output and not the
  semantic bridge into an internal assembler or encoder.
- Existing HIR/LIR/BIR/MIR debug guidance remains intact except where it must
  be clarified to place the new `.s` route.

## Reviewer Reject Signals

- The skill documents a guessed flag name or command that `src/apps/c4cll.cpp`
  does not implement.
- The documentation implies assembly text should be parsed back into backend
  semantics.
- The update presents external `clang` validation as equivalent to internal
  assembler/encoder support.
- Existing LIR/HIR/BIR debugging guidance is removed or broadly rewritten
  without need.

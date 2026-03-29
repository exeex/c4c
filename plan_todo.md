# Plan Todo

Status: Active
Source Idea: ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Repair the active plan scope after Step 1 showed that
      `tests/c/internal/backend_case/call_helper.c` does not currently emit a
      relocation-bearing x86 object contract.

## Ordered Checklist

- [x] Step 1: Capture the bounded reference contract.
- [ ] Step 2: Tighten focused validation.
- [ ] Step 3: Implement the minimal assembler slice.
- [ ] Step 4: Prove end-to-end behavior and guard regressions.

## Next Intended Slice

- Preserve the Step 1 discovery in the active source idea and open a separate
  follow-on idea for a true x86 external-call relocation path.
- Decide whether to correct the active plan toward the real `call_helper.c`
  local-call object contract or switch to the new external-call initiative.
- Avoid implementing relocation-bearing x86 assembler work against the current
  `call_helper.c` fixture until the lifecycle state is corrected.

## Blockers

- `tests/c/internal/backend_case/call_helper.c` currently emits:
  `.type helper, %function`, a local `helper:` definition, and `call helper`.
  External `clang --target=x86_64-unknown-linux-gnu -c` resolves that local
  call directly and produces no `.rela.text` entry, so the active plan's
  expected relocation-bearing contract does not match the current backend-owned
  asm path.

## Resume Notes

- This plan was activated from a staged child idea because the umbrella roadmap
  in `ideas/open/__backend_port_plan.md` explicitly said not to activate it
  directly.
- Keep the implementation bounded to one relocation-bearing helper-call object
  path. Split again if broader relocation or linker work becomes necessary.
- Step 1 evidence on 2026-03-29:
  `./build/c4cll --codegen lir --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c`
  emits a local `helper` definition plus `call helper`, and external assembly
  of that text produces a 12-byte `.text` section with symbols `helper` and
  `main` but no relocation records.
- The previously staged x86 linker fixture still proves the external-call
  relocation shape separately: `R_X86_64_PLT32` at offset `1` with addend `-4`
  targeting `helper_ext`.

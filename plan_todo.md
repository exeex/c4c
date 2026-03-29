# AMD64 Linux Recovery TODO

Status: Active
Source Idea: ideas/open/x86_64_linux_failures_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Checklist
- [ ] 1. Baseline & Instrumentation (ctest subset + artifact capture)
- [ ] 2. SysV Varargs classification & homes
- [ ] 3. Variadic function pointer calls
- [ ] 4. Inline diagnostics shim stability
- [ ] 5. Inline asm `yield` alias
- [ ] 6. Glibc macro parsing support
- [ ] 7. Validation & regression guard

## Active Slice
- Target: Step 1 Baseline & Instrumentation
- Narrow scope: rerun failing subset, capture `c4cll` vs clang artifacts for each failure family, and stage summaries under `ref/amd64_varargs/`.
- Owner notes: need fresh `test_before.log` to ensure counts still match 75 failures.

## Next Slice
- Step 2 SysV varargs classification once instrumentation artifacts confirm register layout gaps.

## Blockers
- None yet; awaiting new baseline artifacts.

## Handoff Notes
- Create `ref/amd64_varargs/README.md` explaining captured files when Step 1 completes.
- Keep baseline logs tied to test dates; record host triple `x86_64-linux-gnu`.

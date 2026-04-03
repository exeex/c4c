Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback

Current active item: Step 2, first parity slice.

Completed this iteration:
- Audited the current production legacy boundaries in `backend.cpp`,
  `backend.hpp`, `llvm_codegen.cpp`, and `c4cll.cpp`.
- Recorded the concrete removal checklist and test owners in
  `ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md`.
- Captured a clean full-suite baseline in `test_before.log` before edits.

Next intended slice:
- Replace the public `BackendModule(ir.*)` contract at the backend header and
  emitter-header boundary with a BIR-owned contract shape, starting with the
  type-system and module-surface gaps needed to remove `../../ir.hpp` from
  `src/backend/x86/codegen/emit.hpp` and `src/backend/aarch64/codegen/emit.hpp`.

Resume notes:
- `backend.cpp` still contains both the legacy route (`emit_legacy_module`) and
  the BIR-to-legacy reconversion route (`lower_bir_to_backend_module`).
- `c4cll.cpp` still rescues `--codegen asm` through LLVM IR/asm conversion and
  a second retry via `CodegenPath::Llvm`.

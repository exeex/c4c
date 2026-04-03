Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback

Current active item: Step 2, next parity slice after the public header/input boundary split.

Completed this iteration:
- Audited the current production legacy boundaries in `backend.cpp`,
  `backend.hpp`, `llvm_codegen.cpp`, and `c4cll.cpp`.
- Recorded the concrete removal checklist and test owners in
  `ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md`.
- Rebuilt and revalidated the full suite after this slice, recording the
  green run in `test_after.log` (`2728/2728` passing).
- Replaced the inline `BackendModuleInput` ownership model with an out-of-line
  wrapper that can own either legacy `BackendModule` or BIR `bir::Module`
  inputs without forcing `backend.hpp` to include `ir.hpp`.
- Removed `../../ir.hpp` from `src/backend/x86/codegen/emit.hpp` and
  `src/backend/aarch64/codegen/emit.hpp` by forward-declaring
  `BackendModule` at the emitter-header boundary.
- Added a direct-BIR backend pipeline test plus a compile-time header-boundary
  regression.

Next intended slice:
- Start replacing the remaining public legacy `BackendModule(ir.*)` emitter
  contract with BIR-owned seams inside the backend implementation, beginning
  with the small emitter-facing module/type features still forcing
  `lower_bir_to_backend_module(...)` in `backend.cpp`.

Resume notes:
- `backend.cpp` still contains both the legacy route (`emit_legacy_module`) and
  the BIR-to-legacy reconversion route (`lower_bir_to_backend_module`).
- `backend.hpp` now exposes a BIR-capable input seam, but x86/aarch64 emitters
  still consume `BackendModule` internally, so the next step is to move the
  BIR ownership boundary deeper than the public headers.
- `c4cll.cpp` still rescues `--codegen asm` through LLVM IR/asm conversion and
  a second retry via `CodegenPath::Llvm`.

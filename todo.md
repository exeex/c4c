Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback

Current active item: Step 2, move the direct-BIR backend conversion seam from `backend.cpp`
into the x86/aarch64 emitter layer for the currently supported bounded slices.

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
- Moved the direct-BIR conversion seam out of `backend.cpp` and into the
  x86/aarch64 emitter layer by adding direct `bir::Module` emitter entry
  points and routing backend BIR execution through them.
- Added end-to-end x86/aarch64 regression coverage proving direct
  `BackendModuleInput{bir::Module}` callers still reach backend assembly
  emission without pre-lowering to `BackendModule`.

Next intended slice:
- Start replacing the remaining public legacy `BackendModule(ir.*)` emitter
  contract with BIR-owned seams inside the backend implementation, beginning
  with direct emitter-native handling for the bounded BIR arithmetic slices
  that still lower through `lower_bir_to_backend_module(...)` inside
  `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp`.

Resume notes:
- `backend.cpp` still contains the legacy route (`emit_legacy_module`), but
  direct BIR backend execution now dispatches into emitter-owned BIR entry
  points instead of pre-lowering in the backend facade.
- `backend.hpp` now exposes a BIR-capable input seam, and x86/aarch64 emitters
  now own the remaining `bir::Module` -> `BackendModule` lowering internally,
  so the next step is to replace those emitter-local lowering calls with
  native BIR handling for supported slices.
- `c4cll.cpp` still rescues `--codegen asm` through LLVM IR/asm conversion and
  a second retry via `CodegenPath::Llvm`.

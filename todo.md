Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback

Current active item: Step 2, continue shrinking legacy backend-IR ownership now
that the x86/AArch64 direct-`bir::Module` fallback through
`bir_to_backend_ir.*` is gone.

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
- Replaced the x86/aarch64 direct `bir::Module` arithmetic-entry fallback with
  native emitter-side handling for return-immediate, return-add/sub-immediate,
  and bounded affine return slices, while keeping `lower_bir_to_backend_module`
  only as the unsupported direct-BIR fallback.
- Added direct-BIR staged-affine regression coverage for both x86 and AArch64
  emitter entry points.
- Extended direct `bir::Module` affine parsing in the x86 and AArch64 emitters
  to accept zero-parameter staged constant chains, so multi-instruction direct
  BIR constant-return slices now emit natively instead of falling back through
  `bir_to_backend_ir.*`.
- Added BIR text-surface coverage plus direct-BIR x86/AArch64 end-to-end
  regressions for the zero-parameter staged constant chain slice.
- Revalidated the full suite after the zero-parameter staged-constant direct
  BIR slice with `test_after.log` (`2728/2728` passing) and confirmed
  monotonic no-regression behavior against `test_before.log`.
- Removed the x86/AArch64 direct-`bir::Module` rescue path through
  `bir_to_backend_ir.*`; unsupported manual direct-BIR input now fails
  explicitly instead of silently converting back into legacy backend IR.
- Deleted `src/backend/lowering/bir_to_backend_ir.cpp` and
  `src/backend/lowering/bir_to_backend_ir.hpp`, removed their build wiring, and
  switched the remaining bridge-style backend tests to
  `lower_lir_to_backend_module(...)`.
- Added direct-BIR regression coverage proving unsupported multi-function BIR
  modules now throw explicit x86/AArch64 backend errors rather than re-entering
  the legacy backend-IR route.
- Rebuilt from a clean `build/` directory and reran the full suite into
  `test_fail_after.log`; the regression guard passed against
  `test_fail_before.log` with `after: 2728 passed, 0 failed` and no newly
  failing tests.

Next intended slice:
- Audit whether Step 3 can now start by moving more emitter-owned boundaries off
  `BackendModule(ir.*)` in favor of direct BIR data structures, or whether the
  next smallest safe slice is removing another legacy route from
  `src/backend/backend.cpp`.

Resume notes:
- `backend.cpp` still contains the legacy route (`emit_legacy_module`), but
  direct BIR backend execution now dispatches into emitter-owned BIR entry
  points instead of pre-lowering in the backend facade.
- `backend.hpp` still exposes both legacy `BackendModule` and BIR-capable input
  seams, but x86/aarch64 direct-BIR entry points now have no fallback back into
  legacy backend IR; only the native affine-return subset is accepted there.
- `c4cll.cpp` still rescues `--codegen asm` through LLVM IR/asm conversion and
  a second retry via `CodegenPath::Llvm`.

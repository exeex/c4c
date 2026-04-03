Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback

Current active item: Step 2, widen the bounded BIR straight-line arithmetic
scaffold with constant-only unsigned remainder (`urem`) on the BIR
text/lowering path, while keeping any new default-route exposure limited to
RISC-V and not claiming new x86/AArch64 direct-emitter coverage yet.

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
- Expanded the BIR scaffold with `i8` scalar support in `bir.hpp`,
  `bir.cpp`, and `lir_to_bir.cpp`, including typed immediates, printer output,
  validator acceptance, and bounded `i8` add/sub lowering coverage.
- Added targeted BIR lowering and pipeline regressions proving the widened
  `i8` slice reaches the RISC-V BIR text surface without re-entering the
  legacy backend-IR route.
- Widened `lir_to_bir.cpp` to accept `i64` scalar signatures and arithmetic
  text operands in addition to the existing `i8`/`i32` surface, and updated
  the scaffold failure message to reflect the new bounded type coverage.
- Added `i64` BIR printer, validator, lowering, and RISC-V pipeline
  regressions so the widened word-sized arithmetic slice is covered on the BIR
  text path without implying new x86/AArch64 direct-emitter support yet.
- Rebuilt the tree, reran focused backend BIR validation, and refreshed
  `test_fail_after.log`; the regression guard passed against
  `test_fail_before.log` with `after: 2728 passed, 0 failed` and no newly
  failing tests.
- Widened the BIR straight-line arithmetic scaffold with bounded `mul`
  coverage by adding `bir.mul` printer/lowering support plus targeted
  regression helpers and RISC-V BIR pipeline tests for the constant-only slice.
- Tightened `src/codegen/llvm/llvm_codegen.cpp` so automatic BIR-pipeline
  selection remains RISC-V-only; x86/AArch64 backend codegen no longer picks up
  newly lowerable BIR slices unless the caller explicitly requests the BIR
  pipeline.
- Rebuilt from a clean `build/` directory, reran targeted backend validation,
  refreshed `test_fail_after.log`, and passed the regression guard against
  `test_fail_before.log` with `--allow-non-decreasing-passed`
  (`2728/2728` before and after, no newly failing tests).
- Widened the BIR straight-line arithmetic scaffold with bounded constant-only
  signed division by adding `bir.sdiv` printer/lowering support, backend test
  fixtures, and targeted BIR text-surface regressions for the explicit
  pipeline plus the RISC-V default-route case.
- Rebuilt from a clean `build/` directory, reran `backend_bir_tests` plus the
  new `backend_codegen_route_riscv64_return_sdiv_defaults_to_bir` coverage,
  refreshed `test_fail_after.log`, and passed the regression guard against
  `test_fail_before.log` with `--allow-non-decreasing-passed`
  (`2728 -> 2729` passed, `0 -> 0` failed, no newly failing tests).
- Widened the BIR straight-line arithmetic scaffold with bounded constant-only
  signed remainder by adding `bir.srem` printer/lowering support, backend test
  fixtures, explicit BIR pipeline coverage, and a new RISC-V default-route
  regression for `return 14 % 5;`.
- Rebuilt the tree and reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_srem_defaults_to_bir` coverage to keep
  the `srem` slice bounded to the intended BIR text surfaces before the full
  regression pass.
- Refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run and passed the regression guard against
  `test_fail_before.log` with `--allow-non-decreasing-passed`
  (`2728 -> 2730` passed, `0 -> 0` failed, no newly failing tests).
- Widened the BIR straight-line arithmetic scaffold with bounded constant-only
  unsigned remainder by adding `bir.urem` printer/lowering support, backend
  test fixtures, explicit BIR pipeline coverage, and a new RISC-V
  default-route regression for `return 14u % 5u;`.
- Rebuilt the tree, reran `backend_bir_tests` plus the new
  `backend_codegen_route_riscv64_return_urem_defaults_to_bir` coverage, then
  refreshed `test_fail_after.log` with a full `ctest --test-dir build -j
  --output-on-failure` run.
- Passed the regression guard against `test_fail_before.log` with
  `--allow-non-decreasing-passed` (`2728 -> 2731` passed, `0 -> 0` failed, no
  newly failing tests).

Next intended slice:
- Continue Phase 1/2 parity by widening BIR into the next straight-line
  comparison gap after `urem`, likely a minimal integer compare-and-branch or
  compare-and-select slice, while keeping default BIR auto-routing gated to
  RISC-V until x86/AArch64 direct-BIR emitters grow native support for it.

Resume notes:
- `backend.cpp` still contains the legacy route (`emit_legacy_module`), but
  direct BIR backend execution now dispatches into emitter-owned BIR entry
  points instead of pre-lowering in the backend facade.
- `backend.hpp` still exposes both legacy `BackendModule` and BIR-capable input
  seams, but x86/aarch64 direct-BIR entry points now have no fallback back into
  legacy backend IR; only the native affine-return subset is accepted there.
- `lir_to_bir.cpp` now accepts bounded straight-line `i8` arithmetic slices in
  addition to the existing `i32` and `i64` scaffolds, but backend asm emitters
  still only consume the narrower direct-BIR affine subset.
- The next bounded gap is instruction coverage rather than another scalar type:
  the BIR scaffold still rejects straight-line integer arithmetic outside
  `add/sub/mul/sdiv/srem/urem`, plus the upcoming compare/select/control-flow
  clusters, even when the slice can stay entirely on the BIR text path.
- Auto-selection into the BIR pipeline in `llvm_codegen.cpp` is intentionally
  constrained to `Target::Riscv64`; explicit BIR pipeline options are still the
  only supported way to exercise non-RISC-V direct-BIR emitter slices.
- `c4cll.cpp` still rescues `--codegen asm` through LLVM IR/asm conversion and
  a second retry via `CodegenPath::Llvm`.

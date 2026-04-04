Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 2, land the bounded native-emitter coverage slice
for the new straight-line BIR cast family (`sext`/`zext`/`trunc`) on x86_64
and aarch64.
Completed in this slice: added a structured backend route-selection seam in
`backend.hpp/.cpp` and covered legacy-LIR, BIR-LIR, direct-BIR, and pre-lowered
legacy inputs without relying on `riscv64` passthrough text.
Completed in this slice: extended target-neutral BIR pipeline coverage so the
existing widened `i8` two-parameter add lowering is now exercised through the
explicit BIR route surface instead of living only in lowering-level tests.
Completed in this slice: batch-added widened `i8` compare-return route
coverage (`eq/ne/ult/ule/ugt/uge/slt/sle/sgt/sge`) so this whole lowering
cluster is now exercised at the explicit BIR pipeline seam instead of only in
lowering-only tests.
Completed in this slice: batch-added widened `i8` arithmetic/bitwise/immediate
route coverage (`add/sub/mul/and/or/xor/shl/lshr/ashr/sdiv/udiv/srem/urem`
plus immediate return), so the remaining zero-parameter widened `i8` scaffold
family now has explicit BIR pipeline coverage.
Completed in this slice: refreshed the Step 1 removal-map snapshot in
`ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md` so it matches the
current mixed BIR/legacy contract, including the cleaned header boundary, the
remaining production legacy routes, the app-layer LLVM rescue hooks, and the
still-legacy test buckets.
Completed in this slice: added explicit BIR cast support (`sext`/`zext`/`trunc`)
to the straight-line scaffold, including printer/validator coverage plus
target-neutral lowering and explicit BIR-pipeline tests for parameter-fed cast
shapes that do not collapse into the older compare/immediate folds.
Completed in this slice: extended the x86_64 and aarch64 BIR-pipeline tests to
cover straight-line `sext`/`zext`/`trunc` fixtures, and taught both direct-BIR
native emitters to lower that bounded cast-return family instead of rejecting
it as unsupported.
Completed in this slice: repaired the stale backend baseline around the renamed
`widen_signed` cast fixture, so the backend regression gate improved from
`99%` (`1/394` failing) to `100%` (`0/394` failing).
Next target: pick the next bounded BIR-owned lowering family after the
straight-line casts, start with target-neutral route/lowering assertions, and
only add x86_64/aarch64 emitter checks once that family’s direct-BIR/native
ownership boundary is clear.

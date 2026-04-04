Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 2, continue the bounded native-emitter coverage
expansion for direct-BIR x86_64/aarch64 widened `i8`
compare-fed `bir.select`/join families.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the existing `choose2_add_post_chain_tail_ne_u`
split-predecessor join family, proving that both native emitters already
preserve the bounded post-select `add`/`sub`/`add` arithmetic tail without
falling back to legacy backend IR.
Completed in this slice: reran the required regression checks with monotonic
full-suite results (`100% tests passed, 0 tests failed out of 2833` in both
`test_before.log` and `test_after.log`), so the added native-emitter coverage
did not introduce new failures.
Completed in this slice: taught the direct-BIR x86_64 and aarch64 emitter
entrypoints to accept the bounded widened `i8` compare-fed `bir.select`
family with predecessor-local add chains and a short post-select arithmetic
tail, without routing through legacy backend IR adaptation.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the narrow `choose2_add_post_ne_u` fixture so the new
native path is exercised at the backend seam instead of only through the
legacy-backend-module surface.
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
Completed in this slice: taught both direct-BIR native emitters to accept the
bounded constant compare-fed `bir.select` return shape by routing it through
their existing minimal conditional-return asm path, and added explicit direct-
BIR x86_64/aarch64 pipeline coverage for that family.
Completed in this slice: generalized the direct-BIR x86_64/aarch64
conditional-select path so the native emitters now accept the bounded
parameter-fed `bir.select` family as direct input, including one-parameter
immediate-arm and two-parameter value-select shapes, with explicit pipeline
coverage for both targets.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the existing `choose2_add_post_chain_ne_u` split-
predecessor join family, proving that both native emitters already preserve the
bounded post-select `add`/`sub` arithmetic tail without falling back to legacy
backend IR.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8` `choose2_mixed_post_ne_u`
split-predecessor mixed-affine join family, proving that both native emitters
already preserve the bounded predecessor-local `add`/`sub` affine chains plus
the short post-select `add` tail without falling back to legacy backend IR.
Completed in this slice: added explicit direct-BIR x86_64 and aarch64
pipeline coverage for the widened `i8` `choose2_mixed_post_chain_ne_u`
split-predecessor mixed-affine join family, proving that both native emitters
already preserve the bounded predecessor-local `add`/`sub` affine chains plus
the short post-select `add`/`sub` tail without falling back to legacy backend
IR.
Completed in this slice: reran the required regression checks with monotonic
full-suite results (`100% tests passed, 0 tests failed out of 2833` in both
`test_before.log` and `test_after.log`), so the added native-emitter coverage
did not introduce new failures.
Next target: move to the bounded widened `i8`
`choose2_mixed_post_chain_tail_u` split-predecessor mixed-affine join family
and add matching direct-BIR x86_64/aarch64 pipeline coverage for the
post-select `add`/`sub`/`add` arithmetic tail if the native emitters already
handle that slice.

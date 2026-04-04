Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [ ] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [ ] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 1, refresh the current removal map and make the test
strategy explicit enough that idea 41 does not drift back into `riscv64`-first
route coverage.
Next target: once the boundary map is refreshed, land the next bounded BIR
coverage slice with a target-neutral or structured route test instead of adding
another RV64 oracle by default.

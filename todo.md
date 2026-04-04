Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [ ] Refresh the concrete inventory of production legacy IR boundaries and legacy-coupled test surfaces
- [x] Identify route/fallback seams that should stop depending on `riscv64` passthrough text as the main oracle
- [ ] Continue bounded `lir_to_bir` coverage expansion with target-neutral tests first
- [ ] Migrate x86/aarch64 emitter-facing contracts away from `ir.*`
- [ ] Remove legacy backend IR files and backend/app LLVM rescue paths
- [ ] Delete transitional legacy test buckets once their coverage is migrated or no longer needed

Current active item: Step 1, finish the concrete removal-map inventory now that
the entry route seam has a structured observation helper
(`select_lowering_route(...)`) and target-neutral regression coverage.
Completed in this slice: added a structured backend route-selection seam in
`backend.hpp/.cpp` and covered legacy-LIR, BIR-LIR, direct-BIR, and pre-lowered
legacy inputs without relying on `riscv64` passthrough text.
Next target: refresh the remaining production/test boundary inventory, then
land the next bounded `lir_to_bir` slice with one target-neutral route/lowering
test before any target-specific emitter assertions.

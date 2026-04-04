Status: Active
Source Idea: ideas/open/40_target_profile_and_execution_domain_foundation.md
Source Plan: plan.md

- [ ] Inventory current target-triple/data-layout ownership across CLI, frontend, HIR, LIR, and backend
- [ ] Introduce a shared `TargetProfile` and canonical resolver
- [ ] Thread `TargetProfile` through active compile/codegen boundaries without changing host-only output
- [ ] Add declaration-level `ExecutionDomain` metadata with default `Host`
- [ ] Refactor `HIR -> LIR` ownership so future multi-target splitting has a clear seam
- [ ] Centralize test target/layout fixtures and reduce accidental RISC-V-only architectural coupling

Current active item: Step 1, inventory the exact ownership chain for target
triple, data layout, and target-sensitive trait queries; use that inventory to
define the first `TargetProfile` insertion point.
Next target: once the ownership map is concrete, land the minimal shared
`TargetProfile` type plus one canonical resolver path from CLI target selection
into frontend/codegen.

Context carried forward from the parked idea 41 plan:
- Step 2 BIR work under idea 41 was making real progress, but recent discussion
  exposed that route coverage is converging too much onto the RISC-V passthrough
  harness because it is the easiest direct-BIR-vs-LLVM oracle.
- The core issue is architectural, not just testing style: the pipeline still
  threads a raw `target_triple`, derives `data_layout` late, re-parses target
  identity in multiple layers, and has no first-class execution-domain model.
- We should resume idea 41 after this foundation lands so backend-IR migration
  is built on shared target-profile ownership rather than on scattered target
  strings and route-specific test assumptions.

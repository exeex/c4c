Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [x] Lock x86 legacy `BackendModule` emitter entry
- [x] Lock aarch64 legacy `BackendModule` emitter entry
- [x] Remove shared backend `BackendModule` public emission entry
- [x] Remove legacy backend-IR-centric test targets from active CMake / ctest
- [ ] Coordinate A/B/C/D parallel cleanup lanes
- [ ] Reintegrate worker commits without overlap regressions
- [ ] Run broad post-merge backend validation

Current active item:
- maintain the mainline integration lane while Groups A/B/C/D work from
  [`todoA.md`](/workspaces/c4c/todoA.md),
  [`todoB.md`](/workspaces/c4c/todoB.md),
  [`todoC.md`](/workspaces/c4c/todoC.md),
  and
  [`todoD.md`](/workspaces/c4c/todoD.md)

Mainline rules:
- mainline owns planning files and cross-group conflict resolution
- worker lanes should not edit other groups' todo files
- worker lanes should compile only their own representative `.o` target before handoff
- only mainline performs the final broad build/test pass after reintegration

Next integration steps:
- land Group A x86 emitter cleanup
- land Group B aarch64 emitter cleanup
- land Group C shared lowering/legacy-IR cleanup
- land Group D parked legacy test retirement/migration
- update this file after each merge

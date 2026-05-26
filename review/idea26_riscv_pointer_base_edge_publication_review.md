# Idea 26 RISC-V Pointer-Base Edge Publication Review

## Review Scope

- Active source idea: `ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md`
- Active plan: `plan.md`
- Current todo state: `todo.md`
- Implementation commit reviewed: `1d1ae2e64 Support RISC-V pointer-base edge publications`
- Report path: `review/idea26_riscv_pointer_base_edge_publication_review.md`

## Review Base

Chosen base commit: `218a67a78 [plan] Activate RISC-V pointer-base edge publication plan`.

Reason: this is the lifecycle activation checkpoint for the current source idea and active runbook. The source idea itself was created earlier, but `218a67a78` is the commit that made idea 26 the active plan/todo state. The later `d7a4f612f` commit is inventory-only and does not reset the active source idea.

Commit count since base: 2.

## Findings

No blocking findings.

## Evidence

- Source-idea alignment: the idea requires RISC-V `PointerBasePlusOffset -> Register` consumption through shared `edge_publications`, target-local address materialization, no local rediscovery, and fail-closed unsupported destinations (`ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md:18`, `ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md:20`, `ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md:29`, `ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md:37`). The plan transcribes the same constraints and explicitly keeps stack destinations and broader stack-source work out of scope (`plan.md:32`, `plan.md:41`, `plan.md:127`).
- Implementation route: `consume_edge_publication_move_intent` still obtains the move only from `find_unique_indexed_prepared_edge_publication` before rendering source/destination operands (`src/backend/mir/riscv/codegen/emit.cpp:123`). Pointer-base rendering resolves the named base through prepared value-home lookups, accepts only register bases, requires a signed-12-bit delta, and records the resulting provenance (`src/backend/mir/riscv/codegen/emit.cpp:83`). Emission is target-local RISC-V `addi` for nonzero deltas and `mv` for zero deltas (`src/backend/mir/riscv/codegen/emit.cpp:171`).
- Unsupported homes remain fail closed: non-register bases, missing base names, missing deltas, out-of-range deltas, and stack-slot destinations return unsupported statuses rather than expanding scope (`src/backend/mir/riscv/codegen/emit.cpp:90`, `src/backend/mir/riscv/codegen/emit.cpp:93`, `src/backend/mir/riscv/codegen/emit.cpp:156`; `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:344`).
- Overfit/local-rediscovery check: the implementation does not scan predecessor/successor blocks or match fixture labels/value ids for semantic behavior. The concrete fixture values live in tests, while the production rule is home-kind plus prepared lookup plus RISC-V immediate-range policy.
- Test coverage: positive coverage proves shared lookup-backed `addi a1, s2, 12`, helper provenance, and zero-delta `mv`; clearing `publications_by_edge_destination` proves no local rediscovery (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:273`, `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:323`). Negative coverage exercises missing base name, missing delta, unresolved base, out-of-range deltas, non-register base, stack destination, missing shared lookups, missing publication, and non-move publication (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:344`, `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:447`).
- Validation evidence: `todo.md` records focused proof passing 5/5, a matching focused regression guard passing 5/5 before and after, and broader backend validation passing 163/163. `test_after.log` currently contains the broader backend run with `100% tests passed, 0 tests failed out of 163`.

## Judgments

- Idea alignment: matches source idea.
- Runbook transcription: plan matches idea.
- Route alignment: on track.
- Technical debt: acceptable.
- Validation sufficiency: narrow proof plus backend bucket sufficient for Step 6 handoff/closure.
- Overfit risk: low; no testcase-overfit or local rediscovery found.

## Recommendation

Continue current route. The slice is ready to proceed to Step 6 handoff/closure, provided the handoff stays scoped to register-base, signed-12-bit `PointerBasePlusOffset -> Register` support and preserves the existing caveats for source-to-`StackSlot` destinations, stack-source policy broadening, non-register bases, and deltas requiring wider materialization.

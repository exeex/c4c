# Step 5 x86 Handoff Dirty Slice Review

## Review Base

- Base commit: `b079b8cc [plan] Activate BIR raw label fallback cleanup plan`
- Rationale: this is the commit that activated the current `plan.md` for `ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md`; later lifecycle commits since that point are Step 1-5 todo/progress commits, not a replacement activation or reviewer checkpoint.
- Commits since base: 8
- Reviewed scope: dirty diff for `src/backend/mir/x86/module/module.cpp`, `tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp`, `src/backend/mir/x86/codegen/*`, and `todo.md`, checked against active Step 5 and source idea 120.

## Findings

### High: Dirty `module.cpp` is testcase-shaped x86 renderer recovery, not Step 5 id-consumption proof

`src/backend/mir/x86/module/module.cpp:598` introduces `render_supported_function()` as a large local dispatcher over very narrow BIR shapes, while `emit()` now routes every defined function through it before falling back to the existing stub path at `src/backend/mir/x86/module/module.cpp:778`.

The implementation recognizes instruction counts, operand positions, single-param/single-block forms, and same-module global smoke shapes (`src/backend/mir/x86/module/module.cpp:623`, `src/backend/mir/x86/module/module.cpp:644`, `src/backend/mir/x86/module/module.cpp:686`, `src/backend/mir/x86/module/module.cpp:703`). It emits handwritten asm strings directly from those shapes. It does not consume prepared control-flow metadata, prepared block label ids, branch label ids, or `BlockLabelId` authority for the Step 5 compare/branch handoff boundary.

This is route drift from idea 120. The source idea is about removing or narrowing raw BIR label fallback dependencies after assembler/backend consumers prove structured `BlockLabelId` authority. Rebuilding scalar x86 module emission by shape recognition is a target-codegen capability route, not raw-label fallback cleanup. It may be useful infrastructure, but it belongs in a separate initiative or an explicitly narrowed plan before more execution.

### High: Current proof is still red and does not establish the boundary Step 5 requires

`test_after.log` shows the delegated command compiles and links, but `backend_x86_handoff_boundary` still fails at:

`scalar-control-flow compare-against-zero branch lane: x86 prepared-module consumer did not emit the canonical asm`

The first failing assertion is in `tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp:524`. The later tests that would verify prepared-control-flow ownership and rejection of raw branch recovery (`tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp:1309`, `tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp:1318`, `tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp:1326`) are not reached. The dirty slice therefore has not restored/proven assembler/backend id-consumption boundaries for Step 5.

Passing earlier scalar lanes after adding `module.cpp` support would be insufficient anyway because those lanes are not label/branch authority checks. They only show that the x86 prepared-module consumer can render a few non-control-flow scalar shapes before reaching the branch family.

### Medium: The test mutation uses `const_cast` across a const lookup surface

`tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp:832` casts away constness from `find_prepared_parallel_copy_bundle()` so the test can relocate a bundle. That keeps the test compiling, but it weakens the API boundary that Step 5 is supposed to prove. If this mutation remains needed, the route should add an explicit mutable prepared-control-flow test helper or construct the prepared fixture through an owned mutable path rather than normalizing const removal in the handoff proof.

### Medium: `todo.md` records dirty, failing implementation progress as "Just Finished"

`todo.md:11` through `todo.md:35` describes the dirty x86 renderer work as completed progress, then `todo.md:74` through `todo.md:77` records the proof as blocked. It is good that the blocker is documented, but the accepted-progress wording is too strong for a slice that is both dirty and not Step 5-aligned. If this route is rejected or split, the lifecycle owner should rewrite `todo.md` to distinguish accepted Step 5 facts from the unaccepted x86 renderer attempt.

## Judgments

- Plan-alignment judgment: `route reset needed`
- Idea-alignment judgment: `drifting from source idea`
- Technical-debt judgment: `action needed`
- Validation sufficiency: `needs broader proof`

## Recommendation

`rewrite plan/todo before more execution`

Do not accept or commit the dirty `module.cpp` renderer slice as Step 5 progress. It is not a legitimate proof of assembler/backend id-consumption boundaries, and it has strong testcase-overfit/route-drift risk because it advances the optional x86 handoff test by adding shape-specific scalar asm rendering unrelated to label identity.

Recommended next action:

- Hand this report to the plan owner.
- Split the x86 prepared-module renderer recovery into a separate source idea or explicit plan subroute if the project wants to rebuild that capability.
- Keep Step 5 focused on a bounded proof that the existing assembler/backend handoff consumes prepared control-flow label authority and rejects missing/drifted ids, without adding broader handwritten BIR-shape dispatch as the means of reaching the assertion.

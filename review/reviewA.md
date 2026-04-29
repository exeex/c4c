# Review A: Idea 132 Reopened LIR Signature Type-Ref Repair

## Scope

Requested review of the current implementation slice from the repaired active-plan point to `HEAD`, focusing on commit `2c722228 Own LIR function signature type tags`.

Chosen base commit: `b5c50f53 [plan+idea] Reopen parser rendered record template cleanup for rebuilt LIR regression`

Why this base is correct: it is the lifecycle checkpoint that reopened idea 132, created the active repair `plan.md`, and linked the plan/todo state back to `ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md`. The later `f79589e1` commit is todo metadata only, not the repaired active-plan point.

Commit count since base: 2

Reviewed diff: `git diff b5c50f53..HEAD`

## Findings

No blocking findings.

## Route Drift / Overfit Check

The implementation stays aligned with the reopened repair runbook. The target failure was a mismatch between the printed function signature and structured LIR signature return metadata. The fix changes the ownership and authority path in LIR lowering rather than matching `declared_pair`, weakening `frontend_lir_function_signature_type_ref`, or relaxing the verifier.

Evidence:

- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:37` adds `lir_owned_type_spec`, which copies `TypeSpec::tag` strings into LIR-owned storage before they are retained in `LirFunction.return_type`, `LirFunction.params`, and `LirFunction.signature_params`.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:43` changes aggregate type-ref construction so the structured `StructNameId` comes from the structured tag spelling, not from rendered mirror text.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1280` and `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1321` now store LIR-owned return type tags for declarations and definitions.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:123` and `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:137` now store LIR-owned parameter tags for signature metadata and function params.
- `src/codegen/lir/ir.hpp:613` and `src/codegen/lir/ir.hpp:617` add module-owned tag storage rather than borrowing frontend/HIR string lifetime.

This is semantic repair of the LIR metadata authority path, not testcase-overfit.

## Lifetime / Ownership Review

The lifetime fix is directionally sound. `LirModule::intern_type_tag` stores copied strings in module-owned storage and returns stable `c_str()` pointers for retained `TypeSpec` copies. The use of shared storage also preserves tag lifetime across ordinary `LirModule` copies.

No immediate lifetime hazard was found in the repaired function-signature path.

Residual watch item: `todo.md` correctly notes that globals or stack-object TypeSpec tags were not broadened in this packet. That is not a blocker for this repaired failure, because the failing verifier path compares `LirFunction.return_type` and `LirFunction.signature_*` metadata, but it is worth keeping as a future ownership audit item if later LIR-held `TypeSpec` copies are expected to outlive or diverge from their HIR source.

## Plan Alignment

Plan-alignment judgment: `on track`

The diff matches active Step 2's goal of repairing the structured authority path that caused the mismatched return type-ref. The todo update also records the first bad fact and explains why the fix is semantic rather than name-specific.

## Idea Alignment

Idea-alignment judgment: `matches source idea`

The reopened source idea says the rebuilt LIR failure invalidates the close proof and must be repaired as structured identity propagation/mirror authority, not as a new unrelated initiative. The current fix does that.

## Technical Debt

Technical-debt judgment: `watch`

The new LIR tag storage is acceptable for this slice. The remaining debt is scope boundary debt: only function return/parameter/signature TypeSpec retention is covered. That is reasonable for the repaired failure, but should not be treated as proof that every LIR-held TypeSpec tag has been audited.

## Validation

Validation sufficiency: `needs broader proof`

The focused before/after guard is strong for the repaired regression:

- before: `frontend_lir_function_signature_type_ref` failed with `LirFunction.signature_return_type_ref: return mirror for function 'declared_pair' names a different structured return type than %struct.Big`
- after: `frontend_lir_function_signature_type_ref` passed 1/1

The supervisor also reported a sanity subset covering parser, HIR lookup, LIR function signature type-ref, extern-decl type-ref, and call type-ref passing 5/5. That is enough to continue the route, but because the commit touches shared LIR aggregate type-ref construction and LIR module metadata ownership, the active Step 3 baseline/reclose readiness still needs broader proof before a renewed close.

## Recommendation

Reviewer recommendation: `continue current route`

Continue to Step 3 baseline/reclose readiness. No plan/todo rewrite is required for this implementation slice. Keep the remaining non-signature LIR-held TypeSpec ownership question as a watch item rather than expanding this repair retroactively.

Status: Active
Source Idea Path: ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Inventory HIR-To-LIR String Identity Seams

# Current Packet

## Just Finished

Step 4 from `plan.md`: extended
`review/103_hir_post_dual_path_legacy_readiness_audit.md` with a HIR-to-LIR
string identity seam inventory covering `src/codegen/lir/hir_to_lir/`,
`src/codegen/shared/fn_lowering_ctx.hpp`, `src/codegen/shared/llvm_helpers.hpp`,
and the requested HIR payload fields. The inventory separates output spelling
from downstream semantic handoffs that block HIR-only legacy lookup deletion.

## Suggested Next

Execute Step 5 from `plan.md`: Write Follow-Up Recommendations And Proof Gaps.
Focus the handoff on converting the audit into idea 105 bridge work. The
highest-risk bridge packet should cover HIR-to-LIR callable/global fallbacks:
`DeclRef::link_name_id` coverage, `find_function_by_name_legacy` fallback
removal readiness, and `select_global_object(name)` replacement strategy.

## Watchouts

- This active plan is audit-only; do not edit implementation files.
- Do not demote, delete, or rewrite expectations as part of the audit.
- `Function::name`, `GlobalVar::name`, `DeclRef::name`, `TypeSpec::tag`,
  `HirStructDef::tag`, and `HirStructField::name` all have legitimate spelling
  uses, but some HIR-to-LIR consumers still use them as semantic identity.
- `LinkNameId` is already the preferred cross-boundary callable/global link
  authority where present; deletion blockers are the invalid-ID fallback paths.
- `TypeSpec::tag` and `HirStructField::name` remain major non-function blockers
  because layout, ABI classification, field traversal, and const-init selection
  still consume rendered text as identity.

## Proof

Audit-only packet; no build or test command required by the supervisor.
Lightweight read-only checks were run: verified `c4c-clang-tool`,
`c4c-clang-tool-ccdb`, and `build/compile_commands.json`; ran
`c4c-clang-tool-ccdb function-signatures` on
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`,
`src/codegen/lir/hir_to_lir/const_init_emitter.cpp`, and
`src/codegen/lir/hir_to_lir/call/target.cpp`; ran focused `rg` scans over
`src/codegen/lir/hir_to_lir`, `src/codegen/shared`, and
`src/frontend/hir/hir_ir.hpp`; and inspected the relevant HIR-to-LIR/shared
files read-only. No `test_after.log` was produced because the delegated proof
explicitly did not require a build or test command.

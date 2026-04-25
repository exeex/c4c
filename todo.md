Status: Active
Source Idea Path: ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Diagnostics, Dumps, ABI, And Printer Spelling

# Current Packet

## Just Finished

Step 3 from `plan.md`: extended
`review/103_hir_post_dual_path_legacy_readiness_audit.md` with a Step 3
classification of diagnostics, dumps, ABI/link spelling, printer output, and
`TypeSpec::tag` textual surfaces separately from semantic lookup authority.

## Suggested Next

Execute Step 4 from `plan.md`: Inventory HIR-To-LIR String Identity Seams.

## Watchouts

- This active plan is audit-only; do not edit implementation files.
- Do not demote, delete, or rewrite expectations as part of the audit.
- Diagnostics and HIR printer output can keep rendered spelling for readability;
  that does not make the strings semantic lookup authority.
- ABI/link spelling and HIR-to-LIR `TypeSpec::tag` consumers are different:
  emitted symbol spelling must preserve output, and layout/type emission still
  depends on rendered tags until a downstream bridge exists.
- Dump surfaces that call `classify_*_decl_lookup` are proof/debug visibility,
  not an additional lookup contract.

## Proof

Audit-only packet; no build or test command required by the supervisor.
Lightweight read-only checks were run: focused `rg` scans over
`src/frontend/hir`, `src/codegen/lir/hir_to_lir`, and `src/codegen/lir` for
diagnostics, dumps, printer paths, `TypeSpec::tag`, link-name spelling,
ABI-visible symbol rendering, and `struct_defs.find` consumers, plus read-only
inspection of the relevant printer, compile-time diagnostic, HIR-to-LIR, and LIR
printer files. No `c4c-clang-tool` query was needed for this textual-surface
classification. No `test_after.log` was produced because the delegated proof
explicitly did not require a build or test command.

Status: Active
Source Idea Path: ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify String-Keyed HIR Maps And Fallbacks

# Current Packet

## Just Finished

Step 2 from `plan.md`: extended
`review/103_hir_post_dual_path_legacy_readiness_audit.md` with a Step 2
classification of remaining string-keyed HIR maps, rendered-name fallbacks,
and parity-only mirrors as `safe-to-demote`, `legacy-proof-only`,
`bridge-required`, or `needs-more-parity-proof`.

## Suggested Next

Execute Step 3 from `plan.md`: classify diagnostics, dumps, ABI/link spelling,
printer output, and other textual surfaces separately from semantic lookup
authority.

## Watchouts

- This active plan is audit-only; do not edit implementation files.
- Do not demote, delete, or rewrite expectations as part of the audit.
- Direct `struct_methods_`, `struct_method_ret_types_`, `struct_defs`,
  enum/const-int name maps, template name registries, and HIR-to-LIR rendered
  fallback paths are classified as bridge-required or needing more parity proof;
  do not treat the existence of structured mirrors as cleanup permission.
- HIR-to-LIR call and const-init paths still fall back from `LinkNameId` to
  rendered names; keep idea 104 HIR cleanup separate from idea 105 bridge work.
- Step 2 found `c4c-clang-tool-ccdb function-callers` insufficient for several
  method-qualified/cross-TU helper paths; focused source scans and definitions
  were used for those classifications.

## Proof

Audit-only packet; no build or test command required by the supervisor.
Lightweight read-only checks were run: `command -v c4c-clang-tool`,
`command -v c4c-clang-tool-ccdb`, `test -f build/compile_commands.json`,
focused `rg` scans over `src/frontend/hir` and `src/codegen/lir/hir_to_lir`,
read-only inspections of the relevant HIR storage/helper definitions, and
targeted `c4c-clang-tool-ccdb function-callers` attempts for selected helper
symbols. No `test_after.log` was produced because the delegated proof
explicitly did not require a build or test command.

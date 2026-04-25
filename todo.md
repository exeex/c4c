Status: Active
Source Idea Path: ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory HIR Legacy Lookup APIs And Callers

# Current Packet

## Just Finished

Step 1 from `plan.md`: inventoried remaining HIR legacy lookup APIs and direct
callers, then created `review/103_hir_post_dual_path_legacy_readiness_audit.md`
with a Step 1 table covering module function/global lookup, compile-time
template/consteval registries, enum/const-int maps, struct owner/method/member
helpers, fallback-only paths, and authoritative rendered lookup seams.

## Suggested Next

Execute Step 2 from `plan.md`: classify remaining HIR string-keyed maps and
fallbacks, using the Step 1 inventory to separate parity-only mirrors from
bridge-required rendered-name authority.

## Watchouts

- This active plan is audit-only; do not edit implementation files.
- Do not demote, delete, or rewrite expectations as part of the audit.
- Direct `struct_methods_`, `struct_method_ret_types_`, `struct_defs`,
  enum/const-int name maps, and template name registries still have
  authoritative rendered lookup paths; do not treat the existence of structured
  mirrors as cleanup permission without Step 2 classification.
- HIR-to-LIR call and const-init paths still fall back from `LinkNameId` to
  rendered names; keep idea 104 HIR cleanup separate from idea 105 bridge work.

## Proof

Audit-only packet; no build or test command required by the supervisor.
Lightweight read-only checks were run: `command -v c4c-clang-tool`,
`command -v c4c-clang-tool-ccdb`, `test -f build/compile_commands.json`,
targeted `c4c-clang-tool-ccdb function-signatures` queries, targeted
`c4c-clang-tool-ccdb function-callers` queries, and focused `rg` scans.
No `test_after.log` was produced because the delegated proof explicitly did not
require a build or test command.

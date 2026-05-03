# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Codegen Tag Surfaces

## Just Finished

Lifecycle switch: parent idea
`ideas/open/141_typespec_tag_field_removal_metadata_migration.md` is parked at
the frontend/Sema/HIR boundary after Step 4 deletion probing. The next first
field-removal boundary is downstream codegen/LIR aggregate identity metadata,
tracked by the new active follow-up idea
`ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md`.

## Suggested Next

Execute Step 1 by inventorying `src/codegen` `.tag` surfaces and classifying
each as semantic aggregate identity, layout lookup, call/ABI type ref,
field/member access, final LLVM spelling, diagnostic text, or compatibility.
Use the known first deletion-probe boundary
`src/codegen/shared/llvm_helpers.hpp:444` as the starting point.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve final LLVM spelling, diagnostics, dumps, mangling,
  ABI/link-visible text, and explicitly named compatibility payloads.
- Treat parser/Sema/HIR residuals as parent-idea scope only if a codegen route
  proves an upstream producer carrier is still missing.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Use deletion builds only as probes; do not commit a broken deletion build.

## Proof

Lifecycle-only transition; no implementation proof command was run.

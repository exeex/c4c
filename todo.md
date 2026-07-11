Status: Active
Source Idea Path: ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Reference-Only Core Adapter

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from `ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`. No implementation work has started.

## Suggested Next

Start Step 1 by adding a reference-only `PreparedMirCoreView` / `PreparedMirFunctionView` adapter over the current `prepare::PreparedBirModule`. Keep the first packet behavior-preserving and prove it with a fresh build plus any focused adapter test that fits the touched surface.

## Watchouts

- Do not expose the entire `PreparedBirModule` through a renamed view.
- Keep diagnostic/proof facts observational; they must not select emitted operands, labels, homes, ABI resources, provenance, or feature admission.
- Keep x86 public API compatibility stable until the Step 2 internal migration packet explicitly changes the call path.
- Leave RV64, AArch64, optional feature views, raw dependency gates, and old/new comparator work out of Step 1 unless needed for compilation.

## Proof

Activation-only proof:

- `git diff --check -- plan.md todo.md ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md docs/prepared_mir_view_contract_research`

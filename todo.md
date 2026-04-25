Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured Mirrors for Struct Definitions

# Current Packet

## Just Finished

Completed the next Step 3 slice by dual-writing lowerer-local structured record-owner mirrors for template struct primary and specialization registration without changing existing template lookup authority.

Added:
- `template_struct_defs_by_owner_` beside `template_struct_defs_`, keyed by the primary template struct declaration owner key when metadata is complete.
- `template_struct_specializations_by_owner_` beside `template_struct_specializations_`, keyed by the primary template struct declaration owner key when metadata is complete.
- Dual-writes in `register_template_struct_primary` and `register_template_struct_specialization` while leaving `find_template_struct_primary` and `find_template_struct_specializations` on the rendered-name / `ct_state_` path.

Existing rendered-name maps, `ct_state_` template lookup behavior, HIR dumps, codegen spelling, tests, and expectation files remain authoritative and unchanged.

## Suggested Next

Continue Step 3 by adding focused dual-read/parity probes that compare structured and rendered template-struct lookup results without redirecting existing consumers.

## Watchouts

Keep `TypeSpec::tag`, `HirStructDef::tag`, `Module::struct_defs`, `struct_def_order`, and lowerer template lookup consumers as the rendered output-spelling bridge. This slice records structured mirrors only; no existing reader has been redirected. Template-struct specialization mirrors are keyed by primary declaration owner identity, matching the rendered map's primary-name ownership, not by a full specialization identity.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.

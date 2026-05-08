Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire Member Typedef And Signature Recovery Fallbacks

# Current Packet

## Just Finished

Completed Plan Step 5 by retiring rendered/tag-map owner recovery in `resolve_struct_member_typedef_if_ready` after a complete structured owner key exists but misses.

`owner_tag_from_type_metadata` now treats a complete `owner_key_from_type(owner)` result as authoritative: it returns the structured owner tag on an owner-index hit and stops on a miss before scanning `struct_def_nodes_` or `module_->struct_defs` by rendered/tag metadata. Legacy rendered compatibility remains available only when structured owner metadata is absent. Focused coverage now proves stale `struct_def_nodes_` and stale module `struct_defs` tag recovery cannot resolve a deferred member typedef after the complete owner key misses.

## Suggested Next

Next coherent packet: continue Step 5 by retiring any remaining signature/member-typedef rendered recovery path outside `resolve_struct_member_typedef_if_ready`, if supervisor review identifies one; otherwise move to the next Step 5 fallback family.

## Watchouts

- `resolve_struct_member_typedef_type(const HirRecordOwnerKey&, ...)` still bridges a structured owner hit to rendered `struct_def_nodes_` storage through the module tag; this packet only makes complete owner-key misses terminal.
- Existing parser-owned owner-key canonicalization tests remain intentionally spelling-repaired before the structured hit/miss decision.
- `module_->struct_defs` can still be used as the rendered storage bridge after a structured owner hit; it is no longer a fallback authority after a complete owner-key miss.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_hir_lookup_tests && ctest --test-dir build -R '^frontend_hir_lookup_tests$' --output-on-failure > test_after.log`

Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Tighten `last_resolved_typedef` Around `TextId`

# Current Packet

## Just Finished

Step 2 audit packet completed for the remaining `last_resolved_typedef` call
sites in `parser_types_base.cpp` and `parser_declarations.cpp`.

`parser_declarations.cpp` already hands return-type typedef metadata to
function declarations through `last_resolved_typedef_text_id`. In
`parser_types_base.cpp`, the typedef-resolution path sets
`last_resolved_typedef` from the resolved typedef spelling, then immediately
uses `last_resolved_typedef_text_id` for alias-template key lookup,
fallback alias-template probing, qualified template-struct primary lookup, and
transformed-owner propagated typedef metadata. The remaining spelling uses are
lookup input, fallback spelling, or bridge compatibility for existing helper
APIs, not semantic metadata authority.

## Suggested Next

Step 2 can advance to Step 3. Use an inventory-only packet for
`current_struct_tag` active-context mirror cleanup, classifying where
structured `current_struct_tag_text_id` already drives visibility or ownership
and where spelling still drives owner registration, injected/member context, or
diagnostic fallback.

## Watchouts

- This packet intentionally did not change alias-template storage,
  template-struct lookup, or typedef visibility table semantics.
- `set_last_resolved_typedef(...)` remains the only direct writer in the
  audited Step 2 path; it recomputes the stored `TextId` from spelling and all
  audited semantic handoff sites read the `TextId` field.
- `current_struct_tag` remains the higher-risk family because several paths
  still use spelling for owner registration and injected/member contexts.

## Proof

No build required for this audit-only packet.

Local validation passed:
`git diff --check`

Proof log path: none updated; `test_after.log` was not touched.

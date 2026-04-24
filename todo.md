Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce one structured key family for alias-template registration and lookup

# Current Packet
## Just Finished
Completed plan Step 2 by moving alias-template registration and lookup onto a structured `QualifiedNameKey` path:
- `template_state_.alias_template_info` is now keyed by `QualifiedNameKey` instead of rendered strings.
- `parser_declarations.cpp` registers alias-template metadata through `alias_template_key_in_context(...)` from the active parser context.
- `parser_types_declarator.cpp` and `parser_types_base.cpp` now query alias-template metadata through `find_alias_template_info_in_context(...)` rather than direct rendered-name map probes.
- The other template-owned string tables were left untouched, per packet scope.

## Suggested Next
Start the Step 3 packet that retargets alias-template registration away from `set_last_using_alias_name(...)`-style spelling bookkeeping and toward the structured alias key path already introduced here.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Keep any bridge-only spelling recovery narrow and parser-local.

## Proof
Ran `bash -lc 'cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$" | tee test_after.log'`.
`test_after.log` was updated and the delegated proof passed.

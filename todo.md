Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget alias-template registration away from spelling-shaped bookkeeping

# Current Packet
## Just Finished
Finished Step 3 by capturing the alias-template registration key in parser context and writing `alias_template_info` from the structured alias key instead of recovering it from rendered alias spelling.

## Suggested Next
Start Step 4 by migrating one template-owned lookup family at a time to the structured identity path now that alias-template registration is no longer spelled-name driven.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Keep registration and lookup aligned to the structured key path where it is already available.
- Preserve any remaining compatibility behavior as explicit parser-local bridge logic.
- Avoid widening into template-owned string-table cleanup or Step 5 heuristic narrowing.

## Proof
`cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$" | tee test_after.log`
Subset: `frontend_parser_tests`
Log: `test_after.log`
Result: green

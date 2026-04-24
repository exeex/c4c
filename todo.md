Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate one template-owned lookup family at a time

# Current Packet
## Just Finished
Finished Step 4 for the primary template struct registration/lookup family by adding a structured `QualifiedNameKey` registry path, registering primary templates through parser context instead of only spelled names, and switching the nearby parser-owned primary lookups to that structured path while leaving the legacy string table as an explicit compatibility bridge for unowned parser consumers.

## Suggested Next
Migrate the next template-owned lookup family on the same route, with specializations and remaining compatibility-only direct string-table probes as the likely next bounded packet.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Keep registration and lookup aligned to one structured key path at a time.
- Preserve the legacy string table only as a bridge for parser files outside this packet that still probe `template_struct_defs` directly.
- Avoid widening this packet into template specialization identity or non-parser HIR template registries.
- Watch for namespace-sensitive lookups that still arrive through raw rendered names in unowned parser files.

## Proof
`cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$" | tee test_after.log`
Subset: `frontend_parser_tests`
Log: `test_after.log`
Result: green

Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate one template-owned lookup family at a time

# Current Packet
## Just Finished
Finished another Step 4 template-owned lookup family by adding a structured `QualifiedNameKey` specialization registry, registering template-struct specializations through parser context instead of only spelled primary names, and switching the nearby parser-owned specialization selection path to that structured lookup while leaving the legacy string map as an explicit bridge.

## Suggested Next
Keep Step 4 on the same route by narrowing the remaining compatibility-only direct string-table probes around template-owned parser lookup, then move to Step 5 once specialization and alias-application bridge paths are clearly separated.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Keep registration and lookup aligned to one structured key path at a time.
- Preserve the legacy string tables only as bridges for parser files outside the current packet that still probe them directly.
- Avoid widening this packet into non-parser HIR template registries or unrelated alias-application cleanup before Step 5.
- Watch for namespace-sensitive lookups that still arrive through raw rendered names in unowned parser files.

## Proof
`cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$" | tee test_after.log`
Subset: `frontend_parser_tests`
Log: `test_after.log`
Result: green

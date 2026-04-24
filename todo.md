Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate one template-owned lookup family at a time

# Current Packet
## Just Finished
Finished another Step 4 parser-owned lookup cleanup by factoring shared owner-prefix extraction/resolution around `QualifiedNameRef`, then routing the remaining declaration/expression owner probes through that structured path instead of re-deriving owner names from the first qualifier or ad-hoc visible-type fallback. This keeps `ns::Owner::member`-style owner detection aligned with the same structured template/type identity helpers already used by the parser template lookup path.

## Suggested Next
Keep Step 4 on the same route by converting the remaining alias-member owner/member probes in `parser_types_base.cpp` away from rendered owner-tag recovery and onto the same structured owner resolution helper, then move to Step 5 once alias-application heuristics are the main bridge-only holdout.

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

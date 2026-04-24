Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate one template-owned lookup family at a time

# Current Packet
## Just Finished
Finished another Step 4 parser-owned lookup cleanup by routing template-primary presence checks in helper/type-probe paths through `find_template_struct_primary(...)`, so parser-side template-owned lookup now prefers the structured `QualifiedNameKey` registry before falling back to legacy string tables that remain as explicit bridges for untouched call sites.

## Suggested Next
Keep Step 4 on the same route by replacing the remaining non-parser helper direct probes into `template_struct_defs` with structured-first checks or dedicated wrappers, then move to Step 5 once alias-application heuristics are the main bridge-only holdout.

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

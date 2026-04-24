Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate one template-owned lookup family at a time

# Current Packet
## Just Finished
Finished another Step 4 parser-owned lookup cleanup by adding a shared `QualifiedNameRef` parser for qualified spellings, then routing alias-member owner/member recovery in `parser_types_base.cpp` through structured owner refs instead of raw `rfind("::")` splitting and string-only template-owner probes. This keeps alias-member follow-through aligned with the same structured owner lookup helpers already used by the declaration/expression template-owner path.

## Suggested Next
Treat Step 4 as close to exhausted and review whether the remaining alias-application `_t` / trait-family transforms in `parser_types_base.cpp` are now the main bridge-only holdout; if so, advance to Step 5 and narrow those heuristics instead of adding more owner-spelling recovery.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Keep registration and lookup aligned to one structured key path at a time.
- Preserve the legacy string tables only as bridges for parser files outside the current packet that still probe them directly.
- Avoid widening this route into non-parser HIR template registries or broad alias-application cleanup before the Step 5 handoff is explicit.
- The remaining alias fallback surface appears to be trait-style `_t` and unary transform heuristics rather than owner/member parsing; review those as bridge-only behavior before declaring Step 4 done.

## Proof
`cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$" | tee test_after.log`
Subset: `frontend_parser_tests`
Log: `test_after.log`
Result: green

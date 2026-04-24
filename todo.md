Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Narrow bridge-only heuristics and validate parser coverage

# Current Packet
## Just Finished
Finished Step 5 by narrowing the alias-application bridge in `parser_types_base.cpp`: structured owner/member lookup now runs before the `_t` / trait-style compatibility fallbacks, and the legacy bridge branch is called out explicitly as fallback-only. The owned parser slice stayed in `parser_types_base.cpp` and the delegated frontend parser test subset passed.

## Suggested Next
Advance to the next parser packet in the idea by checking whether any other alias-application compatibility bridges still outrank structured identity, then tighten only those remaining fallbacks if they exist.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Keep registration and lookup aligned to one structured key path at a time.
- Preserve the legacy string tables only as bridges for parser files outside the current packet that still probe them directly.
- Avoid widening this route into non-parser HIR template registries or broad alias-application cleanup before the Step 5 handoff is explicit.
- The remaining alias fallback surface should stay bridge-only; do not re-promote `_t` / trait recovery ahead of structured owner/member lookup.

## Proof
`cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$" | tee test_after.log`
Subset: `frontend_parser_tests`
Log: `test_after.log`
Result: green

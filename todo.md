Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Structured Mirrors for Member and Static-Member Lookup

# Current Packet

## Just Finished

Completed the Step 5 parity-only structured dual-read slice for member and static-member lookup helpers.

Added local structured-key construction and parity counters for static member declarations, static member const values, and member symbol IDs. `find_struct_static_member_decl`, `find_struct_static_member_const_value`, and `find_struct_member_symbol_id` now compare rendered local hits against the structured owner/member mirrors when metadata is available, while still returning the same rendered/local-or-base-recursive result as before.

## Suggested Next

Step 5 appears ready to move to Step 6 from this executor slice; no remaining base/static parity work was identified inside the delegated scope.

## Watchouts

Parity is recorded only on rendered local hits, matching the existing method parity pattern. Base-class recursion is unchanged, so inherited hits still record parity in the recursive base-tag call. Structured maps remain observational and do not affect `MemberExpr::resolved_owner_tag` spelling, rendered map writes, or codegen behavior.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.

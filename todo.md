Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Structured Mirrors for Member and Static-Member Lookup

# Current Packet

## Just Finished

Completed the first Step 5 write-only mirror slice for member and static-member lookup metadata.

Added `HirStructMemberLookupKey` as `HirRecordOwnerKey + member TextId` and lowerer-owned structured mirror maps for member symbol IDs, static member declarations, and static const values. Ordinary struct lowering and instantiated template struct field/static-member registration now dual-write those mirrors when owner/member metadata is complete. Existing rendered maps, rendered member-symbol interning, and all member/static lookup readers remain authoritative and unchanged.

## Suggested Next

Continue Step 5 with dual-read/base parity checks for `find_struct_static_member_decl`, `find_struct_static_member_const_value`, and `find_struct_member_symbol_id`, keeping rendered lookup results authoritative while comparing structured local hits where owner/member metadata is available.

## Watchouts

The existing rendered `struct_static_member_decls_` map records every named field node before static filtering; this slice mirrors that same write surface to avoid changing current behavior. Template field mirrors use the template-instantiation owner key before the struct definition is inserted, then the existing owner registration path still indexes the same key later.

This slice does not add parity counters, convert any reader to structured authority, touch method lookup maps, change base-class recursion, or alter codegen output behavior.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.

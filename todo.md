Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Structured Mirrors for Method Lookup

# Current Packet

## Just Finished

Completed the Step 4 parity-only dual-read slice for struct method lookup helpers.

`find_struct_method_mangled`, `find_struct_method_link_name_id`, and `find_struct_method_return_type` still return the rendered-map/local-or-base-recursive result exactly as before. On each rendered local hit, they now derive the structured owner/method/const key when metadata is available, compare the matching structured mirror, and record lowerer-local parity check/mismatch counters for the mangled name, link-name id, and return-type mirrors.

## Suggested Next

Continue Step 4 only if the supervisor wants the remaining method/base/template validation tightened further, such as surfacing the new parity counters in an existing diagnostic path or adding a focused follow-up for any uncovered const-fallback or template-instance method lookup route. Do not convert method readers to structured authority yet.

## Watchouts

The parity bridge resolves rendered tags back through `module_->struct_def_owner_index`, so template-instantiation method mirrors can be compared with their template owner key instead of inventing a declaration key from the mangled tag. Base-class recursion is unchanged; parity is recorded by the recursive local hit that actually finds the rendered method.

This slice does not touch member or static-member lookup maps, rendered method map writes, test expectations, or codegen behavior. Ref-overloaded methods still share the existing rendered key authority; the mirror tracks the same owner/method/constness identity currently represented by `struct_tag::method[_const]`, not a fuller overload signature.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.

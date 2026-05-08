Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Retire Compatibility Authority

# Current Packet

## Just Finished

Completed Step 6 implementation for `Validate And Retire Compatibility Authority`.

Parser-local template-scope relabeling in `src/frontend/parser/impl/declarations.cpp` now builds an owner `QualifiedNameRef` from existing owner segment `TextId` metadata and calls `find_template_struct_primary(const QualifiedNameRef&)`. The rendered `qualified_owner` and `qualified_owner_tag` strings remain only as display values for `owner_struct_tag` and emitted function names.

Retired rendered-owner template-primary lookups at the out-of-class conversion-operator, constructor, and generic qualified declarator owner-scope relabel sites. Added focused `frontend_parser_lookup_authority` coverage that manually pushes a `FreeFunctionTemplate` frame and proves parser relabeling comes from structured owner metadata even when the full rendered `ns::Owner` TextId has no template-owner authority.

## Suggested Next

Supervisor review and commit for this completed parser-owned Step 6 slice, then decide whether remaining HIR/LIR compatibility routes need a separate plan-owner packet or closure review.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- `qualified_owner` and `qualified_owner_tag` are still valid display/fallback spellings, but they no longer feed parser template-owner relabel authority in `declarations.cpp`.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; it is now kept only after structured out-of-class metadata is absent or incomplete in both attached-method and non-method lowering.
- Do not retire HIR/LIR aggregate compatibility in the same packet as parser template-scope relabeling; that path is broader layout/type recovery and needs separate proof.

## Proof

Delegated proof passed:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_lookup_authority') > test_after.log 2>&1`

Proof log: `test_after.log`.

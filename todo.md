Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4B
Current Step Title: Delete TextId-Only Parser-Map Record Fallback

# Current Packet

## Just Finished

Step 4A/4B repaired the remaining parser-owned carrier gap after deleting the
context-defaulted unique same-`TextId` parser-map fallback from
`resolve_record_type_spec`. `vector<T, Allocator>::~vector` now parses as an
out-of-class destructor by carrying the owner metadata from the consumed
template owner `TypeSpec` into the declarator name and normalizing the function
type to `void` before incomplete-object checks. Out-of-class template member
bodies now also consult the current-record dependent member typedef carrier for
unqualified member typedefs such as `reverse_iterator`.

Parser tests cover the fallback deletion, stale structured-context rejection,
the out-of-class destructor owner path, and the repaired `reverse_iterator`
member-typedef carrier path.

## Suggested Next

Supervisor should review and commit the completed Step 4B slice, or escalate to
broader validation if treating this as a milestone.

## Watchouts

Do not restore parser-map uniqueness, rendered-key matching, or
context-defaulted TextId lookup. The accepted route is parser-owned structured
carrier repair in `base.cpp` and `declarator.cpp`; Sema/HIR files were not
needed.

## Proof

`test_after.log` records the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_eastl_vector_parse_recipe)$'`.
Build passed. `frontend_parser_tests`,
`frontend_parser_lookup_authority_tests`, and `cpp_eastl_vector_parse_recipe`
all passed with the Step 4B fallback deletion still in place.

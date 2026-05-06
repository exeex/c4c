Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4A
Current Step Title: Supply Parser/Sema Carrier For Record References

# Current Packet

## Just Finished

Lifecycle repair rewrote Step 4 into ordered substeps after the reviewed
`resolve_record_type_spec` repair exposed a missing carrier handoff. The active
route stays inside the source idea because parser provisional carriers and
Sema-owned record identity are already in scope.

## Suggested Next

Execute Step 4A: provide the missing parser/Sema carrier for record references
that reach `resolve_record_type_spec` without `record_def`. The immediate
target is the EASTL `vector` and `reverse_iterator` TypeSpecs that failed as
incomplete after the TextId-only parser-map fallback was removed.

The next packet should supply authoritative namespace/owner metadata or a
Sema-owned record identity handoff for those references before deleting or
narrowing the unique same-`TextId` path in Step 4B.

## Watchouts

`TypeSpec` has no legacy rendered tag visible to `resolve_record_type_spec`
(`support_legacy_typespec_tag_if_present(const TypeSpec&, long)` returns
`nullptr`), so a parser-only exact rendered fallback cannot repair the EASTL
failure. Restoring the old unique same-`TextId` scan would reintroduce the
reviewed route drift. Do not broaden parser-map uniqueness, rendered-key
matching, or context-defaulted TextId lookup while implementing Step 4A.

## Proof

`test_after.log` records the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_eastl_vector_parse_recipe)$'`.
Build passed and `frontend_parser_tests` plus
`frontend_parser_lookup_authority_tests` passed. `cpp_eastl_vector_parse_recipe`
failed with incomplete `vector` and `reverse_iterator` records after the
TextId-only fallback was removed.

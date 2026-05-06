Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4A
Current Step Title: Supply Parser/Sema Carrier For Record References

# Current Packet

## Just Finished

Step 4A supplied the missing qualified-template parser carrier upstream of
`resolve_record_type_spec`: non-dependent qualified template record references
now invoke template instantiation during `try_parse_qualified_base_type`, so
`eastl::vector<int>` and `eastl::reverse_iterator<int*>`-shaped TypeSpecs carry
`record_def` while retaining namespace/qualifier metadata. Added parser proof
that stale display spelling still resolves through `record_def` without a
parser-map fallback.

## Suggested Next

Execute Step 4B: delete or narrow the context-defaulted unique same-`TextId`
candidate path in `resolve_record_type_spec`, replacing any still-needed
acceptance with full structured carrier matches or a Sema-owned structured
record lookup.

## Watchouts

Step 4A deliberately materializes only non-dependent qualified template
references; dependent template references stay as structured origin/argument
carriers for later instantiation. Step 4B should not reintroduce parser-map
uniqueness, rendered-key matching, or context-defaulted TextId lookup. Existing
tests still include `test_parser_record_layout_const_eval_accepts_unique_structured_record_match`,
which should be revised or removed as part of Step 4B's fallback deletion.

## Proof

`test_after.log` records the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_eastl_vector_parse_recipe)$'`.
Build passed, and `frontend_parser_tests`,
`frontend_parser_lookup_authority_tests`, and `cpp_eastl_vector_parse_recipe`
all passed.

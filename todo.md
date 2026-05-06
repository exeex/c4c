Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C parser static-member record lookup migration completed. After
`ensure_template_struct_instantiated_from_args`, the lookup now accepts only the
structured `TypeSpec` record authority returned by the instantiation path and no
longer falls back through `definition_state_.struct_tag_def_map.find(ref_mangled)`
or compatibility-map recovery keyed by the rendered template instance name.

## Suggested Next

Supervisor should review and commit this focused Step 4C slice with
`src/frontend/parser/impl/types/template.cpp` and this `todo.md` update. The
untracked `review/step4c_repair_route_review.md` remains outside this packet.

## Watchouts

`ref_mangled` is still populated because the existing instantiation API requires
an output name, but this path no longer uses it as semantic lookup authority.
Static-member lookup now fails if structured instantiation does not return a
usable `record_def`.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_template_struct_advanced_cpp|cpp_eastl_vector_parse_recipe|cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp|cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp|cpp_std_vector_parse_debug_progress|cpp_std_vector_parse_debug_progress_file)$' > test_after.log`

Result: passed, 7/7 tests. Proof log: `test_after.log`.

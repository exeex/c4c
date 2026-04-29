Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Template Primary And Specialization Lookup Structured-Primary

# Current Packet

## Just Finished

Step 2 is closed for canonical execution. The supervisor reviewed the remaining
`defined_struct_tags.count`, `struct_tag_def_map.find`,
`struct_tag_def_map[...]`, and `has_defined_struct_tag` hits after commits
`16f3b2d6` and `37f0b42f`; remaining record-mirror uses are producer writes,
documented compatibility fallbacks after structured lookup, tests/fixtures, or
template-family paths owned by Step 3/4. Matching `frontend_parser_tests`
before/after guard passed with equal pass counts allowed.

## Suggested Next

Next coherent packet: begin Step 3 by inventorying
`template_struct_defs`, `template_struct_specializations`, and related template
lookup helpers, then convert the first narrow semantic lookup path to prefer
`QualifiedNameKey`, `TextId`, parser template ids, direct definition
references, or another existing structured carrier before rendered template
spelling.

## Watchouts

Do not expand Step 3 by weakening expectations or deleting rendered mirrors
before all semantic consumers have structured replacements. Preserve generated
specialization spelling as final artifact or debug text, and make any rendered
fallback visibly non-authoritative.

## Proof

Passed:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Proof log: `test_after.log`.

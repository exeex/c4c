Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Record Definition Lookup Structured-Primary

# Current Packet

## Just Finished

Step 2 converted the remaining direct parser record mirror consumers named in
the packet to structured-primary lookup where a carrier exists. Expression
qualified-owner disambiguation now checks `QualifiedNameRef` record identity
before rendered `defined_struct_tags`; template base instantiation fixups and
member-typedef owner repair now consult `TypeSpec::record_def` before rendered
`struct_tag_def_map`; dependent typename owner traversal now resolves owner and
nested owner records through structured qualified/type identity before rendered
map fallback.

## Suggested Next

Next coherent packet: have the supervisor review Step 2 for closure, with the
remaining direct mirror hits classified as producer writes, existing
string-only compatibility fallbacks, or parser paths outside this packet's
consumer conversion scope.

## Watchouts

Rendered maps remain necessary compatibility bridges for tag-only paths. The
remaining direct `struct_tag_def_map.find()` calls in the touched parser paths
are now after structured lookup and documented as fallbacks; producer writes and
test fixture setup were not changed.

## Proof

Passed:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Proof log: `test_after.log`.

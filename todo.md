# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Propagate Typed Record Identity Through Template Records

## Just Finished

Completed Step 4 first bounded template propagation packet.

`types/template.cpp::ensure_template_struct_instantiated_from_args()` now
preserves `TypeSpec::record_def` when explicit full-specialization reuse
selects a concrete specialization, even if a stale rendered instantiation map
entry already exists. Its tag-only injected fallback reconstruction also
copies `record_def` from the known instantiated `struct_tag_def_map` node.

`types/base.cpp` now returns the newly created direct template-instantiation
record through the caller `TypeSpec::record_def`, and both template-base
fallback reconstructions from `base_mangled` preserve the known base
instantiation node as `record_def`.

Focused parser coverage now checks explicit specialization reuse against a
stale rendered template tag-map payload, and direct template emission checks
that newly created instantiations return the created `record_def`.

## Suggested Next

Run the broader Step 4 substitution copy audit: member typedefs, fields, method
return types, and method parameters should preserve `record_def` when template
type-parameter substitution copies a record-backed actual type into an
instantiated record payload.

## Watchouts

Rendered template instantiation keys, direct-emission dedup sets,
`TypeSpec::tag`, and `struct_tag_def_map` mirror writes were intentionally
left unchanged. Direct template-emission reuse does not yet infer
`record_def` from an existing rendered map entry because that can be stale; it
only returns `record_def` when this producer creates the concrete node.

The remaining Step 4 work is propagation through substitution copies, not
deleting rendered compatibility state.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Result: build completed; `frontend_parser_tests` and `frontend_hir_tests`
passed.

Proof log: `test_after.log`

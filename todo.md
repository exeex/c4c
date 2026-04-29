# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Convert Parser Record Consumers To Typed Identity

## Just Finished

Completed Step 3 remaining-consumer inventory after the support,
dependent-typename, and incomplete-declaration conversions.

Current `struct_tag_def_map` classification:

- Already converted through `resolve_record_type_spec()` /
  `TypeSpec::record_def` first:
  - `support.cpp` layout helpers, `compute_offsetof`, and both
    `eval_const_int` overloads.
  - `expressions.cpp` `__builtin_offsetof` constant folding, because it builds
    `NK_OFFSETOF` and immediately calls `eval_const_int`.
  - `declarations.cpp` top-level and local incomplete-object checks.
  - `declarations.cpp` constexpr/global initializer const-eval, because it
    routes through `eval_const_int`.
  - `types/base.cpp` attribute constant expressions and `types/struct.cpp`
    bitfield widths, because they route through `eval_const_int`.
  - The initial typedef-derived owner lookup in
    `types/declarator.cpp::parse_dependent_typename_specifier`.
- TypeSpec-backed candidates still reading by rendered tag:
  - `types/declarator.cpp` nested dependent-typename owner traversal still
    resolves `nested_decl->type.tag` through `struct_tag_def_map`; it should
    try `resolve_record_type_spec(nested_decl->type, ...)` first.
  - `types/base.cpp::lookup_struct_member_typedef_recursive` resolves typedefs
    to a struct-like `TypeSpec` but then finds the owner by rendered
    `resolved_tag`; it should prefer the resolved `TypeSpec::record_def` where
    available.
  - `types/template.cpp::eval_deferred_nttp_default` recursively scans base
    classes for static members by `base_ts.tag`; instantiated base `TypeSpec`
    values should use typed identity before rendered lookup once propagation
    reaches those paths.
- Rendered-name-only compatibility or final-spelling paths:
  - `support.cpp::register_struct_definition_for_testing`.
  - `types/struct.cpp::register_record_definition`, which maintains source and
    canonical rendered tag mirrors.
  - direct map checks used only to test or preserve the existence of rendered
    template instantiation mirrors.
- Future template propagation work:
  - `types/template.cpp::ensure_template_struct_instantiated_from_args` writes
    and tests rendered instantiation keys, but its `out_resolved` full
    specialization and fallback paths still only populate `tag`.
  - `types/base.cpp` direct template instantiation/base propagation paths set
    instantiated `TypeSpec::tag` from rendered keys and generally do not attach
    the instantiated `Node*` to `record_def`.

## Suggested Next

Next bounded packet: convert the nested dependent-typename owner traversal in
`types/declarator.cpp` so `nested_decl->type` uses
`resolve_record_type_spec()` / `TypeSpec::record_def` before falling back to
`struct_tag_def_map` by rendered tag. Add focused parser coverage for a nested
owner field whose `record_def` points at the real nested record while
`type.tag` points at a stale rendered map entry.

Suggested focused proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

The `expressions.cpp` `NK_OFFSETOF` site is already covered by the support
conversion through `eval_const_int`; do not spend an implementation packet
there unless a new direct lookup appears. Template instantiation paths are
still a larger propagation family and should not be mixed into the nested
dependent-typename packet.

## Proof

Inventory-only packet. Build/tests intentionally not run.

Local validation:
`git diff --check -- todo.md`

No new proof log generated.

# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Demote struct_tag_def_map To Compatibility Mirror

## Just Finished

Step 5 converted the template static-member lookup blocker in
`types/template.cpp`.

`try_eval_template_static_member` now asks
`ensure_template_struct_instantiated_from_args()` for a resolved `TypeSpec` and
uses `resolve_record_type_spec()` so `TypeSpec::record_def` is semantic
authority when present. The rendered `struct_tag_def_map` lookup remains only
as fallback for tag-only compatibility.

Recursive inherited static-member lookup now resolves each `base_ts` through
`resolve_record_type_spec(base_ts, &definition_state_.struct_tag_def_map)`,
which prefers typed base identity before rendered tag fallback.

Focused parser coverage now proves stale rendered map entries cannot redirect
template static-member lookup: the instantiated record map key points at one
stale record and the base tag points at another stale record, but lookup still
returns the static member value from the real typed base `record_def`.

## Suggested Next

Run a final Step 5 classification pass over remaining
`DefinitionState::struct_tag_def_map` readers/writers and record whether the
map can now be documented or renamed as a compatibility mirror without changing
behavior. Keep the packet inventory-first and only make a tiny documentation or
name-local clarification if the classification is unambiguous.

## Watchouts

Do not delete `struct_tag_def_map` or remove rendered template compatibility
keys during Step 5. Preserve `TypeSpec::tag` as spelling, diagnostics, emitted
text, and compatibility payload.

The converted static-member path still passes the map to
`resolve_record_type_spec()` and `eval_const_int()` for tag-only/layout
compatibility; that is intentional fallback behavior, not semantic authority
when a `record_def` exists.

## Proof

Delegated proof for this packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests|frontend_cxx_)' > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.

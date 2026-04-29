# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Propagate Typed Record Identity Through Template Records

## Just Finished

Completed Step 4 direct base-type substitution propagation.

`types/base.cpp` now copies `pts.record_def` when direct template
instantiation substitutes a record-backed template type parameter into
`inst->base_types[bi]`. Existing rendered tag spelling, pointer depth, and cv
flag propagation remain unchanged.

Focused parser coverage now extends the direct substitution fixture with a
template-instantiated base type. The test proves the instantiated base keeps
the actual `TypeSpec::record_def` from the typed template argument even when
the rendered `Payload` tag points at stale `struct_tag_def_map` state.

## Suggested Next

Send Step 4 to plan-owner review. The bounded template record identity
propagation packets identified so far are now complete, and the next decision
is whether to advance to compatibility demotion or split any remaining
template-only semantic lookup work into a separate packet.

## Watchouts

Rendered template instantiation keys, direct-emission dedup sets,
`TypeSpec::tag`, and `struct_tag_def_map` mirror writes were intentionally
left unchanged and should stay compatibility/final-spelling/dedup state.
Direct template-emission reuse should not infer `record_def` from an existing
rendered map entry because that can be stale; producers should carry typed
identity from the selected/generated node or from already-typed `TypeSpec`
inputs.

This slice only covers direct substitution of a template type parameter into
`inst->base_types[bi]`. It intentionally leaves template instantiation keys,
template-base reconstruction, and rendered compatibility map writes unchanged.

## Proof

Delegated proof passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Additional local validation:

`git diff --check -- src/frontend/parser/impl/types/base.cpp tests/frontend/frontend_parser_tests.cpp todo.md test_after.log`

Proof log: `test_after.log`

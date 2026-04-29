# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Propagate Typed Record Identity Through Template Records

## Just Finished

Completed Step 4 substitution-copy propagation for direct template
instantiation payloads.

`types/base.cpp` now copies `TypeSpec::record_def` when a template type
parameter is substituted into cloned member typedefs, fields, method return
types, and method parameter types. Existing base/tag, pointer, reference, and
cv propagation behavior is otherwise preserved.

Focused parser coverage now instantiates a manually built `Box<T>` primary
with a record-backed `PayloadAlias` actual type and checks that the cloned
member typedef, field, method return, and method parameter all keep the
`Payload` `record_def`.

## Suggested Next

Run a final Step 4 inventory pass over the remaining template record identity
paths and decide whether any additional bounded template propagation packet is
needed before moving to compatibility demotion.

## Watchouts

Rendered template instantiation keys, direct-emission dedup sets,
`TypeSpec::tag`, and `struct_tag_def_map` mirror writes were intentionally
left unchanged. Direct template-emission reuse does not yet infer
`record_def` from an existing rendered map entry because that can be stale; it
only returns `record_def` when this producer creates the concrete node.

The new substitution coverage uses a hand-built parser fixture so the direct
instantiation clone path can inspect method payloads without depending on
record-body method parsing support.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Result: build completed; `frontend_parser_tests` and `frontend_hir_tests`
passed.

Proof log: `test_after.log`

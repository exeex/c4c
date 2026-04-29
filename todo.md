# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add TypeSpec Record Identity Payload

## Just Finished

Completed Step 2 first implementation packet. `TypeSpec` now has nullable
parser-owned record identity storage as `Node* record_def`, defaulting to null
under existing zero-initialization and aggregate-copy behavior.

Direct parser-owned record construction paths now populate `record_def` only
when they already hold a concrete `NK_STRUCT_DEF`: ordinary
`parse_base_type()` struct/union definitions and nested record member TypeSpecs
set the pointer when `sd->n_fields >= 0`. Tag-only or forward-reference record
TypeSpecs keep `record_def == nullptr`, and `TypeSpec::tag` spelling is left
unchanged.

Focused parser tests now prove direct struct and union TypeSpecs carry the same
record pointer registered in the compatibility tag map, while a tag-only
forward record TypeSpec keeps null typed identity.

## Suggested Next

Next bounded packet: add a parser-local record-resolution helper for
`const TypeSpec&` that returns `ts.record_def` first and falls back to
`struct_tag_def_map[ts.tag]` only for tag-only compatibility, then convert one
small consumer family to use it.

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

The payload is intentionally parser-owned; do not require HIR/LIR/backend
consumers to understand parser node pointers. Template-instantiated and alias
propagation paths may still produce null or copied `record_def` depending on
their source TypeSpec; convert those deliberately in later packets rather than
through broad incidental rewrites.

## Proof

Delegated proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Proof log: `test_after.log`

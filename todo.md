# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Convert Parser Record Consumers To Typed Identity

## Just Finished

Completed Step 3 support-layout packet. Added parser-local
`resolve_record_type_spec()` for `const TypeSpec&`, returning
`TypeSpec::record_def` first and falling back to `struct_tag_def_map[ts.tag]`
only when typed identity is absent.

Converted the parser support layout/constant-eval consumer family to route
record `TypeSpec`s through that helper: `field_align`, `struct_align`,
`struct_sizeof`, `compute_offsetof`, and both `eval_const_int` overloads now
prefer typed record identity for sizeof/alignof/offsetof while preserving the
tag-only compatibility fallback.

Focused parser tests now prove a stale rendered tag-map entry cannot override a
populated `record_def` for sizeof, alignof, or offsetof, and that tag-only
record TypeSpecs still resolve through the legacy compatibility map.

## Suggested Next

Next bounded packet: convert one additional parser-owned `struct_tag_def_map`
consumer family to use `TypeSpec::record_def` where the caller already has a
record `TypeSpec`, without changing HIR/LIR/backend contracts.

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

This packet intentionally changed only parser support layout helpers. Other
parser call sites that look up by rendered tag may still be legitimate
compatibility paths, especially where no `TypeSpec` is available.

## Proof

Delegated proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Proof log: `test_after.log`

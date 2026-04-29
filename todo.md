# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Convert Parser Record Consumers To Typed Identity

## Just Finished

Completed Step 3 declarations incomplete-object packet. Converted both
duplicated `is_incomplete_object_type` lambdas in `declarations.cpp` so direct
record `TypeSpec` values use `resolve_record_type_spec()` /
`TypeSpec::record_def` before falling back to `struct_tag_def_map` by rendered
tag.

Focused parser coverage now reaches both top-level and local declaration
incomplete-object checks through typedef-carried record identity. Each fixture
uses a complete populated `record_def` while the rendered `TypeSpec::tag`
points at an incomplete stale `struct_tag_def_map` entry, proving typed record
identity wins without parser hooks.

## Suggested Next

Next bounded packet: inventory and convert the `expressions.cpp` `NK_OFFSETOF`
constant-folding record lookup consumer, preserving rendered tag fallback for
tag-only `TypeSpec` values and adding focused stale-tag coverage if the path is
reachable without hooks.

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

This packet intentionally changed only the declarations incomplete-object
checks. Other parser call sites that look up by rendered tag may still be
legitimate compatibility paths, especially where no `TypeSpec` is available.

## Proof

Delegated proof passed after the declarations conversion:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Proof log: `test_after.log`

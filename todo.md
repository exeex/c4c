Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 first HIR consumer tightening for out-of-class struct method attachment and ordinary-function skip classification.

- AST-backed lookup confirmed `make_ns_qual` and `try_parse_qualified_struct_method_name` in `src/frontend/hir/hir_lowering_core.cpp`, plus `make_out_of_class_struct_method_lookup_key` and `attach_out_of_class_struct_method_defs` in `src/frontend/hir/hir_build.cpp`.
- `make_out_of_class_struct_method_lookup_key` now builds the owner key directly from complete `Node::qualifier_text_ids`, `namespace_context_id`, and `is_global_qualified`, with `Node::unqualified_text_id` as the method identity.
- The structured lookup result now distinguishes absent/incomplete metadata from complete structured metadata that misses owner or method lookup, so rendered `try_parse_qualified_struct_method_name` fallback runs only for absent/incomplete metadata.
- Complete structured misses do not attach through rendered fallback and do not lower as standalone functions.
- Focused HIR coverage now mutates `Node::name` and stale `qualifier_segments` independently from the `qualifier_text_ids`/owner key metadata, including complete-structured-miss cases for both attachment and ordinary-function skip behavior.

## Suggested Next

Next packet: inspect the remaining HIR consumers that still derive owner identity through `make_ns_qual`/rendered qualifier segment mirrors, starting with struct-definition owner registration and method lowering call sites.

Suggested boundary: classify where direct `Node::qualifier_text_ids` can safely replace re-interned `qualifier_segments`, and identify any remaining text-table ownership blocker before implementation.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; this slice keeps it only after structured out-of-class metadata is absent or incomplete, not after complete structured misses.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.

## Proof

Ran the delegated proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

Result: passed.

Supervisor-side regression guard passed with `--allow-non-decreasing-passed`.

Result: 2/2 passed before and 2/2 passed after.

Log state: `test_after.log` was rolled forward to `test_before.log`.

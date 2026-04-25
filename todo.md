# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Add Sema Structured Key Helpers

## Just Finished

Step 1 - Add Sema Structured Key Helpers: inspected sema's current string-keyed
lookup paths in `validate.cpp` and added behavior-preserving structured name
helper APIs in `validate.hpp`/`validate.cpp`.

The new helpers derive local and namespace/qualified sema keys from existing
parser metadata on `Node` (`unqualified_text_id`, `qualifier_text_ids`,
`is_global_qualified`, and `namespace_context_id`) and add presence/pointer
comparison helpers for future dual-read checks. No existing string-keyed lookup
or diagnostics path was rewired in this packet.

## Suggested Next

Delegate Step 2 to add dual local-scope symbol maps using `sema_local_name_key`
where declaration/reference nodes expose stable `unqualified_text_id`, while
leaving the current string maps as the authoritative behavior path.

## Watchouts

- Do not require HIR data-model cleanup in this plan.
- Do not remove `Node::name`, `TypeSpec::tag`, rendered diagnostics, mangled
  names, or link-name surfaces.
- Do not add testcase-shaped special cases or expectation downgrades.
- Structured key derivation intentionally returns no key when
  `unqualified_text_id` is missing, qualifier metadata is malformed, or
  qualifier segments lack valid `TextId`s.
- `TypeSpec::tag` and `TypeSpec::qualifier_segments` do not currently carry
  base-name `TextId` metadata, so this packet did not add a TypeSpec tag key
  helper.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_' > test_after.log 2>&1`

Result: passed. `test_after.log` records 34/34 `positive_sema_` tests passing.

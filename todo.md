# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Record Identity Through TypeSpec

## Just Finished

Lifecycle switched from parser cleanup idea 123 to typed record identity bridge
idea 127 after Step 3 identified that `struct_tag_def_map` cannot be converted
honestly while `TypeSpec` carries record identity only as `const char* tag`.

## Suggested Next

Execute Step 1 by inventorying `TypeSpec::tag` producers and consumers, then
choose the smallest typed record identity representation and the first bounded
implementation packet.

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

## Proof

Lifecycle-only switch. Validation run:
- `git diff --check -- plan.md todo.md ideas/open/123_parser_legacy_string_lookup_removal_convergence.md`
- `git diff --check --no-index /dev/null ideas/open/127_typed_parser_record_identity_bridge.md`

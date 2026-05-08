Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Constructor And Special-Member Classification

# Current Packet

## Just Finished

Completed Step 3 local classification inventory after the carrier migration in `src/frontend/parser/impl/declarations.cpp`.

- Re-inspected the `parse_top_level` special-member probe, consume, qualified operator, and qualified constructor paths.
- Found no remaining local special-member classification decision that compares rendered owner strings after the Step 3 carrier migration.
- Confirmed `probe_special_member_owner` and `consume_special_member_owner` use component `TextId` equality for constructor terminal matching.
- Classified retained rendered strings as non-local or compatibility/display: `qualified_owner` still feeds legacy function names and template-owner lookup; `qualified_op_name` and `qualified_ctor_name` remain emitted function-name mirrors; `qualified_owner_tag` belongs to the generic declarator owner-scope path and was not touched.

## Suggested Next

Next packet: start Step 4 by inspecting the narrow Sema/HIR consumers reached by `attach_out_of_class_method_owner` and `make_out_of_class_method_qn`, and classify any remaining rendered owner lookup as structured carrier use, temporary compatibility, or semantic authority.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- This packet uses `TextId` equality only for the local owner/member component match; compound owner meaning still lives in the owner segment sequence and qualified-name metadata.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- `consume_special_member_owner` still computes component `TextId`s locally only for token-stop validation; it no longer exports owner metadata to the branches.
- Generic declarator owner lookup through `decl_qn`, `qualified_owner_tag`, and template primary lookup remains outside this Step 3 packet.
- No tests were changed in this packet.

## Proof

Inventory-only packet; no implementation change was made and no build was required.

No proof command was run, per delegated instructions for no-code inventory.

Log state: no `test_after.log` update was made.

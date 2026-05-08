Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Constructor And Special-Member Classification

# Current Packet

## Just Finished

Completed Step 3 qualified operator/constructor migration packet in `src/frontend/parser/impl/declarations.cpp`.

- Made the qualified operator branch source `qualified_owner_text_ids`, `qualified_owner_segments`, and global qualification from `SpecialMemberOwnerProbe`.
- Made the qualified constructor branch source owner metadata from `SpecialMemberOwnerProbe` and use the probe terminal member `TextId`/name when available.
- Tightened `consume_special_member_owner` so it only advances and validates parser state, instead of rebuilding owner metadata through output parameters.
- Kept rendered `qualified_owner`, `qualified_op_name`, and `qualified_ctor_name` as legacy display/compatibility mirrors.

## Suggested Next

Next packet: inventory and migrate any remaining local special-member classification checks in `parse_top_level` that still use rendered owner strings, without touching generic declarator owner lookup or Sema/HIR.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- This packet uses `TextId` equality only for the local owner/member component match; compound owner meaning still lives in the owner segment sequence and qualified-name metadata.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- `consume_special_member_owner` still computes component `TextId`s locally only for token-stop validation; it no longer exports owner metadata to the branches.
- No tests were changed in this packet.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_parser_(core|declarations|declarator)|cpp_hir_.*member_owner|cpp_hir_expr_operator_member') > test_after.log 2>&1`.

Supervisor-side regression guard passed with `--allow-non-decreasing-passed` using matching before and after logs.

Result: 14/14 passed before and 14/14 passed after.

Log state: `test_after.log` was rolled forward to `test_before.log`.

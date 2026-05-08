Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Owner-Probe Carrier

# Current Packet

## Just Finished

Completed Step 2 first implementation packet in `src/frontend/parser/impl/declarations.cpp`.

- Added a local `SpecialMemberOwnerProbe` carrier for the special-member owner probe with owner segment `TextId`s, fallback spellings, global qualification, and terminal member `TextId`.
- Replaced the rendered `next_name == seg` constructor probe decision with `TextId` component equality.
- Replaced the `parser.token_spelling(parser.peek(1)) == seg` consume stop decision with `TextId` component equality.
- Kept `qualified_owner`, `ctor_name`, and `qualified_ctor_name` as legacy display/compatibility mirrors for existing function names and template-owner lookup.

## Suggested Next

Next packet: migrate the remaining special-member classification decisions in this local parser path that still depend on flattened owner rendering, starting with destructor/conversion/operator-member shape checks before touching Sema/HIR consumers.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- This packet uses `TextId` equality only for the local owner/member component match; compound owner meaning still lives in the owner segment sequence and qualified-name metadata.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- Existing frontend coverage already includes global out-of-class constructor/operator structured-key tests, so no new test was added in this packet.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_cxx_') > test_after.log 2>&1`.

Result: passed.

Supervisor-side acceptance checks:
- Regression guard passed with `--allow-non-decreasing-passed`.
- `test_after.log` was rolled forward to `test_before.log`.
- Broader focused validation passed 14/14 with `ctest --test-dir build -j --output-on-failure -R 'cpp_hir_parser_(core|declarations|declarator)|cpp_hir_.*member_owner|cpp_hir_expr_operator_member'`.

Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Owner String Authority

# Current Packet

## Just Finished

Completed Step 1 inventory for rendered owner string authority in the parser out-of-class owner-probe path.

Classification:

| Site | Names | Classification | Notes |
| --- | --- | --- | --- |
| `src/frontend/parser/impl/declarations.cpp:2705` `probe_special_member_owner` | `qualified_owner`, `owner_base_name`, `token_spelling(...)` | semantic authority | Builds rendered `owner` segments while probing and uses rendered `next_name == seg` to decide qualified constructor shape. It also returns rendered `qualified_owner` as the owner string that later names constructors/operators and drives template-owner lookup. |
| `src/frontend/parser/impl/declarations.cpp:2775` `consume_special_member_owner` | `out_owner_text_ids`, `out_owner_segments`, `token_spelling(...)` | mixed structured carrier plus semantic authority | Collects structured owner `TextId` segments and fallback strings for later owner metadata, but the constructor stop condition still compares `parser.token_spelling(parser.peek(1)) == seg`. Operator stop uses token kind only and is syntax handling. |
| `src/frontend/parser/impl/declarations.cpp:2835` qualified operator branch | `qualified_owner`, `qualified_owner_segments`, `qualified_owner_text_ids` | compatibility/display plus semantic authority | Structured owner data is used for `set_current_struct_tag_from_unqualified_owner`, `attach_out_of_class_method_owner`, and `QualifiedNameKey` registration. Rendered `qualified_owner` still builds `fn->name` through `qualified_op_name` and drives `find_template_struct_primary(... parser_text_id_for_token(kInvalidText, qualified_owner))` plus `owner_struct_tag`. |
| `src/frontend/parser/impl/declarations.cpp:3014` qualified constructor branch | `qualified_owner`, `ctor_name`, `qualified_ctor_name`, `qualified_owner_segments`, `qualified_owner_text_ids` | compatibility/display plus semantic authority | `ctor_name` spelling becomes the function display/legacy base and `qualified_ctor_name` becomes `fn->name`; structured owner data is attached through `make_out_of_class_method_qn`. Rendered `qualified_owner` still drives template-owner lookup and `owner_struct_tag`. |
| `src/frontend/parser/impl/declarations.cpp:3083` constructor initializer member names | `token_spelling(...)` | unrelated | Captures member-initializer spelling after constructor classification; it does not decide owner identity. |
| `src/frontend/parser/impl/declarations.cpp:3615` generic qualified declarator owner scope | `qualified_owner_tag`, `decl_name`, `decl_qn` | semantic authority gate plus structured carrier | `decl_name` is split with `rfind("::")` to decide whether to enter owner scope and to render `qualified_owner_tag`; then structured `decl_qn.qualifier_text_ids` and `decl_qn.qualifier_segments` set current owner. Template-owner lookup still uses rendered `qualified_owner_tag`. This can influence ordinary out-of-class member-owner decisions. |
| `src/frontend/parser/impl/types/declarator.cpp:2289` qualified declarator parsing | `token_spelling(...)`, `parse_operator_declarator_name`, `out_qn` | syntax handling plus structured carrier | Builds rendered `qualified_name` for legacy name output while also filling `QualifiedNameRef` segments/text IDs. Operator names are canonical parser spellings, not owner authority by themselves. |
| `src/frontend/parser/impl/types/declarator.cpp:2367` destructor from owner type | `token_spelling(...)`, `qualified_name_text(...)` | compatibility/display | Uses structured `owner_type`/`QualifiedNameRef` to form destructor metadata, then renders `owner_name` and destructor text for legacy `out_name`. No rendered owner comparison was found in this path. |
| `src/frontend/parser/impl/core.cpp:3234` qualified-name carriers | `peek_qualified_name`, `parse_qualified_name`, `qualified_name_text(...)` | structured metadata with display rendering | `QualifiedNameRef` already carries global qualification, qualifier segments, qualifier text IDs, base spelling, and base text ID. `qualified_name_text(...)` is the compatibility renderer and should not become the decision key. |

## Suggested Next

First implementation packet: replace the special-member owner probe/consume constructor matching in `src/frontend/parser/impl/declarations.cpp` with a structured local probe result that carries owner segment `TextId`s, fallback spellings, global qualification, and terminal member `TextId`; use `TextId` equality for constructor owner/member matching and keep rendered `qualified_owner`/`qualified_ctor_name` only as legacy display mirrors.

Owned files for first packet: `src/frontend/parser/impl/declarations.cpp`; add `tests/frontend/frontend_parser_tests.cpp` only if existing frontend coverage does not exercise the changed constructor/operator owner probe path.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Do not claim progress by renaming `qualified_owner` while preserving rendered-string decision authority.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- The first code packet should remove the rendered `next_name == seg` / `token_spelling(peek(1)) == seg` constructor checks before attempting broader HIR or Sema threading.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.

## Proof

Inventory-only slice; no build required and no `test_after.log` update made.

First implementation packet proof command: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_cxx_'`

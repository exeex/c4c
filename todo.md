Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect parser alias/template tables and active-context fields by true identity kind

# Current Packet
## Just Finished
Completed the Step 1 inspection for parser alias/template identity. The concrete split is: `ParserTemplateState::alias_template_info` and the `template_struct_*_by_key` tables are semantic identity; the `template_struct_defs` / `template_struct_specializations` string maps are legacy bridge fallback; `last_using_alias_name`, `last_resolved_typedef`, and `current_struct_tag` are active-context copies whose `TextId` fields carry the semantic anchor; parse/debug strings stay diagnostics-only.

## Suggested Next
Use `QualifiedNameKey` through `alias_template_key_in_context(...)` as the first structured key family for Step 2, then retarget alias-template registration and lookup to that semantic path before touching any broader template-owned string maps.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Do not widen into a repo-wide identity migration.
- Leave rendered-name recovery in place only as explicit fallback bridge behavior.
- The template struct string maps still matter as compatibility fallbacks, but they are not the semantic source of truth.

## Proof
Ran `bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log'`.
`test_after.log` is preserved at the repo root.

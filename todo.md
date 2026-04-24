Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce a structured key path for alias-template registration and lookup

# Current Packet
## Just Finished
Completed Step 2 for alias-template registration. The `using Alias = ...` handoff now records alias-template metadata from the structured `QualifiedNameKey` path by recovering the just-registered typedef through `find_structured_typedef_type(alias_key)`, rather than reading back the rendered alias spelling copy.

## Suggested Next
Tighten alias-template application lookup in the parser type resolution path so it prefers the structured alias key before any rendered-name fallback, without widening into the broader `template_struct_*` string-map cleanup.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Do not widen into a repo-wide identity migration.
- Treat the legacy string-keyed typedef bindings as bridge fallback only.
- The template struct string maps still matter as compatibility fallbacks, but they are not the semantic source of truth.

## Proof
Ran `bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log'`.
`test_after.log` is preserved at the repo root.

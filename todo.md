Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget template-owned member and type follow-through away from spelling recovery where structured identity already exists

# Current Packet
## Just Finished
Completed Step 3 for template-owned member/type follow-through. Alias-template application now probes the structured alias key before any rendered-name recovery, and qualified member typedef follow-through now checks structured identity first before dropping to spelling-based lookup.

## Suggested Next
Retarget the remaining `template_struct_*` string-map bridges in parser template primary/specialization lookup so the structured-key path stays semantic and the spelling-based recovery remains bridge-only.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Do not widen into a repo-wide identity migration.
- Treat the legacy string-keyed typedef bindings as bridge fallback only.
- The template struct string maps still matter as compatibility fallbacks, but they are not the semantic source of truth.

## Proof
Ran `bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log'` successfully.
`test_after.log` is preserved at the repo root.

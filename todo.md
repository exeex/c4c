# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Completed Step 1 deletion-boundary reprobe.

The final known direct fixture blockers were migrated, including
`tests/frontend/cpp_hir_value_args_residual_metadata_test.cpp`. That target now
uses a SFINAE-gated helper for the transitional `.tag` fixture setup while
keeping the four structured metadata assertions active and unchanged.

Temporarily removed `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp` and ran
`cmake --build build --target cpp_hir_value_args_residual_metadata_test`.
The owned target built successfully with the field removed, so no remaining
deletion-probe blocker was found inside this owned target. Restored the field
before final proof, leaving no `ast.hpp` diff.

Supervisor broad deletion probe 16 then temporarily removed `const char* tag`
from `src/frontend/parser/ast.hpp` and ran `cmake --build build`. The full
tree built successfully with the field removed. The field was restored
afterward, leaving no `ast.hpp` diff.

Lifecycle decision: Step 1 is complete. The source idea is not complete yet
because `TypeSpec::tag` still exists in the checked-in tree; continue with Step
6 permanent deletion and validation.

## Suggested Next

Delegate Step 6: permanently delete `const char* tag` from `TypeSpec`, remove
any now-obsolete compatibility shims that only preserved the field, and run the
supervisor-selected build and focused validation. Start with
`cmake --build build --target c4cll`, then escalate to the broader validation
the supervisor selects once focused proof is green.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- This owned test still has a SFINAE helper reference to `.tag` by design.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target cpp_hir_value_args_residual_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_value_args_residual_structured_metadata$'`

Result: passed.
`cpp_hir_value_args_residual_structured_metadata` is green in
`test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran
  `cmake --build build --target cpp_hir_value_args_residual_metadata_test`;
  the focused target built successfully with the field removed, so no next
  blocker appeared inside this target.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.

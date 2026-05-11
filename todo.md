Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
HIR struct member-symbol rendered lookup is now explicitly fenced as no-owner
rendered compatibility while preserving `HirStructMemberLookupKey` authority,
structured field member-symbol ids, and closed complete record_def/text-key
misses.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/impl/lowerer.hpp` | Documented member-symbol rendered tag/member lookup as no-owner compatibility and `struct_member_symbol_ids_by_owner_` plus field ids as authority. |
| `src/frontend/hir/hir_types.cpp` | Annotated the no-owner rendered member-symbol fallback, field-id authority path, and owner-key parity-only rendered comparison. |
| `tests/frontend/frontend_hir_tests.cpp` | Renamed focused member-symbol tests/messages to no-owner rendered compatibility and added field member-id authority coverage after an owner-map miss. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | No changes needed; existing lookup coverage remains focused on adjacent static/member and rendered-bridge families. |
| `todo.md` | Recorded this executor packet and proof. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Continue Step 4 with the next rendered-qualified/no-owner handoff family after
the completed `struct_defs`, declaration `fn_index`/`global_index`,
`template_defs`, static-member declaration/const compatibility, and
member-symbol compatibility slices.

## Watchouts

- `struct_member_symbol_ids_by_owner_` and `HirStructDef::fields[*].member_symbol_id`
  are the member-symbol authority; rendered `module_->member_symbols` lookup is
  retained only for no-owner compatibility.
- Complete member-symbol record_def/text-key misses still fail closed before
  rendered owner tag lookup, while structured base lookup remains intentional.
- `template_defs` remains intentionally rendered-name keyed for HIR
  preservation/bookkeeping; do not treat it as AST template-definition
  authority.
- CompileTimeState `has_template_def`/`find_template_def` remains the
  authoritative structured/template definition path for deferred instantiation.
- The retained declaration rendered fallback is still intentionally available
  for missing/incomplete metadata and for the named rendered-qualified or
  self-consistent compatibility predicates after a structured miss.
- Complete structured declaration-key misses remain closed before
  `fn_index`/`global_index` unless those named compatibility predicates allow
  the rendered bridge.
- Keep HIR `FunctionCtx` local/label/generated-name cleanup out of this plan;
  idea 169 owns route-local identity domains.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Proof command, logged to `test_after.log`:

`cmake --build build --target frontend_hir_tests frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$'`

Result: passed; 2/2 focused tests passed (`frontend_hir_tests`,
`frontend_hir_lookup_tests`).

Additional check: `git diff --check` passed.

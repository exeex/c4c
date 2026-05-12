Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Type-Binding And NTTP Bridges

# Current Packet

## Just Finished

Step 3 fenced the remaining localized Sema NTTP bridges for forwarded and
default compatibility without broad HIR cleanup. Retained rendered NTTP maps
and call-env assignments now have nearby legacy/deprecated owner, limitation,
and removal-condition notes.

`cpp_hir_sema_consteval_type_utils_structured_metadata` now covers default NTTP
call-env output as a metadata-rich path: TextId metadata resolves the default,
a complete stale TextId miss cannot recover through the rendered mirror, and
no-metadata callers still retain the rendered compatibility bridge.

## Suggested Next

Continue Step 3 by having the supervisor/reviewer decide whether the
type-binding and NTTP bridge fencing is sufficient for this runbook step, or
whether one more bounded Sema-only bridge remains before moving on.

## Watchouts

- The rendered enum bridge is intentionally retained because HIR/static-member
  no-metadata callers still opt in through the named compatibility API.
- Type-binding and NTTP rendered names may remain as display/source payload or
  explicit no-metadata compatibility; they must not act as semantic authority
  after a complete structured miss.
- `TypeSpec` in this build has no legacy `tag` field, so tests cannot
  manufacture a rendered typedef display tag through
  `set_legacy_tag_if_present`; rely on existing key/TextId stale-rendered
  coverage for this ABI and avoid adding impossible rendered-tag assertions.
- Forwarded/default NTTP rendered maps are now covered in the focused metadata
  test; remaining Step 3 work should avoid broad HIR cleanup unless assigned.
- Do not broaden Step 3 into HIR lowerer cleanup unless the supervisor assigns
  that scope; the current plan is Sema-owned compatibility retirement.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_parser_lookup_authority_tests)$"' > test_after.log 2>&1`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.

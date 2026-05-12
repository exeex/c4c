Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Consteval Function And Local Const Lookup

# Current Packet

## Just Finished

Step 4 local-const lookup fencing is complete for this packet. Retained
rendered local-const lookup in `ConstEvalEnv::lookup(std::string)` and the
rendered local branch in `lookup_rendered_compatibility` now carry nearby
legacy/deprecated owner, limitation, and removal-condition notes. The existing
interpreter `by_name` mirror note remains the fenced no-metadata compatibility
boundary for local bindings.

`cpp_hir_sema_consteval_type_utils_structured_metadata` now has focused direct
local-const proof: key metadata beats stale rendered locals, TextId metadata
beats stale rendered locals, complete local metadata misses do not recover
through rendered local maps, and no-metadata local lookups keep rendered
compatibility.

## Suggested Next

Continue Step 4 with any remaining consteval function rendered-lookup fencing
review the supervisor wants, or hand Step 4 to reviewer scrutiny if the current
function/local-const proof set is considered complete.

## Watchouts

- The rendered enum bridge remains intentionally retained because HIR/static-
  member no-metadata callers still opt in through the named compatibility API.
- Type-binding and NTTP rendered names may remain as display/source payload or
  explicit no-metadata compatibility; they must not act as semantic authority
  after a complete structured miss.
- Step 4 should stay Sema-owned. Do not broaden into parser, HIR lowerer, BIR,
  LIR, or backend cleanup unless the supervisor assigns that scope.
- Retained local-const rendered lookup now has the requested nearby
  legacy/deprecated notes; future edits should preserve the invariant that a
  local TextId/key metadata miss sets `skip_local` and cannot fall through to
  `local_consts` or interpreter `by_name`.
- A metadata-rich miss must not fall through to rendered `consteval_fns`.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_parser_tests)$"' > test_after.log 2>&1`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.

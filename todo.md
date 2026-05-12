Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Consteval Function And Local Const Lookup

# Current Packet

## Just Finished

Step 3 is complete. It fenced the remaining localized Sema NTTP bridges for
forwarded and default compatibility without broad HIR cleanup. Retained
rendered NTTP maps and call-env assignments now have nearby legacy/deprecated
owner, limitation, and removal-condition notes.

`cpp_hir_sema_consteval_type_utils_structured_metadata` now covers default NTTP
call-env output as a metadata-rich path: TextId metadata resolves the default,
a complete stale TextId miss cannot recover through the rendered mirror, and
no-metadata callers still retain the rendered compatibility bridge.

## Suggested Next

Start Step 4 by inventorying the rendered `consteval_fns` compatibility lookup
after structured consteval metadata checks and the interpreter `by_name` local
const compatibility mirrors.

Fence one narrow metadata-rich consteval function or local-const route first:
prefer structured binding metadata or route-local identity when present, fail
closed after a complete structured miss, and retain rendered lookup only behind
an explicit legacy/no-metadata compatibility boundary with a concrete removal
condition.

Add focused stale rendered consteval or local-const proof for the converted or
fenced route.

## Watchouts

- The rendered enum bridge remains intentionally retained because HIR/static-
  member no-metadata callers still opt in through the named compatibility API.
- Type-binding and NTTP rendered names may remain as display/source payload or
  explicit no-metadata compatibility; they must not act as semantic authority
  after a complete structured miss.
- Step 4 should stay Sema-owned. Do not broaden into parser, HIR lowerer, BIR,
  LIR, or backend cleanup unless the supervisor assigns that scope.
- Retained consteval function and local-const rendered lookup must get nearby
  legacy/deprecated owner, limitation, and removal-condition notes.
- A metadata-rich miss must not fall through to rendered `consteval_fns` or
  interpreter `by_name` mirrors.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_parser_lookup_authority_tests)$"' > test_after.log 2>&1`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.

No Step 4 proof has run yet.

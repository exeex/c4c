Status: Active
Source Idea Path: ideas/open/159_sema_consteval_domain_table_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make template type and NTTP binding lookup structured-first

# Current Packet

## Just Finished

Completed Step 4, "Make template type and NTTP binding lookup
structured-first." `resolve_type` now routes legacy rendered TypeSpec names
through structured/TextId type binding bridge maps before rendered fallback and
treats bridge misses as authoritative. Forwarded consteval NTTP lookup now
reports `nttp_binding_metadata_miss` on TextId/key metadata misses and keeps
rendered NTTP lookup available only for no-carrier compatibility. Added focused
`frontend_parser_tests` coverage for forwarded NTTP TextId miss rejection,
matching TextId success, and no-metadata rendered compatibility.

## Suggested Next

Start Step 5 from `plan.md`: make record-layout lookup prefer owner keys and
fail closed on covered owner metadata misses.

## Watchouts

- The current `TypeSpec` shape has no rendered `tag` field in this build, so
  rendered-name type bridge behavior is covered through active TextId/key
  carriers rather than a synthetic stale `TypeSpec::tag` fixture.
- A local exploratory run from the prior packet found
  `frontend_parser_lookup_authority_tests` red for an unrelated value-domain
  assertion; this packet used the delegated proof target instead.
- Do not weaken tests, mark supported paths unsupported, or rely on
  testcase-shaped shortcuts.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_lookup_value_structured_metadata|positive_sema_)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and 36/36
passing delegated tests.

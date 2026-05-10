Status: Active
Source Idea Path: ideas/open/159_sema_consteval_domain_table_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Make record-layout lookup prefer owner keys

# Current Packet

## Just Finished

Completed Step 5, "Make record-layout lookup prefer owner keys." Consteval
record-layout lookup now treats a complete owner-key miss with
`struct_def_owner_index` present as authoritative, so stale rendered record tags
cannot reopen layout lookup for covered `sizeof`/`alignof` paths. Added focused
coverage where the stale rendered tag is present in `link_name_texts` and the
owner index, proving both successful owner-key layout lookup and owner-key miss
failure for `sizeof` and `alignof`.

## Suggested Next

Start Step 6 from `plan.md`: consolidate retained rendered-bridge
documentation and run the final validation ladder for the active consteval
authority migration.

## Watchouts

- The `link_name_texts` canonicalization bridge is still available only after
  `record_owner_key_from_typespec` cannot form a complete owner key; complete
  owner-key misses fail closed before that bridge.
- A local exploratory run from an earlier packet found
  `frontend_parser_lookup_authority_tests` red for an unrelated value-domain
  assertion; this packet used the delegated proof target instead.
- Do not weaken tests, mark supported paths unsupported, or rely on
  testcase-shaped shortcuts.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_parser_tests|positive_sema_)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and 36/36
passing delegated tests.

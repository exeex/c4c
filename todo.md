# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Continued Step 3.3 by materially narrowing the Sema global branch in
`lookup_symbol()`: a rendered global compatibility hit no longer wins when the
reference carries qualifier metadata, the rendered global has structured
metadata, and the reference's structured/unqualified base name conflicts with
the terminal rendered global spelling after the structured global key misses.

Added focused stale qualified-global coverage in
`frontend_parser_lookup_authority_tests`: a reference whose rendered spelling is
`RenderedNs::stale` but whose structured/unqualified base is `missing` must not
recover the `RenderedNs::stale` global through rendered compatibility after the
structured key miss.

## Suggested Next

Continue Step 3.3 by reviewing the remaining Sema `lookup_symbol()` rendered
compatibility routes, especially enum constants and local/global cases where the
reference still lacks complete qualifier or namespace producer metadata.

## Watchouts

- This packet intentionally preserves the existing qualified rendered global
  compatibility needed by positive cases such as `lib::seed` and anonymous
  namespace globals, where Sema still receives a structured key that misses even
  though the reference base spelling matches the rendered declaration.
- `c4c-clang-tool-ccdb` was available on `PATH`, but
  `build/compile_commands.json` did not load `src/frontend/sema/validate.cpp`;
  targeted source reads were used after that tooling miss.
- The static-member template-instantiation optimistic route noted in the prior
  packet remains unchanged.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 884/884
passing delegated positive Sema tests.

Additional check: `cmake --build --preset default --target frontend_parser_lookup_authority_tests && build/tests/frontend/frontend_parser_lookup_authority_tests`

Result: passed, including the new stale qualified-global rendered lookup
authority coverage.

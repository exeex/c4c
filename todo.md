# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Completed Step 3.2 consteval function-body value fallback deletion. The final
rendered-name NTTP recovery is now blocked when populated local/parameter/loop
metadata records an authoritative structured-key or `TextId` miss and NTTP
metadata exists for the call.

Updated `consteval_local_binding_metadata.cpp` with a mixed local + NTTP
consteval case so direct NTTP metadata/producer-incomplete compatibility stays
covered while the rendered local-map predicate remains absent.

## Suggested Next

Execute Step 3.3 with one focused Sema owner/member/static rendered-route
packet outside the consteval value/type metadata route.

Recommended first packet:

- Inventory the live Sema owner/member/static semantic lookup helpers that
  still consult rendered names, especially `fallback`, `legacy`,
  `compatibility`, `lookup_struct_static_member_*`, instance-field legacy
  probes, owner-by-rendered-name fallback, and rendered global/enum
  compatibility indexes.
- Select the smallest route where the call site already has an owner key, AST
  node, declaration, `TextId`, or structured map carrier.
- Delete or collapse that rendered semantic route to the structured/domain-key
  API. Do not keep a string/string_view overload beside the structured route.
- Add focused same-feature Sema coverage proving structured metadata wins over
  stale or disagreeing rendered spelling.
- Produce a fresh build plus narrow Sema/frontend proof in canonical
  `test_after.log`.

## Watchouts

- The earlier packet only deleted the intrinsic TypeSpec-key miss fallback in
  `resolve_type()`. It did not remove the structured-name, text-id,
  rendered-name, or legacy type-binding mirror routes for TypeSpecs that do not
  carry intrinsic template-parameter metadata.
- Do not use `has_rendered_local_binding()`, rendered local maps, or an
  equivalent rendered-name predicate as the authority boundary for local
  metadata misses.
- The `local_binding_metadata_miss` deletion is intentionally limited to final
  rendered-name NTTP recovery. Direct structured/TextId NTTP hits still win, and
  producer-incomplete routes with no NTTP metadata remain compatible.
- Step 3.3 is outside the consteval value/type metadata producer route. Do not
  reopen Step 3.1/3.2 unless the selected Sema owner/member/static path depends
  on that same metadata boundary.
- Keep no-metadata rendered compatibility only when the exact missing producer
  is recorded here or split into a separate open idea.
- Treat helper renames, wrapper extraction, local rendered-key reconstruction,
  fallback/legacy relabeling, and expectation downgrades as non-progress.
- HIR-owned metadata carriers and `src/frontend/hir/*` remain untouched and out
  of scope.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '(^cpp_positive_sema_.*consteval.*_cpp$|^cpp_negative_tests_.*consteval)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 29/29
passing delegated consteval positive/negative tests, including
`cpp_positive_sema_consteval_local_binding_metadata_cpp` and
`cpp_positive_sema_consteval_nttp_cpp`.

# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Step 3.1 repaired the parser parameter metadata producer: `parse_param` now
copies the declarator identifier `TextId` and unqualified source name onto the
returned `NK_DECL` without adding namespace qualifiers. Focused consteval parser
tests cover parsed function parameters and show parameter binding/readback still
uses `TextId` metadata when rendered parameter/reference names are stale.

## Suggested Next

Continue Step 3.1 with one bounded metadata-producer packet for consteval
locals, loop locals, synthetic locals, or `TypeSpec` intrinsic identifier
metadata before attempting any consteval rendered-fallback deletion.

## Watchouts

- Do not treat the previous consteval value/type fallback slice as closed while
  same-spelling rendered fallback can still decide lookup after populated
  metadata misses.
- Same-spelling consteval local/loop compatibility remains a metadata-producer
  target, not acceptable lookup-deletion progress.
- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea before treating it as parser/Sema closure work.
- Step 3 must not count diagnostics, display, mangle/final spelling, or
  comment-only classification as semantic lookup removal.
- Namespace-qualified rendered bridges and synthetic locals without structured
  metadata remain compatibility candidates; do not collapse those routes until
  their producers carry equivalent structured metadata.
- Consteval `TypeSpec` resolution still depends on the recorded rendered-name
  to structured/`TextId` mirror maps to find metadata for a `TypeSpec` tag;
  Step 3.1 owns either adding intrinsic `TypeSpec` identifier metadata for a
  bounded route or parking the exact missing producer.
- Consteval interpreter locals still need same-spelling rendered compatibility
  because some existing parameter/loop-local references do not have complete
  matching local `TextId`/structured mirrors. Treat removal of that compatibility
  as a metadata-producer packet, not a lookup-only packet.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.

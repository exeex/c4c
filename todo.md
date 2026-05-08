Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Focused Owner-Probe Tests Or Probes

# Current Packet

## Just Finished

Completed Step 5 focused owner-probe coverage for `Add Focused Owner-Probe Tests Or Probes`.

- Added parser coverage for `Outer::Inner::method` showing the parsed out-of-class method carries owner segment `TextId`s for both `Outer` and `Inner`; the test then drifts rendered owner spelling and validates through Sema to prove the structured segment sequence, not flattened rendered owner text, remains authoritative.
- Added HIR consumer coverage for nested out-of-class method attachment where the structured owner key names `RealOuter::Inner::run` while the rendered function name would match a stale `StaleOuter::Inner::run` fallback; the probe asserts only the structured pending method is captured.

## Suggested Next

Next packet: begin Step 6 by inventorying remaining rendered-owner compatibility routes and deciding which ones can be retired versus documented as temporary compatibility.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; it is now kept only after structured out-of-class metadata is absent or incomplete in both attached-method and non-method lowering.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.
- The AST-node owner-key helpers now treat available spelling carriers as the cross-table canonicalization path for parser-owned ids; consumers without a reliable spelling carrier still need a separate route.
- The no-module `typespec_aggregate_owner_key(TypeSpec)` path remains a structured parser-id fallback because it has no link-name table for cross-table canonicalization.
- This slice does not canonicalize member typedef `tag_text_id` itself; the touched tests keep member ids canonical and isolate owner-key behavior.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.
- `make_out_of_class_struct_method_lookup_key` lives in `hir_build.cpp`; the initial owned-file list named `hir_lowering_core.cpp`, but that draft is not the active consumer for this regression.
- `pr44164.c` still exposes frontend/HIR metadata drift: `struct Y` can exist in `struct_defs` without being listed in `struct_def_order`, and some member expressions carry stale rendered owner tags. This packet handles those facts in LIR consumers without parser changes.
- The const-init active-layout guard is a recursion safety valve; it should not be used to justify accepting future owner/layout cycles when exact metadata is available.
- The `member_symbol_id` recovery intentionally accepts only a unique layout hit.
- The stale rendered qualified-name test covers the non-method path by leaving the structured owner present but the structured method key absent; rendered method maps remain populated to prove the fallback is gated.
- Nearby same-feature behavior considered in Step 5: the new parser probe covers nested out-of-class methods reached from parsed owner-probe metadata, and the HIR probe covers stale same-suffix nested owner ambiguity at the consumer boundary. Constructors, destructors, and operators remain covered by existing owner-authority tests and should not be weakened while Step 6 inventories remaining compatibility routes.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

Proof log: `test_after.log`.

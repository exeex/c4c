Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Replace remaining suitable single-name string tables and isolate holdouts

# Current Packet

## Just Finished
Completed the next plan step 5 cleanup slice for template prelude bookkeeping.
Converted injected template-typedef cleanup state from raw `std::string`
tracking to `TextId` plus bridge spelling, added `TextId`-aware
register/unregister helpers for synthesized typedef bindings, and kept
record-member template prelude scope injection on the same semantic ids used
by active template-scope lookup. Added a focused parser unit test that proves
synthetic typedef cleanup can unregister through the semantic `TextId` even
when fallback spelling is corrupted.

## Suggested Next
Continue step 5 by auditing the remaining compatibility-only parser surfaces
that still retain single-name spelling alongside semantic identity and decide
whether any touched lexical lookup path still depends on flat strings after
this template-prelude cleanup. `enum_consts` and `const_int_bindings` are
already `TextId`-native, so the next packet should either isolate a real
lexical holdout or explicitly mark the remaining string carriers as
compatibility-only. The last proof was still narrow, so the next execution
packet should widen focused parser/frontend coverage before more route work.

## Watchouts
Keep lexical concept visibility separate from namespace traversal. Do not
reopen the qualified-owner lookup slice completed under idea 84. Qualified
concept names should stay on structured namespace-context keys rather than
reintroducing rendered-name sets, and unqualified lexical concept checks
should keep using `TextId`-native identity for the touched parser paths.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_record_member_template_friend_cleanup_dump)$'`
with proof captured in `test_after.log`.

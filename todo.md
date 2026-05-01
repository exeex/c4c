# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3.3A
Current Step Title: Carry Parser Using-Value-Alias Authority Into Sema

## Just Finished

Step 3.3.3 narrow global fallback probe: attempted to make qualified global
references reject an unqualified rendered global after the qualified structured
key misses. The direct qualifier-match tightening also rejected the existing
supported `wrap::exported_value` case imported by `using ::exported_value`.
That reference reaches Sema with the same observable shape needed by this
packet: unqualified rendered lookup spelling, qualifier metadata, a structured
qualified miss, and an unqualified global candidate with no stored qualifier.
The attempted code/test edits were removed rather than broadening rendered
compatibility.

## Suggested Next

Next executor packet should implement Step 3.3.3A: carry parser
`namespace_state_.using_value_aliases`, or an equivalent structured
using-value-alias authority, into Sema global lookup. The packet should make
Sema distinguish namespace using-alias imports from wrong-owner qualified
global references before Step 3.3.3B retries unqualified rendered global
fallback removal.

## Watchouts

- Missing metadata: Sema does not currently see
  `namespace_state_.using_value_aliases`, so `wrap::exported_value` via
  `using ::exported_value` is indistinguishable from the reviewer-reported
  wrong-owner unqualified-global fallback using only `Node` qualifier fields
  and `structured_global_keys_by_name_`.
- This prerequisite belongs inside idea 139 because it is parser-to-Sema
  metadata handoff for a parser/Sema lookup route. Do not create a separate
  open idea unless the next packet proves parser cannot supply the carrier
  inside parser/Sema ownership.
- Do not resolve this by restoring broad rendered compatibility for all
  qualified references; the missing alias authority needs a structured carrier.
- Do not retry Step 3.3.3B unqualified rendered global fallback deletion until
  the structured using-value-alias carrier is present or the exact blocker is
  recorded.
- The C++ overload route still shares the same mirror/query key selection; no
  separate named C++ overload test was added in this blocked packet.
- `review/step33_sema_lookup_route_review.md` was already untracked in the
  worktree and was not touched.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' >> test_after.log 2>&1`

Failed during the attempted qualifier tightening. The focused authority test
passed, but `cpp_positive_sema_using_global_scope_decl_parse_cpp` failed with
`use of undeclared identifier 'exported_value'`. `test_after.log` is the
canonical failed proof log for this blocked packet.

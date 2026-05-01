# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3.3A
Current Step Title: Carry Parser Using-Value-Alias Authority Into Sema

## Just Finished

Step 3.3.3A carried parser using-value-alias authority into Sema global lookup.
Parser now annotates qualified value references resolved through
`VisibleNameSource::UsingAlias` with the structured alias target key, and Sema
consults that target key before rendered global compatibility. The focused
lookup-authority test corrupts the rendered reference spelling for
`wrap::exported_value` imported via `using ::exported_value`, proving the
global lookup succeeds from the structured carrier rather than rendered
fallback.

## Suggested Next

Next executor packet can retry Step 3.3.3B: remove or tighten the remaining
unqualified rendered global fallback for qualified references now that
namespace using-value-alias imports have a structured Sema carrier.

## Watchouts

- The carrier currently feeds Sema global variable lookup. If the next packet
  finds the same alias distinction is needed for function or overload lookup,
  extend the same structured target-key path there instead of re-opening broad
  rendered compatibility.
- Step 3.3.3B should still avoid testcase-shaped matching: reject wrong-owner
  qualified references by semantic metadata, while preserving alias imports
  through the new target carrier.
- The C++ overload route still shares the same mirror/query key selection; no
  separate named C++ overload test was added in this blocked packet.
- `review/step33_sema_lookup_route_review.md` was already untracked in the
  worktree and was not touched.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' >> test_after.log 2>&1`

Passed. `test_after.log` contains the canonical proof log for this packet:
build succeeded and the corrected delegated subset passed, including
`frontend_parser_lookup_authority_tests` and 884 `cpp_positive_sema_` tests
for 885/885 total selected tests.

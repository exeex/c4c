# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Step 3.3.3 implementation and correction: made Sema function, ref-overload,
and C++ overload mirror/query paths prefer qualifier-aware structured metadata
keys when qualifier metadata exists, so same-member/wrong-owner references no
longer recover through broad rendered-name compatibility. The direct function
authority test covers hard rejection, and the corrected ref-overload test
oracle now proves wrong-owner rendered overload recovery is blocked by leaving
the stale call non-diagnostic; before the route fix, stale overload recovery
would emit `no viable overload`.

## Suggested Next

Supervisor should review/accept the corrected Step 3.3.3 slice and decide
whether Step 3.3 needs route review, closure, or another bounded Sema lookup
packet.

## Watchouts

- The direct function test was not weakened.
- The overload test uses the existing missing-overload diagnostic behavior as
  the negative oracle: old wrong-owner recovery diagnoses the call, while the
  fixed path treats it as an unknown function and does not diagnose.
- The C++ overload route shares the same mirror/query key selection but does
  not have a separate named test in this packet.
- `review/step33_sema_lookup_route_review.md` was already untracked in the
  worktree and was not touched.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' >> test_after.log 2>&1`

Passed. `test_after.log` is the canonical proof log.

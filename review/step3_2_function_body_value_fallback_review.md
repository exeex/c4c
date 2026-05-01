# Step 3.2 Function-Body Value Fallback Review

Active source idea: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Chosen base commit: `c398ea8bd` (`Make consteval TypeSpec metadata miss authoritative`)

Why this base: the delegated review scope is the current uncommitted Step 3.2
function-body value fallback deletion slice, specifically `git diff HEAD` for
`src/frontend/sema/consteval.hpp`, `src/frontend/sema/consteval.cpp`, and
`todo.md`. `HEAD` is therefore the right acceptance boundary for this review.
The broader Step 3 lifecycle checkpoint remains the earlier Step 3 split, but
using it here would mix already-accepted TypeSpec work with the current
uncommitted function-body value packet.

Commit count since base: 0 committed changes; the reviewed slice is uncommitted.

## Findings

### Severity High: The Slice Adds Rendered-Name Local Binding Authority To Delete A Rendered NTTP Fallback

`src/frontend/sema/consteval.hpp:233` now suppresses
`lookup_rendered_nttp_compatibility()` only when
`has_rendered_local_binding(n)` returns true. That helper, added at
`src/frontend/sema/consteval.hpp:245`, checks `n->name` against
`local_const_scopes` and `local_consts`, which are the rendered-name local
binding maps. The opt-in is enabled for the function-body interpreter at
`src/frontend/sema/consteval.cpp:816`.

This is not clean Step 3.2 deletion progress. Step 3.2 says to block rendered
fallback after populated structured or `TextId` metadata misses, including
same-spelling local names. The current patch instead asks a rendered-name local
map whether the miss belongs to the local-binding route before deciding whether
another rendered route, `nttp_bindings->find(n->name)`, may run. That replaces
one rendered semantic authority decision with a narrower rendered semantic
predicate.

The result may be behavior-preserving for the current narrow cases, but it does
not prove that the structured/text metadata miss is authoritative. The correct
route should derive the covered local-binding miss from the metadata carriers
themselves, for example by making the key/text lookup report which domain had
an authoritative local miss, or by checking the local key/text maps directly
when a local key or `TextId` is present. It should not consult the rendered
local maps as the discriminator.

### Severity Medium: The Claimed Miss Branch Has No Focused Regression Test

`todo.md:18` says no focused C++ miss-branch test was added because the current
source-level harness cannot construct the stale state directly. That is a
useful limitation to record, but it means this slice does not satisfy the Step
3.2 completion check in `plan.md:971`, which asks for focused same-feature tests
proving metadata miss rejection and no stale same-spelling rendered recovery for
the covered route.

The existing positive tests in `test_after.log` show the change did not break
normal consteval parameter/local/for-init and NTTP behavior. They do not prove
the new suppression branch rejects stale same-spelling rendered NTTP recovery.
Given the implementation uses a rendered predicate, accepting it without a
direct branch test would normalize exactly the kind of compatibility route this
idea is trying to remove.

### Severity Medium: The Narrowing Preserves Direct NTTP Compatibility, But The Boundary Belongs In Producer/Metadata State

The packet intentionally leaves direct NTTP rendered compatibility in place for
producer-incomplete routes. That boundary is reasonable in principle and is
consistent with the source idea's policy to park missing metadata instead of
pulling HIR-owned carriers into this plan. The problem is where the boundary is
implemented: `has_rendered_local_binding()` decides it from rendered local
presence, not from the producer-completeness metadata recorded by the key/text
maps.

This is route drift rather than testcase overfit. I do not see a named-case
shortcut, expectation downgrade, or test weakening. The issue is semantic: a
new rendered-name semantic predicate was introduced in a packet whose goal is
to make structured or `TextId` misses authoritative.

## Judgments

Idea-alignment judgment: drifting from source idea.

Runbook-transcription judgment: plan matches idea.

Route-alignment judgment: drifting.

Technical-debt judgment: action needed.

Validation sufficiency: needs broader proof only after reroute; the current
narrow proof is not sufficient for acceptance because it does not exercise the
new miss branch.

Reviewer recommendation: rewrite plan/todo before more execution, or reject the
current implementation slice and send a narrower producer/metadata repair packet
first.

The supervisor should not accept this exact
`suppress_rendered_nttp_for_local_binding_metadata_miss` /
`has_rendered_local_binding()` approach as Step 3.2 deletion progress. A valid
next packet would either:

- repair or expose the local-binding producer metadata so a populated local
  key/text miss can be identified without consulting rendered local maps, then
  delete the covered fallback; or
- park the direct NTTP producer-completeness gap in `todo.md` and defer this
  fallback deletion until a focused lower-level or source-level test can prove
  stale same-spelling rendered recovery is rejected.

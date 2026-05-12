Status: Active
Source Idea Path: ideas/open/196_hir_pending_consteval_structured_identity.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Route One Metadata-Rich Replay Through Structured Identity

# Current Packet

## Just Finished

Step 2 - Route One Metadata-Rich Replay Through Structured Identity completed.
`PendingConstevalExpr` now carries `PendingConstevalCalleeIdentity` with
namespace context, global qualification, qualifier TextIds, callee TextId, and
an explicit completeness bit. `try_lower_consteval_call_expr` populates that
carrier from the AST callee while retaining `fn_name` as display/no-metadata
compatibility state. Pending pre-check and `evaluate_pending_consteval` now
resolve through the structured consteval key when metadata is complete and fail
closed on structured misses before any rendered `fn_name` fallback.

Focused coverage was added for both identity handoff and pending replay fail-
closed behavior: a pending consteval call with deliberately corrupted complete
callee metadata remains unreduced even though the rendered registry entry still
exists.

## Suggested Next

Step 3 should inspect the next remaining metadata-rich consteval lookup surface
from the active runbook and decide whether it needs the same structured/no-
metadata split or can be recorded as already fenced.

## Watchouts

The new pending replay helper intentionally consults rendered `fn_name` only
when the pending carrier lacks complete metadata. `ConstevalCallInfo::fn_name`
and diagnostics still use the rendered string as display state. The proof is
focused to `frontend_hir_lookup_tests`; broader validation remains a supervisor
decision.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1`

Passed. Proof log: `test_after.log`.

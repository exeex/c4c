Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire LIR and HIR-to-LIR Final-Output Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
Step 4 is sufficiently exhausted for this runbook. The committed Step 4 route
now fences HIR template specialization compatibility, struct layout
compatibility, declaration rendered compatibility, template definition
preservation lookup, static member rendered maps, member symbol rendered
lookup, template global rendered lookup, struct method rendered maps, and
ref-overload rendered maps.

Lifecycle decision: advance canonical execution state to Step 5. The Step 4
primary targets in `plan.md` have either been narrowed to structured authority
or explicitly documented as legacy, rendered-preservation, no-owner, no-decl,
or compatibility boundaries. The previous Step 4 route review judged the route
on track, and its template-global watch item has been addressed by the
subsequent committed packet.

Changed files:

| File | Result |
| --- | --- |
| `todo.md` | Advanced the active lifecycle pointer from Step 4 to Step 5 and recorded the next Step 5 packet. |

## Suggested Next

Delegate the first Step 5 packet:

Bridge family: LIR/HIR-to-LIR final-output bridge retirement.

Initial target: inspect `extern_decl_name_map` and the nearest final-output or
legacy rendered-name scans that recover semantic facts from emitted text.

Packet goal: distinguish display/final LLVM output and verifier diagnostics
from production semantic lookup; move any production lookup that has complete
typed metadata to `LinkNameId`, `StructNameId`, typed signature/type mirrors,
or explicit initializer/function ids. Any retained rendered path should be
renamed or documented as output, diagnostic, raw import, legacy, or
compatibility-only.

Suggested proof: focused LIR or HIR-to-LIR tests covering the touched bridge,
plus a fresh build target for the affected test binary. If the packet touches
both LIR data structures and HIR-to-LIR lowering, escalate to the
supervisor-approved broader subset before acceptance.

## Watchouts

- Preserve final emitted spelling and diagnostics unless the Step 5 packet
  explicitly documents an intentional rendering change.
- Do not claim Step 5 progress by moving final-output text around while leaving
  semantic lookup dependent on rendered strings.
- Treat parity verifier and dump text as display/diagnostic surfaces unless a
  production caller is using that text as lookup authority.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was read only for this lifecycle decision.
- No current blockers.

## Proof

Lifecycle-only update; no code validation was run by the plan owner.

Supervisor context records the latest full-suite baseline candidate as accepted
at 3135/3135 after commit `5cb05e4de`.

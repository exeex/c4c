# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Step 3 review was absorbed from
`review/step3_consteval_value_type_review.md`. The prior consteval value/type
work is not closed: same-spelling rendered fallback can still decide
`ConstEvalEnv::lookup(const Node*)` after populated metadata misses, and
consteval `TypeSpec` lookup still uses rendered-name-to-metadata mirror maps as
an intermediate bridge.

## Suggested Next

Execute Step 3.1. Delegate one bounded metadata-producer packet for consteval
locals, loop locals, parameters, synthetic locals, or `TypeSpec` intrinsic
identifier metadata. If the producer is outside parser/Sema ownership or lacks
a clear source carrier, park that exact blocker in this file before any further
Step 3 lookup-deletion packet.

## Watchouts

- Do not treat the previous consteval value/type fallback slice as closed while
  same-spelling rendered fallback can still decide lookup after populated
  metadata misses.
- Same-spelling consteval local/loop/parameter compatibility is now a
  metadata-producer target, not acceptable lookup-deletion progress.
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

Lifecycle-only review absorption. No implementation proof was run for this
rewrite. The next code-changing Step 3.1 packet must produce fresh canonical
`test_after.log` for the supervisor-selected proof command.

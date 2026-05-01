# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3.2
Current Step Title: Add Qualifier-Aware Sema Global/Enum Metadata Keys

## Just Finished

Lifecycle rewrite for `review/step33_sema_lookup_route_review.md`: Step 3.3 is
split so the accepted injected-local no-carrier boundary is preserved as Step
3.3.1, while the next executable packet is Step 3.3.2 qualifier-aware Sema
global/enum metadata/key producer work.

## Suggested Next

Execute Step 3.3.2: repair Sema qualified global and enum lookup metadata/key
production so same-member/wrong-owner rendered compatibility is rejected after
structured metadata exists. Start from `review/step33_sema_lookup_route_review.md`
and inspect `sema_symbol_name_key(...)`, rendered global metadata predicates,
rendered enum metadata predicates, and `lookup_symbol()` global/enum
compatibility gates.

Add focused tests for same-feature same-member/wrong-owner global and enum
drift. Do not treat mismatched unqualified-name tests as sufficient proof.

## Watchouts

- The broad proof still contains the pre-existing `frontend_parser_tests`
  failure: `record-body using member typedef writer should register a direct
  record/member key`. Supervisor-side matching before/after validation confirms
  this packet did not introduce that failure.
- An over-strict first cut also broke namespace/global using-declaration
  bridges; the retained `structured_global_self_names_` guard is required so a
  same-spelling structured global can still satisfy those imports while stale
  rendered globals whose metadata names another symbol remain rejected.
- `c4c-clang-tools` was not needed for this packet; the candidate route was
  localized in `src/frontend/sema/validate.cpp::lookup_symbol()`.
- Injected predefined locals still have no structured key when no matching
  parser TextId reference is present in the function body or constructor
  initializer arguments; the rendered compatibility path remains limited to
  that no-carrier case.
- Reviewer found the remaining global/enum path drifting: unqualified spelling
  equality can still rescue same-member/wrong-owner qualified global or enum
  references because the current metadata/key shape is not qualifier-aware.
- Do not broaden global/enum rendered compatibility as a no-carrier case. If
  the qualifier, owner, namespace, enum-domain, declaration, or equivalent
  carrier is unavailable in parser/Sema scope, stop and record the exact
  producer ambiguity instead of continuing execution.

## Proof

Lifecycle-only rewrite. No implementation validation was run.

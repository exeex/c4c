Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define Freshness Authority Model And Query Skeleton

# Current Packet

## Just Finished

Lifecycle activation only. No executor packet has completed yet.

## Suggested Next

Start Step 1 from plan.md: inspect prepared/prealloc enum, lookup, query, and
printer conventions, then add the minimal freshness authority model and
fail-closed query skeleton.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- Keep producer rematerialization and explicit publication higher precedence
  than older PriorPreservation only when they are valid for the same value/use.
- Preserve existing fail-closed behavior until the freshness verifier covers
  the same malformed or unknown-authority shape.
- Track closure-inventory notes here as consumers are wired or deliberately
  deferred.

## Proof

Not run. Activation is lifecycle-only and made no implementation changes.

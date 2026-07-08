Status: Active
Source Idea Path: ideas/open/598_select_carrier_alias_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Carrier Alias Consumers

# Current Packet

## Just Finished

No executor packet has run yet. Lifecycle activation created the active
runbook for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: audit select-carrier and select-alias producer,
support, consumer, diagnostic, and target-consume surfaces, then record the
audited consumer set and proposed representative shared-prealloc consumer here.

## Watchouts

- Do not implement a freshness kind before the select-carrier alias ownership
  rule is explicit.
- Do not claim progress through expectation rewrites, unsupported-marker
  edits, allowlist changes, diagnostics-only changes, or target-local shape
  checks.
- Keep destination fan-in, predecessor-edge suppression, pointer/address
  follow-ups, target migration, and Prepared MIR view design separate.

## Proof

Not run. This was a lifecycle-only activation; code validation belongs to the
first implementation packet.

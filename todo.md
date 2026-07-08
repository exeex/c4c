Status: Active
Source Idea Path: ideas/open/598_select_carrier_alias_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Carrier Alias Consumers

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: audited the select-carrier alias authority
surface without changing implementation behavior.

Audited surfaces:

- Producer/support records: `PreparedSelectCarrierAliasAuthority`,
  `PreparedSelectCarrierAliasAuthorityRecord(s)`, and
  `PreparedSelectCarrierAliasAuthorityEvidence` carry publication identity,
  selected source identity, source-producer identity, carrier alias rows,
  candidate counts, status, and `source_use_closure_proven`. They are alias
  support facts today, not selected-source freshness facts.
- Producer/planner: `plan_prepared_select_carrier_alias_authority(...)`
  publishes `available` only for an available select-materialization edge
  publication with a binary source producer, matching join transfer/final
  carrier, unique alias select candidates, alias payload use rather than
  condition use, alias result feeding the final select, and no non-carrier
  source uses.
- Support-fact collectors:
  `collect_prepared_select_carrier_alias_candidates(...)` finds join-block
  alias selects that payload-use the publication source and feed the final
  select; `collect_prepared_select_carrier_alias_authority_evidence(...)`
  emits all rows for diagnostics; `collect_prepared_select_carrier_alias_authorities(...)`
  filters to available rows for consumers.
- Identity producer: `populate_select_carrier_alias_identity(...)`, called from
  `BirPreAlloc::publish_contract_plans()`, interns alias select result names
  for well-formed carrier-alias shapes so later collection can preserve alias
  identity even when value homes are absent.
- Diagnostics/dumps: `append_select_carrier_alias_authorities(...)` prints
  every evidence row under `prepared-select-carrier-alias-authorities`,
  including status, edge/source/destination identity, source producer,
  candidate and alias counts, alias locations, and `source_use_closure`.
  Existing tests assert both available and `non_carrier_source_use` dump rows.
- Shared-prealloc diagnostic support:
  `classify_prepared_object_select_consumer(...)` and
  `diagnose_prepared_object_consumer(...)` classify complete join-transfer
  select carriers and explain missing/ambiguous/malformed carrier facts, but
  they do not consume select-carrier alias authority and do not authorize
  freshness.
- RV64 target-consume-only paths:
  `RiscvPreparedFunctionAdmissionResult` collects the available authority
  records; `prepared_select_edge_binary_source_has_carrier_alias_authority(...)`
  matches a publication against available alias authority and
  `source_use_closure_proven`; `prepared_select_is_authorized_carrier_alias(...)`
  suppresses carrier-alias select emission by alias result name; `object_emission.cpp`
  threads those records through move-bundle, source-producer, binary-source,
  select-fragment, and instruction emission paths.

Proposed representative shared-prealloc consumer: the selected-source
acceptance query currently represented target-side by
`prepared_select_edge_binary_source_has_carrier_alias_authority(...)` inside
`prepared_select_edge_binary_source_has_authorized_consumers(...)`. Step 2
should state the ownership contract for that exact binary select-edge
publication/source use before any migration; after that, Step 3 can add or
reuse shared-prealloc freshness vocabulary and make this source-acceptance gate
fail closed on freshness rather than alias support alone. The RV64 paths should
remain consume-only during that migration.

## Suggested Next

Step 2: State The Ownership Contract.

## Watchouts

- Do not implement a freshness kind before the select-carrier alias ownership
  rule is explicit.
- Do not claim progress through expectation rewrites, unsupported-marker
  edits, allowlist changes, diagnostics-only changes, or target-local shape
  checks.
- Current alias authority proves select-carrier alias shape and source-use
  closure, but not freshness by itself. Alias-only, destination-only,
  suppression-only, complete-home, target-shape, and structural join-transfer
  evidence must remain insufficient unless Step 2 defines an explicit selected
  source freshness rule.
- There is no existing shared-prealloc consumer that already enforces selected
  freshness for this route; the representative migration should either add a
  shared query boundary for the binary select-edge publication source or stop
  after the research/contract packet if the rule cannot be stated cleanly.
- Keep destination fan-in, predecessor-edge suppression, pointer/address
  follow-ups, target migration, and Prepared MIR view design separate.

## Proof

Not run. This was an audit-only packet; no build/test was required and
`test_after.log` was not updated.

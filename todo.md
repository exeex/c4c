Status: Active
Source Idea Path: ideas/open/598_select_carrier_alias_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: State The Ownership Contract

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: stated the select-carrier alias freshness
ownership contract for the representative route identified by Step 1.

Authorized use:

- The use being authorized is the exact binary select-edge publication source
  acceptance currently represented target-side by
  `prepared_select_edge_binary_source_has_carrier_alias_authority(...)` inside
  `prepared_select_edge_binary_source_has_authorized_consumers(...)`.
- The accepted value is the publication source binary result for the exact
  select-materialization edge publication: same function, predecessor,
  successor, destination value/id/name, source value/id/name, binary source
  producer kind, source producer block/inst, join transfer, final select, and
  carrier alias set whose alias selects payload-use the source and feed the
  final select.
- The authority authorizes only treating that selected source as fresh for this
  select-carrier alias publication/source use. It does not authorize unrelated
  source uses, destination fan-in, predecessor-edge consumed suppression,
  target-local select emission, or broad edge-publication move acceptance.

Freshness vocabulary decision:

- A distinct select-carrier alias freshness vocabulary entry is required. The
  required use/source pair should be a new narrow pair, for example
  `PreparedValueFreshnessUseKind::SelectCarrierAliasSource` with
  `PreparedValueFreshnessSourceKind::SelectCarrierAlias`.
- Step 3 should add the matching proof/rank/reference shape narrowly, for
  example a select-carrier-alias proof/rank plus a reference to the exact
  carrier-alias authority/publication facts. The selected authority must match
  the queried value id/name and the exact publication/source-producer program
  point before acceptance.
- Do not reuse `PreparedValueFreshnessUseKind::MoveBundleSource` or
  `PreparedValueFreshnessSourceKind::MoveBundleSource`: that vocabulary owns
  move-bundle source consumption, tied to a concrete `PreparedMoveBundle` and
  `PreparedMoveResolution`. The representative select-carrier alias route is a
  binary select-edge publication source acceptance, not a move-bundle source
  consumption.
- Do not reuse `PreparedValueFreshnessUseKind::DirectEdgePublicationSource` or
  `PreparedValueFreshnessSourceKind::DirectEdgePublication`: that vocabulary
  owns direct edge-publication move freshness through an exact edge publication
  plus move reference. The select-carrier alias route additionally depends on
  alias select closure and final-select carrier shape, and accepting it through
  direct edge-publication freshness would hide the alias-specific ownership
  boundary.

Facts insufficient by themselves:

- Alias-only evidence is insufficient, including available alias rows,
  matching alias value names, alias candidate counts, or
  `source_use_closure_proven`.
- Destination-only evidence is insufficient, including destination value/id/name
  agreement, destination homes, destination register legality, or stack
  destination fan-in facts.
- Suppression-only evidence is insufficient, including predecessor-edge
  consumed suppression, target-local omission of a binary/select, or an empty
  emitted fragment.
- Complete-home evidence is insufficient, including complete source homes,
  direct homes, register homes, stack homes, or alias value homes.
- Target-shape evidence is insufficient, including RV64-supported opcodes,
  operand shape, immediate/register encodability, or target-local helper
  success.
- Structural join-transfer evidence is insufficient, including a complete
  select-materialization join transfer, a matching final select, edge-copy
  publication shape, or parallel-copy legality.

Expected fail-closed statuses/diagnostics:

- Missing freshness: query result `no_candidate`, `missing_value`, or
  `unknown_use` must map to a select-carrier-alias-specific missing freshness
  status/diagnostic, for example
  `missing_select_carrier_alias_source_freshness`.
- Ambiguous freshness: query result `ambiguous_candidate` must map to an
  ambiguous freshness status/diagnostic, for example
  `ambiguous_select_carrier_alias_source_freshness`.
- Stale freshness: a selected or matching candidate whose reference does not
  still match the exact publication, source producer block/inst, join transfer,
  alias set, value id/name, or program point must fail as invalid or
  unsupported freshness, not fall back to alias authority.
- Wrong-value freshness: a candidate for a different value id/name must not
  select; the consumer should report missing freshness for the queried source,
  or invalid freshness if a same-id/same-use candidate has inconsistent
  identity.
- Wrong-use freshness: candidates for `MoveBundleSource`,
  `DirectEdgePublicationSource`, `ProducerPublicationOperand`, or any other use
  kind must not satisfy this query and should surface as missing freshness for
  the select-carrier alias source use.
- Freshness-less route: available alias authority, complete homes,
  destination legality, suppression state, target shape, or structural
  join-transfer evidence with no selected select-carrier-alias freshness must
  fail closed with the missing freshness status/diagnostic.

## Suggested Next

Step 3: Migrate One Representative Consumer by adding the narrow
select-carrier alias freshness vocabulary/query boundary and requiring it at
the representative binary select-edge publication source acceptance gate.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker
  edits, allowlist changes, diagnostics-only changes, or target-local shape
  checks.
- Keep the migration in shared prealloc. The RV64 helpers may remain
  target-consume-only callers of the selected authority, but they should not
  grow target-local freshness logic.
- Step 3 must fail closed unless the selected authority matches the exact
  value, use, source kind, proof/rank, reference, and program point required by
  this contract.
- Reusing `MoveBundleSource` or `DirectEdgePublicationSource` would overload
  an unrelated owner and should be rejected unless the supervisor explicitly
  changes the contract.
- Keep destination fan-in, predecessor-edge suppression, pointer/address
  follow-ups, target migration, and Prepared MIR view design separate.

## Proof

Ran `git diff --check`; passed. This is a todo-only contract packet; no
build/test was required and `test_after.log` was not updated.

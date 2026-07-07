# Target ABI Contract Research Index

This directory contains the completed research set for the target ABI contract
and prepared value-consumption model. The overall result is that the current
pipeline direction is healthy: target facts enter through `TargetProfile`, BIR
records semantic call ABI facts, prealloc turns those facts into prepared call
plans and register placements, and AArch64/RV64 consume the prepared surface
instead of recomputing the calling convention from raw target strings.

The contract is sufficient for ordinary shared AArch64/RV64 call lowering with
named limitations. The gaps are not in the high-level flow; they are in
uniform physical register identity publication, explicit freshness authority
for producer/rematerialization versus prior preservation, and move-bundle
source authority. Closed idea tails show that recent narrow fixes repaired
specific stale-home, publication, rematerialization, and move-bundle symptoms,
but they did not establish one shared value-consumption authority model.

## Answer Files

- [01. How Target Information Enters The Pipeline](01_how_target_information_enters_the_pipeline.md)
- [02. Are Current Contract Fields Sufficient](02_are_current_contract_fields_sufficient.md)
- [03. Where Target Facts Are Split](03_where_target_facts_are_split.md)
- [04. Current Prepared Value Consumption Model](04_current_prepared_value_consumption_model.md)
- [05. Prior Preservation Freshness And Stale-Home Risk](05_prior_preservation_freshness_and_stale_home_risk.md)
- [06. Closed Idea Tails And Follow-Up Questions](06_closed_idea_tails_and_followup_questions.md)

## Synthesis

`TargetProfile`, BIR ABI payloads, prealloc call plans, prepared register
placements, move bundles, preservation records, and target consumers already
form a usable shared pipeline. That pipeline is strongest when carrying
semantic call classification and ordinary argument/result movement.

The weak points are where consumers need to know more than "this route is
well-formed." RV64 object emission often needs stable physical target register
identity, while AArch64 can frequently operate from register names or prepared
placements. Both targets need a clearer rule for when a producer
materialization, publication row, direct home, prior preservation, or move
source is the freshest authority for a value.

The smallest consolidation candidate is the prepared target register profile:
make it the uniform source for x86/AArch64/RV64 ABI register pools,
placements, and `PreparedTargetRegisterIdentity` facts. Value freshness should
be handled as a separate prepared/prealloc authority relation rather than as a
backend fallback.

## Final Recommendations

| Follow-up | Classification | Rationale |
| --- | --- | --- |
| Keep this research set as the durable target ABI contract baseline and cite it from later implementation ideas. | Documentation | The six answer files now capture the current code paths, sufficiency limits, ownership splits, value-consumption order, freshness risk, and closed-tail evidence. |
| Add a stale-row proof matrix for prepared public lookup compatibility across same-consumer x86, AArch64, and RV64 paths before demoting compatibility APIs. | Documentation | The closed tails show public lookup compatibility should remain until wrong producer block, instruction index, source value, edge, duplicate, and obsolete-owner rows fail closed for the same consumers that would lose the fallback. |
| Extend the prepared target register profile into the uniform policy owner for ABI register pools, placements, and `PreparedTargetRegisterIdentity` publication across x86, AArch64, and RV64. | Narrow implementation idea | The current profile layer already owns the closest shared target ABI register policy; the narrow implementation slice is to publish uniform physical identity facts without moving semantic ABI classification or final instruction rendering. |
| Make call plans, value homes, move bundles, and backend consumers depend on the uniform prepared target register identity where physical identity is required. | Narrow implementation idea | This is a consumer migration after identity publication exists. It should be split by target or route so it does not become a broad backend rewrite. |
| Define an explicit prepared freshness authority that ranks direct homes, producer rematerialization, publication rows, prior preservation, and move-bundle sources for a value use. | Discussion-required architecture work | The evidence spans several surfaces and targets. The project needs to choose whether freshness is one shared authority record or local proof fields plus a common verifier before implementation work starts. |
| Decide whether producer rematerialization supersedes prior preservation through producer order, publication epoch, dominance, or an explicit supersedes relation. | Discussion-required architecture work | This rule controls stale-home risk and must be settled before a narrow implementation can safely change `PriorPreservation` precedence. |
| Decide whether move-bundle authority should grow source-freshness facts or link to a separate value-consumption authority. | Discussion-required architecture work | Current move-bundle authority is mostly destination-oriented. Extending it directly may blur ownership unless the architecture first defines where freshness belongs. |

## File-Set Check

The required documentation shape is exactly six numbered Markdown answer files
plus this `index.md`. There are no implementation, test expectation,
unsupported-marker, runtime behavior, source idea, or closed idea edits in this
research index step.

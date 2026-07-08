Status: Active
Source Idea Path: ideas/open/607_destination_fan_in_authority_research.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide The Destination Authority Rule

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: created `docs/destination_fan_in_authority/02_destination_authority_rule.md` documenting the destination legality alternatives and selecting explicit rejection as the implementation-ready rule for the current evidence.

Conclusion: a non-parallel move bundle with more than one register source targeting the same stack destination must fail closed unless prepared/prealloc producer metadata explicitly proves destination ordering, mutual exclusion, or merge authority at the consumer program point. Source freshness remains required for accepted sources, but source freshness is not destination authority.

## Suggested Next

Proceed to Step 3: split follow-up implementation ownership, keeping producer destination-authority work separate from RV64 consumption and recording that the current `125` row family remains rejected until producer authority exists.

## Watchouts

- Keep this route documentation-only.
- Do not change implementation files, tests, expectations, unsupported markers, allowlists, runtime behavior, timeout policy, or accounting.
- Do not select a destination by testcase shape, source order accident, or RV64 assembly convenience.
- Keep source freshness authority separate from destination fan-in authority; ideas `587` and `588` prove source-side authority surfaces, not same-destination legality.
- Any future acceptance route must publish one concrete producer fact: ordering with an authoritative final destination state, mutual exclusion with predicates/selected active candidate, or merge authority with semantic equivalence/explicit merge metadata.
- Do not let the raw current-workdir `128` grep count silently replace the recovery map's `125` planned family count without supervisor/plan-owner direction.
- Preserve exactly one active source idea: `ideas/open/607_destination_fan_in_authority_research.md`.

## Proof

Documentation-only proof:

- `test -f docs/destination_fan_in_authority/02_destination_authority_rule.md`
- `rg -n 'ordering|mutual|merge|explicit rejection|producer facts|source freshness|destination authority' docs/destination_fan_in_authority/02_destination_authority_rule.md`
- `git diff --check -- docs/destination_fan_in_authority/02_destination_authority_rule.md todo.md`

No root-level proof logs were created or modified for this documentation-only packet.

Status: Active
Source Idea Path: ideas/open/607_destination_fan_in_authority_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Split Follow-Up Implementation Ownership

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: created `docs/destination_fan_in_authority/03_implementation_split.md` documenting the follow-up implementation ownership split after Step 2 selected explicit rejection as the current destination authority rule.

Conclusion: producer/prealloc owns destination authority production, RV64 owns only consumption of already-authorized destination fan-in, and the current `125` non-parallel multi-source stack-destination rows remain rejected or blocked until producer metadata proves ordering, mutual exclusion, or merge authority.

## Suggested Next

Proceed to Step 4: build `docs/destination_fan_in_authority/index.md`, verify the documentation directory contains exactly the required answer files plus index, and record the research-package acceptance recommendation.

## Watchouts

- Keep this route documentation-only.
- Do not change implementation files, tests, expectations, unsupported markers, allowlists, runtime behavior, timeout policy, or accounting.
- Do not select a destination by testcase shape, source order accident, or RV64 assembly convenience.
- Keep source freshness authority separate from destination fan-in authority; ideas `587` and `588` prove source-side authority surfaces, not same-destination legality.
- Keep producer-authority work and RV64 consumption work as separate follow-up implementation ideas if implementation is later opened.
- Producer proof must publish one concrete destination fact: ordering with an authoritative final destination state, mutual exclusion with predicates/selected active candidate, or merge authority with semantic equivalence/explicit merge metadata.
- RV64 proof is downstream only: consume already-authorized bundles and preserve rejection for `authority=none`, unsupported authority kinds, mismatched bundle-versus-move authority, and missing source freshness.
- Do not let the raw current-workdir `128` grep count silently replace the recovery map's `125` planned family count without supervisor/plan-owner direction.
- Preserve exactly one active source idea: `ideas/open/607_destination_fan_in_authority_research.md`.

## Proof

Documentation-only proof:

- `test -f docs/destination_fan_in_authority/03_implementation_split.md`
- `rg -n 'producer|RV64|consumer|proof surface|blocked|rejected|follow-up' docs/destination_fan_in_authority/03_implementation_split.md`
- `git diff --check -- docs/destination_fan_in_authority/03_implementation_split.md todo.md`

No root-level proof logs were created or modified for this documentation-only packet.

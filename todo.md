# Current Packet

Status: Active
Source Idea Path: ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the HIR definition-to-occurrence producer seam

## Just Finished

- Lifecycle switch from 838 Step 2: its accepted declaration/store fact capture
  and exact LIR return point are durable in the source idea.

## Suggested Next

- Inspect the HIR aggregate definition-registration and function-signature
  `QualType` construction seams; establish the direct canonical-ref producer
  contract before code changes.

## Watchouts

- This blocker owns HIR occurrence fact production only. Do not migrate LIR
  lowering or recover aggregate identity from legacy owner keys, tags, text, or
  parser pointers.

## Proof

- Lifecycle slice: structural/linkage inspection only. Code-bearing packets
  must select and record fresh focused proof before acceptance.

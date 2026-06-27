# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Document the Helper Contract Route

## Just Finished

Step 1 created
`docs/prepared_fact_contracts/helper_operand_home_contract_plan.md` with the
required idea 413 taxonomy rows, idea 418 audit rows, the idea 418
no-applicable-recovery-row note for idea 416, and helper owner decisions for
`va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`.

## Suggested Next

Execute Step 2 from `plan.md`: inventory the current variadic helper payloads,
producers, verifier checks, and target consumers before changing contracts.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- The idea 418 audit found no selected recovery row requiring immediate idea
  416 cleanup; use the coherent helper rows as boundaries for fail-closed typed
  helper contracts.

## Proof

Docs-only proof; no build required. Verified the new document with `rg` for
the required row IDs and helper names. No `test_after.log` was written because
the packet excluded test logs.

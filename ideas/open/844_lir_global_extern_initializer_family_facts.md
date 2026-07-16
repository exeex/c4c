# LIR Global, Extern Declaration, and Initializer Family Facts

Status: Open
Type: first-owner global/extern type fact migration
Matrix Rows: M15
Dependencies: 841, 838, 840, 839, 843; preserves 760 and 762

## Goal

Migrate global and extern type facts from parallel `TypeSpec`/text/optional-ref
mirrors to family refs while leaving initializer-text semantics separately owned.

## In Scope

- Migrate global/extern lowering, verifier, printer, and family-ref collection
  for type facts.

## Out Of Scope

- Initializer-text semantic redesign, legacy scanner deletion without its named
  producer/carrier migration, or 812/813 non-type string routing.

## Acceptance Criteria

- Fresh build plus global/extern lowering, verifier/printer, collection, and
  legacy initializer compatibility proof.
- Delete semantic `llvm_type` and extern runtime-text only after all global/
  extern consumers use family refs; retain final output and initializer scanner
  until named migration proves deletion.

## 866 Reconciliation And 734 Return

Idea 866 keeps this as the ordered producer/schema successor for global and
extern type facts. Return to 734 only after one exact typed global or extern
handoff is accepted; initializer-text semantics and non-type global policy stay
outside this return and remain delegated to separate evidence routes.

## Reviewer Reject Signals

- Reject treating final rendering or initializer text as type authority,
  deleting the scanner prematurely, testcase-only extern handling, or 762
  scope expansion.

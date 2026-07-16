# LIR Universal Model and String Escape-Hatch Deletion

Status: Open
Type: terminal universal-model convergence and disposition handoff
Matrix Rows: M16
Dependencies: accepted deletion gates from 838--846; terminal handoff to 797

## Goal

Delete universal `LirTypeRef`, semantic `runtime_text`, mutable `str()`,
implicit string conversions, textual equality/classification, and expired
adapters after every M1--M15 replacement is accepted; hand valid LIR's final
disposition to 797.

## In Scope

- Remove only fully migrated universal APIs/fields/factories/conversions and
  adapters, then prove compile-time separation and receiver/dispatcher
  disposition required by 797.

## Out Of Scope

- Retaining a universal compatibility substitute, taking ownership of 797's
  final coverage convergence, or routing residual non-type strings (812/813).

## Acceptance Criteria

- Fresh full build and broad regression/coverage proof show no remaining
  universal fields, factories, conversions, semantic runtime text, or expired
  adapter; all focused deletion gates have accepted evidence.
- Deliver the complete valid-LIR disposition to 797, not a claim that 797 is
  complete.

## 866 Reconciliation And 734 Return

Idea 866 orders this as terminal deletion after M1-M15 replacements and all
needed 734 receipts have accepted dispositions. It does not return directly to
734; its return is a final valid-LIR disposition handoff to 797.

## 846 Handoff

Idea 846 closed after bounded native-authoritative verifier/printer consumer
migrations accepted selected integer, vector, aggregate, scalar boundary, and
memory/index rendering gates. The deletion pass here owns remaining generic
helpers, mutable semantic string escape hatches, implicit string conversions,
and expired compatibility adapters only after confirming each target no longer
has selected semantic callers. Switch selector surfaces remain outside this
handoff and are owned by ideas 821 and 822.

## Reviewer Reject Signals

- Reject deletion before any named M1--M15 consumer/adaptor gate is met.
- Reject a renamed universal bag, generic ID, mutable/text compatibility escape
  hatch, expectation downgrade, or using 797 as a catch-all repair owner.

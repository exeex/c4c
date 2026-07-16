# LIR Universal Model and String Escape-Hatch Deletion

Status: Closed
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

## Closure Disposition

Closed as capability complete for the 847 terminal deletion route after Step 5
handoff.

Accepted evidence:
- Deleted implicit LIR opcode/operand string conversions and the mutable LIR
  operand string accessor.
- Replaced selected semantic string authority with typed facts for comparison,
  arithmetic, call, and ternary operands.
- Threaded LIR type refs through BIR lowering paths for PHI, aggregate
  slots/allocas/loads, `va_arg` slots, params, sret/copy layouts, call aliases,
  stores, local memory identity, and provenance scalar checks.
- Classified the remaining call ABI, global aggregate, aggregate layout,
  memory projection, and local memory/provenance/central-layout text bridges as
  deliberate no-id, legacy, or owner-specific compatibility boundaries rather
  than unfinished 847 deletion work.

Accepted proof:
- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1`
- Result: `100% tests passed, 0 tests failed out of 3038`.

Downstream ownership remains unchanged. Idea 797 owns terminal coverage
convergence after accepted handoffs and receipts; this closure does not claim
797 is complete. Ideas 812 and 813 retain residual non-type/string semantic
authority routes. Ideas 821 and 822 retain switch selector surfaces. Idea 734
and receiver receipts remain downstream dependencies where applicable.

Optional stale code-comment cleanup in `memory/intrinsics.cpp` was classified
as non-blocking housekeeping, not unfinished 847 acceptance work.

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

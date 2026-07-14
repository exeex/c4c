# Unresolved-External Direct-Call Source Representation

## Result

The current supported source and frontend surface has no legal
production-facing carrier for a plain, direct, fixed-empty scalar call that
has native global/link identity while `target_fn` is absent. A declaration
retains an HIR `Function` and therefore resolves `target_fn`; the forms that
do not retain that declaration have no native direct `LinkNameId`, or are
indirect calls.

The evidence and candidate-by-candidate trace are in
[01 — Legal source-to-HIR representation](01_legal_source_to_hir_representation.md).

This documentation does not unblock, complete, supersede, or close ideas 746
or 744. A separate lifecycle decision is required for idea 746.

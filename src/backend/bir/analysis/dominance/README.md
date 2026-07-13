# Dominance Analysis

Status: scaffold. Consumes one exact CFG result and produces dominators,
postdominators where requested, dominance frontiers, and loop-facing queries.
Its result is invalid after any unpreserved CFG mutation.

Legacy coverage: implicit ordering/dominance assumptions in phi, publication,
comparison, select, and producer traversal paths.

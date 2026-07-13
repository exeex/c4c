# Out-of-SSA and MIR Construction

Status: scaffold. Consumes `PreparedBir`, lowers phi/block arguments to explicit
parallel-copy bundles, splits critical edges where required, and creates
target-independent MIR identities.

Legacy coverage: `prealloc/out_of_ssa.cpp`, regalloc phi moves, edge
publications, cyclic copies, multi-phi blocks, and critical edges.

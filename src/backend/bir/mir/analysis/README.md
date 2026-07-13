# MIR Analyses

Status: scaffold.

MIR analyses are revision-bound and machine-stage-local. Initial families are
machine CFG, liveness, live intervals, register pressure, call-clobber effects,
and frame-object interference. Dense numbering cannot escape one analysis
result or become MIR identity.

Legacy coverage: allocation interval construction, liveness, classification,
stack-slot/copy/alloca coalescing, call clobbers, and value-home queries.

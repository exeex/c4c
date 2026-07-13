# MIR Passes

Status: scaffold.

Order: `out_of_ssa -> instruction_select -> call_lowering ->
register_allocation -> spill_reload -> frame_layout -> prologue_epilogue`.

Every pass consumes verified output from its predecessor and produces a
verifiable machine-stage form. Target-independent BIR is read-only after the
boundary.

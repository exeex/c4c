# Spill and Reload Insertion

Status: scaffold. Realizes allocation spill decisions as explicit machine
operations, repairs constrained uses/defs, and exposes frame-slot requirements
to frame layout.

Legacy coverage: spill/reload, storage plans, copy/slot coalescing, rematerialized
values, call-boundary saves, and target load/store restrictions.

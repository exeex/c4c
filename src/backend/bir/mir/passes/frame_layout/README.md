# Frame Layout

Status: scaffold. Assigns frame objects, spill slots, local storage, outgoing
arguments, variadic areas, dynamic-stack anchors, alignment, and offsets. The
result is a typed frame plan attached to MIR, never canonical BIR.

Legacy coverage: `prealloc/frame*`, `prealloc/stack_layout/`, storage plans,
alloca/copy coalescing, dynamic stack, inline-asm stack needs, and target unwind
or alignment rules.

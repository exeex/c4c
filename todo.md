Status: Active
Source Idea Path: ideas/open/563_rv64_f64_global_memory_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect F64 Global-Memory Boundary

# Current Packet

## Just Finished

Activated plan Step 1 from
`ideas/open/563_rv64_f64_global_memory_consumption.md`.

## Suggested Next

Inspect the current `src/20001121-1.c` `unsupported_global_data` diagnostic and
trace why the RV64 object route rejects prepared `double` / `F64` global-memory
accesses after prepared facts have reached target object emission.

## Watchouts

- Keep this as RV64 prepared global-memory consumption work unless inspection
  proves prepared facts are missing or stale.
- Do not synthesize missing object data, relocation authority, symbols, or
  initializer shape in RV64 lowering.
- Do not mix F128, long-double, stack-frame, FPR callee-saved slot, or prepared
  zero-fill work into this lane.
- Do not special-case `src/20001121-1.c`, diagnostic strings, allowlists,
  expected outputs, unsupported markers, or pass/fail accounting.

## Proof

Activation only. No build or test proof was run for this lifecycle packet.

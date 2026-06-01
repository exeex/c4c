Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Align Memory-Backed f128 And Variadic Consumers

# Current Packet

## Just Finished

Step 5 mapping packet completed. `memory.cpp` remains the prepared memory
authority boundary: `apply_stack_layout_to_memory_record` consumes prepared
stack layout and address materialization facts to resolve frame-slot offsets
and mark prepared snapshots, while
`lower_store_global_value_publication_from_plan` accepts only an already
prepared frame-slot `MemoryOperand` for stack-home publication and then emits
the target store.

`f128.cpp` has the first concrete duplicated authority candidate.
`f128_memory_backed_carrier_memory` rebuilds a prepared frame-slot
`MemoryOperand` from the f128 carrier's slot and stack offset, and
`f128_printable_memory_address` carries a local f128 address materialization
path for frame slots and symbols. The f128 ABI-local policy is the q-register
transport and helper marshal validation; the prepared memory ownership should
not live in the f128 printer.

`variadic.cpp` is mostly ABI-local transfer policy rather than the first
prepared-memory authority target. The va-list field accessors consume prepared
variadic homes and field offsets; aggregate `va_arg` lowering owns AArch64
copy chunking, progression, register-save-area selection, and stack-slot
destination stores. Those paths duplicate address formatting helpers, but the
inspected code does not expose a prepared `MemoryOperand` authority boundary
equivalent to the f128 memory-backed carrier path.

## Suggested Next

First concrete Step 5 implementation packet: align memory-backed f128 carrier
transport with the shared prepared memory consumer boundary. Keep the packet
limited to replacing the f128-local prepared frame-slot `MemoryOperand`
construction/materialization with a shared prepared-memory helper or existing
`memory.cpp` callable path, while preserving f128-specific q-register load/store
and helper marshal policy in `f128.cpp`.

## Watchouts

Do not start with variadic aggregate copies unless the supervisor explicitly
chooses a helper-only cleanup packet. The variadic transfer logic is currently
prepared variadic ABI consumption, not a clear prepared-memory ownership
violation. A Step 5 implementation should distinguish reusable address helper
factoring from authority movement: helper factoring alone is not idea 70
progress unless it removes a duplicated prepared memory decision.

For f128, avoid inventing a new f128-only frame-slot contract. The useful
boundary is consumption of prepared memory facts already established by the
memory path; f128 should retain only ABI-local decisions such as q-register
selection, f128 load/store width, helper marshal directions, and diagnostics.

## Proof

Mapping-only packet; no implementation files changed and no root-level proof
log was produced. Checked:

```sh
git diff --check -- todo.md
```

Status: Active
Source Idea Path: ideas/open/614_rv64_pointer_local_memory_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Selected Local-Memory Consumer

# Current Packet

## Just Finished

Executed Step 2, `Implement First Selected Local-Memory Consumer`, for the
scalar prepared frame-slot local-memory packet.

Implemented the selected prepared frame-slot consumer in the RV64 prepared
local-memory emitter for explicit `base=frame_slot` accesses with frame-slot
id, byte offset, access size, access alignment, `base_plus_offset=yes`,
default address space, and supported 1-, 2-, 4-, or 8-byte scalar/floating
widths. The rule now accepts the same alignment contract used by the object
fragment helper (`align <= size`) and emits 8-byte GPR/FPR stack loads/stores
for the simple prepared path.

Focused tests added:

- `builds_prepared_i64_local_frame_object` proves selected 8-byte frame-slot
  local store/load lowering with concrete frame-slot layout.
- `rejects_prepared_i64_local_frame_fail_closed_shapes` preserves rejection
  for missing frame-slot id, missing `base_plus_offset`, non-default address
  space, and unsupported 16-byte width.

Representative probes after the change:

- `src/20000722-1.c`, `src/20010123-1.c`, `src/20011109-2.c`, and
  `src/920429-1.c` still stop at `unsupported_local_memory_access`.
- Prepared dumps show those rows have selected frame-slot accesses consumed by
  this packet, but their remaining first local-memory blocker is adjacent
  prepared string-constant or pointer-value local-memory use. I did not fold
  those adjacent access families into this frame-slot packet.

## Suggested Next

Proceed to Step 3 with a refreshed residual scan for adjacent selected
local-memory access families. The next likely packet is prepared pointer-value
base-plus-offset local-memory consumption only if freshness/selected authority
is explicit and complete; otherwise split the remaining string-constant or
pointer-value producer/consumer gap before more implementation.

## Watchouts

- The representative C rows did not move past the broad
  `unsupported_local_memory_access` owner because the first remaining
  unsupported access is not the frame-slot access handled here.
- Keep `src/20010605-2.c`, `src/20040208-1.c`, and `src/ieee/inf-1.c` as
  unsupported-width guards.
- Keep BIR GEP/address production, direct pointer arithmetic policy, ABI,
  branch/select, runtime, expectation, unsupported-marker, allowlist, timeout,
  and accounting work out of this route.
- Do not infer pointer-value or string-constant authority from final assembly
  shape or testcase names.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed
out of 346`.

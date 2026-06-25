Status: Active
Source Idea Path: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Frame-Slot Memory Facts

# Current Packet

## Just Finished

Completed Plan Step 1 audit for the three residual representatives:

- `src/20000217-1.c`: prepared dump has direct `i16` frame-slot accesses in
  `main` with slots `#4` and `#5`, slot offsets `0` and `2`, access
  offset `0`, size/alignment `2`, and store direction for `%lv.x` and
  `%lv.y`. It also has `address_materialization` for those frame slots as call
  arguments, plus `showbug` pointer-value `i16` loads/stores through `%p.a` and
  `%p.b`; those pointer-value accesses are not a frame-slot base form.
- `src/20000121-1.c`: prepared dump has a `doit` `i8` pointer-value load from
  `%p.id`, and a stack-resident `i64` call argument source
  `%t1` at slot `#2`, stack offset `16`, size/alignment `8`. The first local
  memory diagnostic is pointer-value based; the frame-slot fact here is the
  call-argument value/address source, not a direct local load/store.
- `src/va-arg-13.c`: prepared dump has `test` frame-slot local accesses for
  pointer-width values, including stores/loads at slots `#8`, `#4`, `#6`, and
  `#7`, offsets `0`, sizes/alignment `8`, directions for both load and store,
  and stack slots such as `#9`/`#10` at stack offsets `48`/`56` for call-arg
  frame-slot address sources. It also publishes `prepared-variadic-entry-plans`
  with `overflow_area` slot `#12`, `va_start` helper operands, and
  `llvm.va_start`/`llvm.va_end` calls, which remain under the aggregate
  `va_arg` helper owner.

Current object emission already has a prepared frame-slot local-load/store
hook for scalar sizes 1, 2, 4, and 8 when the prepared memory access is default
address space, nonvolatile, `base=frame_slot`, has a slot id, uses
base-plus-offset, has matching size/alignment, and has an encodable nonnegative
offset. Existing focused coverage includes slot-offset scalar locals and an
`i16` local store. The first supportable missing semantic packet is therefore
prepared frame-slot base-plus-subobject-offset scalar loads/stores where
`access.address.byte_offset > 0` is combined with the prepared slot offset and
the result remains RV64 12-bit stack-relative and aligned.

## Suggested Next

Delegate Plan Step 2 to add focused RV64 object-emission tests and any needed
implementation adjustment for prepared frame-slot base-plus-subobject-offset
scalar loads/stores:

- admit `i8`, `i16`, `i32`, `i64`/`ptr` local load/store when prepared memory
  metadata gives `base=frame_slot`, `frame_slot_id`, nonnegative
  `byte_offset`, matching size/alignment, and the absolute frame offset fits
  the RV64 signed 12-bit load/store immediate;
- prove a store and a load whose final address is `slot.offset_bytes +
  access.address.byte_offset`, not just a different slot base at offset `0`;
- keep fail-closed cases for missing slot id, pointer-value base,
  negative/out-of-range offset, oversized alignment, volatile, non-default
  address space, unsupported width, and aggregate/helper shapes.

## Watchouts

- Keep idea 354 open and inactive until child ideas 368-371 close or are
  intentionally superseded.
- Do not implement aggregate `va_arg`, byval homes, or terminator lowering from
  this plan.
- Do not hard-code representative offsets or source-case names; derive address
  facts from prepared metadata.
- Put analysis logs under `build/agent_state/`, not root-level canonical logs.
- Pointer-value local accesses through parameters are present in
  `src/20000217-1.c`, `src/20000121-1.c`, and `src/va-arg-13.c`; they should
  remain rejected by the frame-slot local-memory diagnostic unless the
  supervisor creates a separate pointer-memory owner.
- `src/va-arg-13.c` also needs vararg helper lowering and frame-slot address
  call-argument materialization; record those as downstream owners if the
  subobject-offset packet advances the direct frame-slot local-memory boundary.

## Proof

Audit-only packet; no build required and no root `test_after.log` written.

Prepared dumps and compact audit logs:

- `build/agent_state/368_step1_20000217-1.prepared-bir.txt`
- `build/agent_state/368_step1_20000217-1.memory-audit.txt`
- `build/agent_state/368_step1_20000121-1.prepared-bir.txt`
- `build/agent_state/368_step1_20000121-1.memory-audit.txt`
- `build/agent_state/368_step1_va-arg-13.prepared-bir.txt`
- `build/agent_state/368_step1_va-arg-13.memory-audit.txt`

Suggested Step 2 focused proof:

```sh
cmake --build build --target c4cll backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

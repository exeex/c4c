Status: Active
Source Idea Path: ideas/open/511_static_local_object_data_contract_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Static-Local Object-Data Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` rebuilt the static-local object-data evidence for
`src/930513-2.c` under
`build/agent_state/511_step1_static_local_object_data_evidence/930513-2/`.

Fresh semantic and prepared BIR dumps still expose three direct global accesses
to `__static_local_eq_0`: two loads and one store in `eq`. Prepared addressing
records all three with `offset=0`, `size=4`, `align=4`,
`base_plus_offset=yes`, `range_verdict=proven_in_bounds`, and
`layout_authority=unknown`.

The current RV64 object route still fails closed with:
`unsupported_global_data: prepared selected object-data contract status=missing_object_label object_size_bytes=0 emitted_byte_count=0 zero_fill_byte_count=0`.

Current `HEAD` has partial general selected object-data publication in
`src/backend/prealloc/object_data.cpp`: ordinary coherent globals can publish
`object_label`, `object_size_bytes`, `align_bytes`, initialized bytes, and
zero-fill counts, while unsupported ordinary globals get
`unsupported_but_coherent` records when they have a `link_name_id`, extent, and
alignment. The static-local representative falls out before that support can
be consumed because `Lowerer::lower_static_local_global` in
`src/frontend/hir/hir_types.cpp` creates `__static_local_eq_0` with a stable
display/storage name and initializer but leaves `link_name_id` invalid. The
prepared object-data publisher keys object identity on `bir::Global::link_name_id`;
with an invalid id it emits no object label, and RV64 lookup by
`global.link_name_id` observes null/missing selected object-data facts.

## Suggested Next

Execute Step 2: trace static-local storage identity from frontend static-local
lowering through LIR/BIR global publication into
`populate_prepared_object_data_plans`, and decide the minimal producer-side
boundary that should publish link-name identity, size, alignment, initialized
bytes, and zero-fill authority for function-scope static storage.

## Watchouts

- Keep the route limited to producer/prealloc object-data contract publication
  for function-scope static storage.
- Do not infer static-local object labels, extents, initialized bytes,
  zero-fill, alignment, relocations, or storage identity in RV64 from testcase
  names, symbol names, access facts, source shape, or magic constants.
- Keep RV64 selected object-data emission, stack-passed parameter homes,
  unrelated global access widths, GOT-required globals, thread-local storage,
  F128, and broad ABI work out of this plan.
- Preserve the existing general object-data guards for externs, TLS,
  GOT-required globals, unsupported initializer semantics, missing extents,
  missing bytes, and missing zero-fill. The Step 1 evidence points at producer
  identity publication for static locals, not an RV64 reconstruction route.

## Proof

- `cmake --build build --target c4cll` passed with no work to do.
- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/930513-2.c` passed; stdout/stderr/rc captured under `build/agent_state/511_step1_static_local_object_data_evidence/930513-2/`.
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/930513-2.c` passed; stdout/stderr/rc captured under `build/agent_state/511_step1_static_local_object_data_evidence/930513-2/`.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/930513-2.c -o build/agent_state/511_step1_static_local_object_data_evidence/930513-2/930513-2.o` failed as expected with exit code `2`; stderr captures the current `missing_object_label` selected object-data diagnostic.

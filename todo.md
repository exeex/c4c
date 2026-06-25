Status: Active
Source Idea Path: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reclassify Representative Blockers

# Current Packet

## Just Finished

Reviewed idea 368 after Step 2 admitted focused direct frame-slot
subobject-offset local memory but Step 3 reran all representatives and still
found the shared diagnostic:

`unsupported_local_memory_access: RV64 object route requires prepared
frame-slot base-plus-offset local memory addressing`

Lifecycle decision:

- Keep idea 368 active; it is not complete because no representative advanced.
- Preserve the Step 2 subobject-offset support as useful focused capability,
  but do not treat it as representative progress.
- Rewrite the active runbook around the first actual residual owner:
  pointer-value local-memory loads/stores.
- Create `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`
  for the separate frame-slot address call-argument materialization owner.

Evidence:

- `build/agent_state/368_step3_representatives.log`: total=3 passed=0
  failed=3.
- `build/agent_state/368_step1_20000217-1.memory-audit.txt`: `showbug`
  contains pointer-value i16 accesses from `%p.a` / `%p.b`; `main` also has
  local frame address materialization for `%lv.x` / `%lv.y` call arguments.
- `build/agent_state/368_step1_20000121-1.memory-audit.txt`: `doit` contains
  an i8 pointer-value load from `%p.id`.
- `build/agent_state/368_step1_va-arg-13.memory-audit.txt`: `dummy` contains
  pointer-value accesses from `%p.vap`; `test` also has frame-slot address call
  arguments for `%t7` / `%t14`.

## Suggested Next

Supervisor should delegate an executor packet for Plan Step 2: admit the first
supportable pointer-value scalar local-memory load/store path in RV64 object
emission.

Suggested packet boundary:

- Owned implementation files:
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, and `todo.md`.
- Target prepared shape: `base=pointer_value` local-memory access where the
  pointer source is already available in a GPR, offset is RV64-encodable,
  width/alignment are scalar and supported, address space is default, and the
  access is nonvolatile.
- Prove at least one success fixture matching i8/i16/pointer-width
  pointer-value load/store and fail-closed coverage for missing pointer
  register, unsupported width, bad alignment, non-default address space,
  volatile, and dynamic/aggregate shapes.
- Keep frame-slot address call-argument materialization out of the packet; that
  belongs to idea 372.

## Watchouts

- Do not expand idea 368 into call-argument address materialization. If a
  representative advances to `missing_frame_slot_arg_publication` or a similar
  call argument boundary, hand it to idea 372.
- Do not implement aggregate `va_arg`, byval homes, terminator lowering, or
  source-shape shortcuts from this plan.
- The shared local-memory diagnostic currently masks multiple owners; inspect
  prepared facts before assigning another implementation packet.
- Put analysis logs under `build/agent_state/`, not root-level canonical logs.
- Keep `test_after.log` reserved for the supervisor-delegated proof command.

## Proof

Lifecycle-only route correction. No build required.

Validation to run before commit:

- `git diff --check`
- `git status --short` scope check for `plan.md`, `todo.md`,
  `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`, and
  `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`

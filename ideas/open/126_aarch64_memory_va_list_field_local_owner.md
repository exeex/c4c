# 126 AArch64 Memory Va List Field Local Owner

## Goal

Extract or isolate the AArch64-local `va_list` field memory handling currently
embedded in `src/backend/mir/aarch64/codegen/memory.cpp`, preserving prepared
variadic authority and existing memory lowering behavior.

## Why This Exists

Idea 124 found that `memory.cpp` is now mostly target-local memory emission
after the remaining shared pending store-global producer gap was split into
idea 125. With idea 125 closed, the next useful memory cleanup is local clarity,
not shared-authority migration.

The `va_list` field memory cluster is a good first physical-split candidate:
it is target-local, ABI-sensitive, and has a bounded helper group:

- `parse_va_list_field_suffix`
- `VaListFieldAddress`
- `find_va_list_field_address`
- `make_va_list_field_memory_operand`
- `make_va_list_field_load_memory_instruction_record`
- `make_va_list_field_store_memory_instruction_record`
- `va_list_field_cursor_update_producer`
- `make_va_list_field_cursor_update_machine_instruction`

These helpers consume prepared variadic entry plans, prepared value homes,
storage plans, and AArch64 register conversion. They should not create new
shared facts or select variadic homes. The cleanup should make this local owner
clearer while leaving shared/prepared ownership untouched.

## Owner Boundary

Shared prepared/BIR code owns:

- variadic entry plans;
- `va_list` layout fields;
- variadic helper operand homes;
- value-home and storage-plan facts.

AArch64 memory lowering owns:

- mapping prepared `va_list` field facts to AArch64 memory operands;
- ABI-sensitive field load/store record construction;
- cursor-update instruction emission;
- scratch register choice for cursor update;
- final machine record construction and diagnostics.

## In Scope

- Isolate the current `va_list` field memory helper cluster behind a local
  owner or private helper surface.
- Keep `lower_memory_instruction` as the consumer that asks the local owner
  whether a load/store should use the special `va_list` field route.
- Preserve the existing fallback to ordinary prepared memory lowering when the
  `va_list` field route does not apply.
- Preserve target-local cursor-update emission and scratch selection.
- Keep public `memory.hpp` surface no wider than necessary.

## Out Of Scope

- Moving variadic entry planning, helper operand homes, `va_list` layout, or
  va-arg payload/home facts out of shared prepared code.
- Moving AArch64 ABI field layout, register spelling, scratch policy, or
  memory operand emission into shared code.
- Reworking `src/backend/mir/aarch64/codegen/variadic.cpp` except for include
  fallout if genuinely necessary.
- Reopening ideas 57, 102, 112, 121, 124, or 125.
- Broad `memory.cpp` splitting or line-count-only cleanup.

## Likely Files

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp` only if the owner needs a narrow
  declaration
- a new private AArch64 memory helper file/header if that produces a cleaner
  boundary without exporting broad internals
- build metadata for any new translation unit
- focused backend/AArch64 tests covering variadic `va_list` field memory paths

## Proof Route

- Characterize the current `va_list` field load/store and cursor-update routes
  before moving code.
- Run focused backend/AArch64 tests covering:
  - variadic `va_start` / `va_arg` memory interaction;
  - `va_list` field loads;
  - `va_list` field stores;
  - cursor-update store emission;
  - ordinary memory fallback when a slot is not a prepared `va_list` field.
- Include existing regression cases around `00204` or nearby variadic aggregate
  coverage if they exercise the touched route.
- Escalate to broader backend validation if helper extraction changes shared
  headers or memory record construction used outside the local owner.

## Acceptance And Completion Criteria

- The `va_list` field memory cluster is behind a clear AArch64-local owner or
  private helper boundary.
- No shared prepared/BIR authority moves or changes semantics.
- `lower_memory_instruction` remains behavior-equivalent for both special
  `va_list` routes and ordinary memory fallback.
- Focused tests pass without expectation downgrades.
- Any new file or public declaration has a narrow reason tied to this owner.

## Reviewer Reject Signals

- The implementation reselects variadic homes, recomputes `va_list` layout, or
  creates new shared variadic authority.
- The implementation moves AArch64 ABI field offsets, scratch registers, or
  memory instruction spelling into shared prepared code.
- The route claims progress through physical movement while the same mixed
  helper cluster remains implicit or more widely exported.
- Tests are weakened, supported variadic paths are marked unsupported, or
  expectations are rewritten to hide changed memory behavior.
- The slice sweeps in unrelated f128/i128 transport, store-source publication,
  prepared memory identity, or frame-slot materialization work.

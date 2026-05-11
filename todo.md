# Current Packet

Status: Active
Source Idea Path: ideas/open/169_route_local_identity_domain_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Route-Local String Authority

## Just Finished

Step 1 inventory completed for route-local string authority in the requested
backend/LIR/BIR/prealloc/MIR areas.

Candidate classifications:
- Display-only: `bir::Value::name`, `bir::Param::name`, BIR printer render paths,
  prepared printer `maybe_*` helpers, target register spellings, inline asm text,
  and x86 final label/register rendering after prepared ids are already chosen.
- Generated local handle: `FnCtx::local_slots`, `FnCtx::param_slots`,
  `FnCtx::continue_redirect`, LIR `str_pool_map`, BIR aggregate leaf slot names,
  `CallInst::sret_storage_name`, prepared physical-register strings, spill
  register names, and MIR temporary render maps.
- Validation key: BIR `validate_local_slot_names`, BIR `find_local_slot`, block
  label uniqueness/fallback checks, structured type spelling lookup, prepared
  stack slice-family maps, and MIR handoff checks that compare BIR names to
  prepared records.
- Cross-pass local identity: BIR block label ids, prepared `FunctionNameId`,
  `BlockLabelId`, `ValueNameId`, `SlotNameId`, `PreparedValueId`,
  `PreparedObjectId`, `PreparedFrameSlotId`, prepared memory-access keys
  `(BlockLabelId, inst_index)`, value homes, liveness, regalloc, control-flow,
  out-of-SSA, and addressing records.
- Compatibility payload: LIR/BIR raw extern name fallbacks, string-pool constant
  names, BIR global/function raw-name fallbacks where `LinkNameId` is invalid,
  `MemoryAddress::base_name` when paired with `LinkNameId`/`BlockLabelId`, raw
  phi/terminator label strings when `BlockLabelId` is invalid, and legacy
  struct/type spelling shadows.

Selected Step 2 family: BIR local-slot authority.

Files:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/aggregate.cpp`
- `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/stack_layout/regalloc_helpers.cpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/x86/module/module.cpp`

Current string authority behavior: BIR declares `LocalSlot::name` as the slot
handle, `LoadLocalInst::slot_name` and `StoreLocalInst::slot_name` reference it
by rendered spelling, BIR validation finds slots with `slot.name == slot_name`,
lowering maps pointer/value/aggregate state by the same spelling, and prealloc
later interns those spellings into `PreparedNameTables::slot_names`.

Proposed new boundary/id authority: add or reuse a typed route-local slot id at
the BIR local-slot boundary, most likely by extending `bir::NameTables` with
`SlotNameTable` and carrying optional/required `SlotNameId` on `LocalSlot`,
local load/store references, local-slot `MemoryAddress` bases, and sret storage.
Keep the string spelling as rendering/compatibility payload and intern it once
at lowering time; prealloc should consume the id rather than rediscover the slot
through spelling.

Why this is in scope: the family is function-local/backend-local, not
parser/sema/HIR identity; it is not link-visible ELF/global symbol identity; it
does not require `TextId` or `LinkNameId`; and it currently connects multiple
backend structures through rendered local spelling.

Step 2 proof command: build the compiler target plus the narrow BIR/prealloc
tests selected by the supervisor, then preserve output in `test_after.log`.
Recommended starting point:
`cmake --build build --target c4c && ctest --test-dir build -R "bir|prealloc|prepared|x86" --output-on-failure > test_after.log 2>&1`.

## Suggested Next

Execute Step 2 for BIR local-slot authority: introduce the smallest typed slot
id boundary, update BIR validation and lowering for local slot declaration and
load/store references, and keep output spelling stable through existing slot
name payloads.

## Watchouts

- Do not use `TextId` or `LinkNameId` for route-local temporary domains.
- Do not claim progress from display-only refactors or cosmetic output tests.
- Preserve final rendered spelling unless an intentional rendering change is
  documented.
- Keep routine inventory notes in this file unless source intent actually
  changes.
- Avoid link-visible global/function raw-name fallback paths; they are outside
  this route-local cleanup.
- Existing prepared ids are already the stronger side of the boundary; the weak
  side is BIR slot declaration/reference matching before prepared interning.

## Proof

Inventory-only packet. Ran `git diff --check -- todo.md`; result: pass. No
`test_after.log` was produced because the delegated proof command is diff-only
for this packet.

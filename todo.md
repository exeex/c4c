Status: Active
Source Idea Path: ideas/open/107_lir_struct_name_id_type_ref_mirror.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect LirTypeRef Construction And Struct Type Spelling

# Current Packet

Prepare Step 1 by inspecting `LirTypeRef` construction and struct type spelling
paths before adding the structured mirror.

## Just Finished

Lifecycle activation only. No implementation packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: inspect `LirTypeRef` construction, identify
struct-valued type-ref creation points, and record the minimal edit target for
adding optional `StructNameId` metadata.

## Watchouts

- Keep rendered `LirTypeRef` strings as printer authority.
- Preserve existing `llvm_ty`, `llvm_alloca_ty`, `TypeSpec::tag`, and rendered
  type string paths.
- Do not expand into globals/functions/extern signature surfaces from idea
  108.
- Do not move layout lookup to `StructNameId`; that belongs to idea 109.
- Any major string-only type-ref surfaces discovered during inspection should
  be recorded here rather than silently broadening the current packet.

## Proof

Not run. Lifecycle-only activation.

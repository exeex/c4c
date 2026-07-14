# Current Packet

Status: Active
Source Idea Path: ideas/open/749_lir_selected_memcpy_current_function_pointer_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define bounded current-function pointer and object authority

## Just Finished

- Lifecycle switch from blocked idea 748 is complete; 748 records its
  interrupted Step 1, accepted baseline, and exact return point.

## Suggested Next

- Execute Step 1 only: define the smallest typed current-function pointer
  definition, local object, owner, and live-lifetime authority needed by the
  selected byval parameter pointer and destination alloca. Do not populate or
  verify a memcpy operation.

## Watchouts

- The selected future consumer is only `src/codegen/lir/hir_to_lir/lvalue.cpp:279-281`.
  No text-derived authority, generic expansion, Raw-BIR/importer work, or
  other memcpy producer belongs in this packet.

## Proof

- After implementation: fresh build plus focused typed definition/object/
  lifetime coverage. The supervisor owns matching regression logs and broader
  checks.

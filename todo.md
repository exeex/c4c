# Current Packet

Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the bounded native memory/VA authority boundary

## Just Finished

- 753 Step 1 completed the bounded native memory/VA producer evidence record
  in `docs/lir_to_new_bir_remaining_coverage/753_memory_va_native_authority_evidence_boundary.md`.
  It identifies the 752 local pointer/object/owner/type/live substrate, keeps
  the selected memcpy row historical only, and records the first missing
  operation-local authority fact for memcpy, memset, va_start, va_end,
  va_copy, and va_arg.

## Suggested Next

- Execute 753 Step 2 only: add native-only opt-in producer/schema/verifier
  authority for the bounded memory/VA family from the Step 1 evidence record.
  Do not begin Raw-BIR receiver work or select a future receiver row.

## Watchouts

- Preserve the selected memcpy row as historical evidence only. Do not derive
  semantic facts from names, operand/rendered text, LLVM, testcase shape,
  `monostate`, or unresolved classification; unconverted forms remain fail
  closed or explicitly compatibility-only. The 752 local authority record is
  substrate, not a generic memory/VA selector or resolver.

## Proof

- `git diff --check` passed for the Step 1 documentation-only packet. The
  evidence note was inspected for all six named families and their explicit
  first missing authority fact; no build or test subset was required.

Status: Active
Source Idea Path: ideas/open/867_lir_memory_va_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish direct-local va_start native pointer authority

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by tracing the current memory/VA/object/lifetime
authority producers and selecting exactly one bounded seam.

Selected seam: direct-local `va_start` native pointer authority on
`LirVaStartOp`.

Current gap: the LIR model has an opt-in native VA pointer carrier and verifier
checks, but the current 867 route still needs a single published, documented
handoff that names only the `va_start` destination `va_list` pointer authority
as the future 734 receiver row. Downstream prepared-BIR VA helper homes and
Raw-BIR receiver work remain out of this packet.

Exact producer: `StmtEmitter::native_direct_local_va_pointer` in
`src/codegen/lir/hir_to_lir/call/builtin.cpp` selects only a direct local
`TB_VA_LIST` decl-ref with rank 0, no pointer indirection, a current
`local_object_authorities` entry, and a matching `local_slots` entry. The
`BuiltinId::VaStart` branch then emits `LirVaStartOp` with
`requires_native_memory_va_authority = true`, `ap_ptr` as the slot SSA operand,
and `ap_authority = LirMemoryVaPointerAuthority{local_pointer}`.

Expected native facts: `LirVaStartOp.ap_ptr.value_id()` equals
`ap_authority.local_pointer.pointer_definition`; the local pointer records a
valid `LirObjectId`, current function `LinkNameId` owner, pointer type `ptr`,
native `va_list` pointee type, and `live = true`.

Owner/type checks: `verify_native_memory_va_authority` in
`src/codegen/lir/verify.cpp` must continue to require current-function local
object authority, an SSA operand matching the pointer definition, a modeled
pointer definition, and agreement with canonical local pointer facts for object,
owner, pointer type, pointee type, and liveness. Unselected `LirVaStartOp`
instances must reject stray authority fields.

In/out boundary: in scope is only the native LIR `va_start` destination pointer
authority carrier, producer population, verifier checks, and its one handoff
row. Out of scope are `va_end`, `va_copy`, `va_arg`, memcpy/memset,
prepared-BIR helper-home publication, Raw-BIR receiver/importer work, and any
textual recovery from rendered operands or LLVM intrinsic spelling.

Files to inspect next: `src/codegen/lir/ir.hpp`,
`src/codegen/lir/hir_to_lir/call/builtin.cpp`,
`src/codegen/lir/verify.cpp`,
`tests/backend/bir/backend_lir_selected_pointer_authority_test.cpp`, and the
future handoff document location chosen by the supervisor/plan-owner for
Step 4.

Focused proof candidate: rebuild the backend test binary and run
`backend_lir_selected_pointer_authority_test`, specifically covering
`test_direct_local_va_lifecycle_populates_authority` and
`test_native_memory_va_authority_verifier_boundary`.

Exact future 734 receiver row: receive one typed Raw-BIR row for
`LirVaStartOp` direct-local destination `va_list` authority, carrying the
current-function source value id for `ap_ptr`, local object id, owner function
id, pointer type `ptr`, native `va_list` pointee type, and live-at-site bit;
reject invalid/missing pointer definition, foreign owner, dead local object,
type mismatch, relation mismatch between `ap_ptr` and authority, and authority
fields on unselected `va_start`.

Rejected candidates:
- Local-object and selected hoisted alloca authority: rejected because local
  object rows are explicitly accepted/stale-not-to-reopen in the triage docs,
  and `test_selected_hoisted_alloca_authority_receipt_and_rejections` already
  covers a Raw-BIR receipt/rejection route.
- VLA stack-save/restore authority: rejected because selected VLA stack
  save/restore rows are explicitly accepted history and out of 867 scope.
- Selected fixed-aggregate byval memcpy and AMD64 overflow aggregate memcpy:
  rejected because selected memcpy receipt history is excluded; the AMD64
  overflow carrier already has focused producer/verifier and Raw-BIR interface
  coverage.
- `va_end`, `va_copy`, and `va_arg`: rejected for this packet because they are
  adjacent VA lifecycle facts. Selecting them would broaden the seam beyond one
  exact row; they can only follow as separate single-owner rows.
- Prepared-BIR variadic helper operand homes: rejected as a different first
  owner/receiver layer. Those tests publish prepared helper-home facts after
  LIR authority and must not become the 867 LIR producer seam.
- Raw-BIR-only memory lowering rows for alloca/load/store/GEP and provenance:
  rejected because 867 is first-owner LIR producer/schema/verifier work, not a
  Raw-BIR receiver/importer packet.

## Suggested Next

Start `plan.md` Step 2 for the selected seam only: publish the direct-local
`LirVaStartOp` native pointer authority as the bounded LIR authority row, or
confirm that existing producer/verifier fields need only a handoff document and
focused proof before Step 4.

## Watchouts

Do not edit Raw-BIR receiver/importer code, reopen accepted local-object/VLA/
memcpy history, or derive authority from operand spelling, printer output,
LLVM text, rendered names, or testcase identity. Keep `va_start` separate from
`va_end`, `va_copy`, `va_arg`, memset, memcpy, prepared-BIR helper homes, and
Raw-BIR receiver work.

## Proof

Step 1 inventory/selection only; no build or implementation proof required.
Validation for this packet: `git diff --check`.

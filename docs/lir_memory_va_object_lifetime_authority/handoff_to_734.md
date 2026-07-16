# LIR Memory/VA/Object/Lifetime Handoff To Idea 734

Status: accepted producer handoff for idea 867
Source Idea: `ideas/open/867_lir_memory_va_object_lifetime_authority.md`
Receiver Idea: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Selected Row

This handoff authorizes exactly one future 734 receiver row:

- `LirVaStartOp` direct-local destination `va_list` pointer authority.

The selected authority is the LIR producer/schema/verifier fact carried by
`LirVaStartOp.ap_authority` when
`LirVaStartOp.requires_native_memory_va_authority` is true. It must not be
reconstructed from `ap_ptr` spelling, printer output, LLVM intrinsic spelling,
prepared-BIR helper homes, Raw-BIR importer state, or testcase identity.

## Producer Contract

The producer is `StmtEmitter::native_direct_local_va_pointer` in
`src/codegen/lir/hir_to_lir/call/builtin.cpp`, consumed by the
`BuiltinId::VaStart` branch.

The selected producer admits only a direct local declaration reference with:

- type `TB_VA_LIST`;
- pointer level `0`;
- array rank `0`;
- a current `local_object_authorities` entry;
- a matching `local_slots` entry;
- a live current-function local pointer whose owner is the current
  `LirFunction.link_name_id`;
- pointer type `ptr`;
- a valid pointer definition and local object id.

When accepted, the producer emits `LirVaStartOp` with:

- `requires_native_memory_va_authority = true`;
- `ap_ptr = LirOperand::ssa(slot, pointer.pointer_definition)`;
- `ap_authority = LirMemoryVaPointerAuthority{local_pointer}`.

## Receiver Fields

734 may receive only these typed facts for this row:

- `LirVaStartOp.ap_ptr.value_id()` as the source pointer value id;
- `LirVaStartOp.ap_authority.local_pointer.pointer_definition`;
- `LirVaStartOp.ap_authority.local_pointer.object`;
- `LirVaStartOp.ap_authority.local_pointer.owner`;
- `LirVaStartOp.ap_authority.local_pointer.pointer_type`;
- `LirVaStartOp.ap_authority.local_pointer.pointee_type`;
- `LirVaStartOp.ap_authority.local_pointer.live`.

The receiver must require `ap_ptr.value_id()` to equal
`ap_authority.local_pointer.pointer_definition`.

## Verifier Contract

`verify_native_memory_va_authority` in `src/codegen/lir/verify.cpp` is the
accepted LIR verifier boundary. For the selected `LirVaStartOp` row it must
continue to require:

- current-function local-object authority via `verify_local_object_authority`;
- an SSA `ap_ptr` operand with a value id matching the authority's pointer
  definition;
- a modeled pointer definition instruction;
- canonical local pointer fact agreement for object, owner, pointer type,
  pointee type, and liveness;
- rejection of selected `va_start` without `ap_authority`;
- rejection of `ap_authority` fields on unselected `va_start`.

## Accepted Tests

Focused proof is in `backend_lir_selected_pointer_authority`.

The selected positive route is
`test_direct_local_va_lifecycle_populates_authority`, which verifies that
direct-local `va_start` publishes the native pointer authority and that
`ap_ptr.value_id()` matches the authority pointer definition.

Malformed and boundary proof is in
`test_native_memory_va_authority_verifier_boundary`, which covers selected
`va_start` missing-authority rejection and unselected `va_start` authority-field
rejection through the shared native memory/VA verifier. The same verifier
helper also rejects foreign owner, dead local object, type mismatch, and
canonical pointer disagreement for selected native memory/VA pointer facts.

Accepted command:

```sh
{ cmake --build build && ctest --test-dir build -R '^backend_lir_selected_pointer_authority$' --output-on-failure; } > test_after.log 2>&1
```

Result: successful build/no-op build and 1/1 passing test.

## 734 Return Point

After idea 867 closes, reactivate idea 734 for one bounded receiver packet:

**Receive selected direct-local `LirVaStartOp` destination `va_list` authority.**

That packet may add only the minimum typed Raw-BIR container/importer/verifier
support needed to consume this selected row transactionally. It must reject:

- missing or invalid `ap_authority`;
- invalid or mismatched `ap_ptr` value id;
- foreign owner;
- dead local object;
- pointer type or pointee type mismatch;
- canonical local pointer disagreement;
- authority fields on unselected `va_start`.

## Explicit Non-Goals

This handoff does not authorize:

- Raw-BIR receiver work inside idea 867;
- `va_end`, `va_copy`, or `va_arg` receiver rows;
- memcpy, memset, local-object, VLA stack-save/restore, or selected memcpy
  reopening;
- prepared-BIR helper-home publication or target backend lowering;
- deriving authority from operand spelling, printer output, LLVM text,
  intrinsic names, rendered names, or testcase identity.

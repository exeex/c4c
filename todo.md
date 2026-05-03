# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
temporarily removed the `TypeSpec::tag` field from
`src/frontend/parser/ast.hpp` and ran `cmake --build --preset default`.
The probe failed in codegen, then the probe edit was reverted.

First remaining boundary:
- `src/codegen/shared/llvm_helpers.hpp:444-445`,
  `562`, `601-602`, and `619-620` still directly read `TypeSpec::tag`.
  These cover final LLVM aggregate spelling in `llvm_ty`,
  `llvm_alloca_ty`, and `llvm_field_ty`, plus the shared `sizeof_ts`
  legacy layout helper.

Additional same-wave residuals visible after that header instantiates:
- `src/codegen/lir/hir_to_lir/call/args.cpp:72` and
  `call/target.cpp:76` retain explicitly named rendered-tag compatibility
  fallback for call `LirTypeRef` construction.
- `src/codegen/lir/hir_to_lir/call/args.cpp:183` and
  `call/vaarg.cpp:197` still use `tag` as a named-aggregate presence check
  before variadic aggregate handling.

## Suggested Next

Next coherent packet: migrate the shared codegen helper boundary by separating
final LLVM aggregate spelling from semantic `TypeSpec::tag` access. Start with
`llvm_helpers.hpp` final-spelling helpers and `sizeof_ts`, then revisit the
remaining call/va_arg compatibility presence checks exposed by the probe.

## Watchouts

- The deletion build is intentionally not committed; `TypeSpec::tag` is restored
  in the working tree.
- `llvm_helpers.hpp:444` remains both a final LLVM spelling boundary and a
  semantic dependency for callers that have no separate aggregate identity
  carrier. Do not remove emitted LLVM type text.
- `sizeof_ts` in `llvm_helpers.hpp` is still a semantic layout helper and needs
  structured owner metadata or an explicit compatibility contract before field
  deletion can proceed.
- The call/va_arg residuals exposed by this probe are compatibility/presence
  checks, not proof that the previous structured carrier migrations failed.

## Proof

Deletion probe command:
`cmake --build --preset default > test_after.log 2>&1`

Expected probe failure was captured in `test_after.log`; first compile failure
is `src/codegen/shared/llvm_helpers.hpp:444` because `TypeSpec::tag` no longer
exists. The temporary `ast.hpp` edit was reverted after the probe, and
`cmake --build --preset default` then passed to confirm the workspace returned
to a buildable state.

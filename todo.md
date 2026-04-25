Status: Active
Source Idea Path: ideas/open/114_lir_type_text_authority_demotion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Demote One Safe Structured-First Consumer Family

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the selected `LirCallOp` verifier family.

Converted verifier checks:

- `LirCallOp.return_type` now uses a shared verifier-local declared-struct
  mirror check: if the return `LirTypeRef` already carries `StructNameId`, that
  structured mirror is accepted after the existing module/type-ref consistency
  proof; if no mirror id exists, declared struct text is the fallback boundary
  that still requires a matching `StructNameId`.
- `LirCallOp.arg_type_refs` now routes each mirrored argument through
  verifier-local structured-first validation: declared struct call text is
  resolved to `StructNameId` and compared against the mirrored id before the
  formatted call-text proof runs.
- Existing formatted argument text parsing, argument-count validation, and the
  final `shadow != call argument type` proof remain in place. This preserves
  non-mirrored/incomplete-call fallback behavior and keeps emitted call text
  unchanged.

## Suggested Next

Run supervisor review or choose the next small demotion family only after
checking whether this `LirCallOp` slice satisfies the source idea without
expanding into globals, extern declarations, function signatures, raw
`LirTypeRef` equality, backend, or MIR.

## Watchouts

- `LirGlobal::llvm_type`, `LirExternDecl::return_type_str`,
  `LirFunction::signature_text`, raw `LirTypeRef` equality/output semantics,
  backend, MIR, printer output, and test expectations were intentionally not
  changed.
- `LirCallOp.arg_type_refs` remains absent for variadic/incomplete parse paths;
  verifier validation only runs when mirrors exist.
- Formatted call text is still parsed and compared as fallback/proof, so this
  slice does not permit mirror/text drift in emitted call sites.

## Proof

Ran the supervisor-selected proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|frontend_lir_|positive_split_llvm_)') > test_after.log 2>&1`

Result: passed. `test_after.log` shows the build completed and 5/5 selected
tests passed.

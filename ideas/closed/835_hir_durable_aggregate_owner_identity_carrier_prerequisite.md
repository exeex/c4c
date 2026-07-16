# HIR Durable Aggregate Owner Identity Carrier Prerequisite

Status: Closed
Type: minimum upstream HIR aggregate-owner identity carrier prerequisite
Blocked Parent: `ideas/open/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`, Step 2 native owner canonicalization repair

## Goal

Provide the smallest HIR-owned durable aggregate owner/spelling/qualified
identity carrier—or canonical record-owner key—that survives parser storage
teardown and lets `lir_owned_type_spec` resolve the valid matching module owner
without dereferencing parser memory.

## Why This Exists

834 established that the LIR-local lookup needs canonical aggregate declaration
and namespace identity, but its parser-backed `record_def` and qualifier
carriers can already be stale during normal HIR lowering. Sanitizing those
carriers preserves the malformed-owner guard but makes valid C `pr51323.c` and
C++ `operator_struct_byval_param.cpp` miss their matching module owner.
Reading them before cleanup restores those valid paths but faults in
`typespec_aggregate_owner_key`. The stable identity must therefore be copied or
canonicalized at the HIR ownership/copy boundary, not reconstructed after parser
storage dies.

## In Scope

- Diagnose the precise parser-to-HIR ownership/copy boundary for aggregate
  `record_def`, spelling, qualifier, and canonical owner identity used by
  `typespec_aggregate_owner_key` and `lir_owned_type_spec`.
- Define and implement only the minimum HIR-owned durable carrier or canonical
  record-owner key needed by LIR's module-owner lookup.
- Preserve invalid, incoherent, genuinely ownerless, foreign, and
  wrong-namespace aggregate-owner rejection.
- First prove the nearby valid C and C++ consumers and the existing malformed
  owner guard; leave later broad/full-suite decisions to 834/831.

## Out Of Scope

- A broad HIR aggregate metadata, parser, symbol-table, or module-table rewrite.
- LIR canonicalization completion beyond consuming the new stable carrier;
  834 resumes that bounded lookup repair after this prerequisite accepts.
- Test/harness/expectation changes, unsupported markers, allowlists, filtering,
  baseline claims, or reopening closed 832/833.

## Acceptance Criteria

1. The first diagnostic packet identifies the exact ownership/copy boundary and
   the smallest durable information needed for canonical aggregate owner lookup.
2. The implementation stores or derives that information in HIR-owned lifetime
   without retaining a parser-memory dereference path.
3. Valid C `pr51323.c` and C++ `operator_struct_byval_param.cpp` consumers and
   the existing malformed-owner guard prove the carrier preserves valid owner
   lookup while rejecting invalid/ownerless inputs.
4. The completion record returns only 834 to Step 2, where it consumes the
   carrier and then performs its own Step 3 focused proof; this idea makes no
   baseline-clearance claim.

## Accepted Step 1 Diagnosis

- The exact parser-to-HIR copy boundary is
  `Lowerer::qtype_from(const TypeSpec&, ValueCategory)` in
  `src/frontend/hir/hir_types.cpp:469-474`; valid `lower_function` and
  `append_callable_param` routes reach it.
- Parser-backed `record_def`, qualifier segments, and qualifier text IDs are
  stale after parser teardown. Raw parser scalar `TextId`s cannot be reused as
  HIR link-name IDs because their values can collide.
- Existing conversion at `src/frontend/hir/hir_build.cpp:44-110` interns AST
  spelling into HIR module `link_name_texts`; the owner index requires a
  complete `HirRecordOwnerKey`. Step 2 must therefore store an optional,
  HIR-owned canonical owner identity/key at HIR copy time for a valid complete
  struct/union record definition only: owned qualifier vector plus canonical
  context/global/declaration IDs. The LLVM owner-key helper should prefer that
  complete durable key; parser-record fallback is pre-HIR only.
- The design must retain missing, incoherent, foreign, wrong-namespace, and
  ownerless rejection. It must not use a tag fallback.

## Reviewer Reject Signals

- Reject retaining or renaming a parser pointer as the purported durable
  carrier, or any route that can still dereference parser memory after teardown.
- Reject a broad aggregate metadata rewrite when a smaller HIR-owned canonical
  owner key or spelling/qualified-identity carrier suffices.
- Reject named-test or suite-specific lookup exceptions, rendered-text probes,
  expectation weakening, unsupported markers, filtering, or harness changes.
- Reject accepting valid C/C++ lookup while weakening invalid, foreign,
  wrong-namespace, or ownerless rejection.
- Reject reopening 832/833, claiming full baseline clearance, or absorbing
  834's later LIR lookup/proof work into this prerequisite.

## Completion Record

Disposition: capability complete; closed after accepted supervisor evidence.

- Step 2 implementation is accepted in `932c3339b` (`hir: retain canonical
  aggregate owner identity`). It publishes the minimum HIR-owned canonical
  aggregate owner identity at the HIR copy boundary; LIR consumers no longer
  need parser-memory dereferences for that identity.
- Step 3 accepted proof: `cmake --preset default && cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R
  '^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_positive_sema_operator_struct_byval_param_cpp|llvm_gcc_c_torture_src_pr51323_c)$'`.
  The matching regression guard recorded 39/42 before and 42/42 after, with no
  new failures.
- Return path: resume parent 834 at Step 2 only. 834 owns consuming the
  carrier, its focused proof, and the eventual return to 831; this prerequisite
  does not claim baseline clearance.

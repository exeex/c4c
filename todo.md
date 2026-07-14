# Current Packet

Status: Active
Source Idea Path: ideas/open/743_lir_i686_long_width_policy_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish the structured target-aware width policy

## Just Finished

- Plan Step 1 inventory is complete. The structured authority/conflict matrix
  is: (1) `src/target_profile.hpp` enumerates I686 and LP64-capable X86_64,
  Aarch64, and Riscv64 profiles; C `long`/`unsigned long` must be 32 bits on
  I686 and 64 bits on those LP64 profiles. (2) Shared LIR production currently
  conflicts: `src/codegen/shared/llvm_helpers.hpp::llvm_base` and its callers
  `llvm_ty`/`llvm_value_ty` map both bases unconditionally to `i64`, despite
  `hir_to_lir.cpp` setting the structured active `TargetProfile`. (3) LIR
  verification repeats the unconditional policy in
  `verify.cpp::exact_plain_scalar_mirror`: it demands i64 for every plain
  scalar except `int`/`unsigned int`, then
  `verify_plain_fixed_scalar_parameter_relationship` applies it to logical,
  signature, and typed-mirror parameter authority.
- New-BIR already consumes structured profile authority in the opposite
  direction: `src/backend/bir/lir_to_bir.cpp::lower_constant_type` maps
  `TB_LONG`/`TB_ULONG` to 32 only when `module.target_profile.arch == I686`,
  otherwise 64. `lower_signature_type` uses that authority and rejects a
  disagreeing typed mirror; `lower_function_parameter_types` feeds it into
  function receipt, `lower_signature_return_info`/`lower_return_info_from_type`
  feed return receipt, and global lowering reaches it through
  `lower_global_compatibility_type`/`lower_global_type`. The same receiver
  therefore accepts I686 long returns/globals as i32 while current producer
  and verifier publish/require i64 parameter mirrors.
- Existing focused evidence is in
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp` for
  LIR signature type-ref production/verification, and
  `tests/backend/bir/backend_lir_to_bir_interface_test.cpp`: direct scalar
  returns assert I686 `long` i32 (around `test_direct_scalar_signature_receipt`),
  globals assert I686 `long` i32 (around
  `test_scalar_global_type_authority_without_mirror`), while plain parameter
  rejections deliberately keep `long`/`unsigned long` blocked pending 743.
  Neighboring `int`, `long long`, float/double, and pointer cases are present
  in those receipt/rejection families and remain regression obligations.

## Suggested Next

- Execute Step 2 only: introduce one shared structured helper such as
  `long_width_bits(const TargetProfile&, TypeBase)` at the shared LLVM type
  policy seam, then make LLVM production, `exact_plain_scalar_mirror`, and
  new-BIR signature/return/global receipt use it. Do not use rendered text,
  triples, names, or testcase branches as authority.

## Watchouts

- Do not use rendered type text, target-triple spelling, names, or testcase
  identity as width authority. Preserve I686 C `long` at 32 bits and LP64 at
  64 bits; do not absorb unrelated idea-734 receiver work.
- Other open candidates remain ineligible for this activation: 732 and
  737-740 are parked pending post-734 order, 734 is blocked on 744, 744 is
  blocked on open idea 746, and 746 has no supported legal source carrier.
- Later proof must build the focused targets with `cmake --build build --target
  frontend_lir_function_signature_type_ref_test backend_lir_to_bir_interface_test`
  and run `ctest --test-dir build --output-on-failure -R
  '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_interface)$'`.
  Extend those families for I686 and one LP64 positive plus malformed mirror
  rejection across parameter, return, and global receipt; retain their current
  `int`, `long long`, floating, and pointer checks. Relevant backend proof is
  the same `backend_lir_to_bir_interface` CTest because it lowers both Raw and
  Canonical BIR transactionally.

## Proof

- `git diff --check` passed. No build or test run was required for this
  read-only inventory; the named build and CTest commands are the required
  later code-packet proof, followed by the supervisor-selected regression
  checkpoint before handoff.

# AArch64 Uninitialized Local Slot Runtime Residual

Status: Closed
Created: 2026-05-20
Split From: ideas/closed/334_aarch64_scalar_machine_node_operand_fact_printing.md
Closed: 2026-05-20

## Goal

Localize and repair the AArch64 runtime mismatch where generated code for
`c_testsuite_aarch64_backend_src_00164_c` reads values from stack slots that
appear not to have been initialized or published before use.

## Why This Exists

After scalar machine-node operand fact printing was repaired in idea 334,
`c_testsuite_aarch64_backend_src_00164_c` advanced past the old scalar `mul`
printer diagnostic:

```text
scalar mul/div/rem node has incomplete printable rhs facts
```

The current failure is a later runtime mismatch. The first visible output
mismatch is at `tests/c/external/c-testsuite/src/00164.c:28`, where the test
expects `46` and the generated program prints `-635898024`. Nearby generated
assembly reads stack locations around `sp+#148`, `sp+#152`, and `sp+#156`;
those locations appear to be uninitialized in the observed route. Later output
also diverges where `1916` is expected.

This is not a scalar printer fact problem anymore. It needs a fresh owner
classification across value publication, local-slot initialization, frame-slot
home assignment, spill/reload sequencing, or store/load ordering.

## In Scope

- Localize the first generated-code point where the stack-slot values used by
  `00164.c` are expected to be produced, stored, published, or reloaded.
- Determine whether the owner is value publication, local-slot initialization,
  frame-slot home assignment, spill/reload sequencing, or store/load ordering.
- Repair the semantic owner so initialized values reach their stack homes
  before later loads consume them.
- Add focused backend coverage for the repaired owner before relying only on
  the external c-testsuite representative.
- Validate `c_testsuite_aarch64_backend_src_00164_c` and classify any next
  first bad fact.

## Out Of Scope

- Reopening scalar machine-node operand fact printing or changing the repaired
  scalar printer behavior from idea 334 without fresh evidence that the old
  printer diagnostics returned.
- Reopening parked frame-layout idea 316 unless localization proves frame
  allocation or slot layout is the first owner for this runtime mismatch.
- Fixing only `00164.c`, one function, one instruction index, one stack offset
  such as `sp+#148`, `sp+#152`, or `sp+#156`, one emitted instruction string,
  or one observed output value.
- Weakening c-testsuite expectations, runner behavior, timeout policy,
  unsupported classification, or CTest registration.
- Broad byval, HFA/floating, stdarg cursor, MOVI, or large-offset work unless
  current generated-code evidence ties that owner to the first bad fact.

## Acceptance Criteria

- The first bad fact is localized to a concrete value, stack slot, generated
  load/store sequence, and backend owner with evidence from dumps or generated
  assembly.
- A general repair ensures stack-slot values in this route are initialized or
  published before their later AArch64 loads consume them.
- Focused backend coverage fails without the repair and proves the owner
  independently of `00164.c` where practical.
- `c_testsuite_aarch64_backend_src_00164_c` either passes or advances to a
  newly classified residual outside this owner.
- Adjacent AArch64 local-slot, spill/reload, frame-slot, and scalar arithmetic
  guardrails remain stable.

## Completion Note

Closed after commit `171690fce repair AArch64 load local publication`.
The repair retargets unpublished `bir.load_local` scalar operands to prepared
source frame-slot accesses when safe, fixes load-local prepared register
emission in dispatch, and adds focused AArch64 backend coverage for stack-home
and GP-home scalar consumers plus the machine-printer frame-slot operand guard.

Close proof:

- `c_testsuite_aarch64_backend_src_00164_c` passes.
- The closed scalar-printer representative
  `c_testsuite_aarch64_backend_src_00214_c` still passes.
- Backend validation passed with `ctest --test-dir build -j
  --output-on-failure -R '^backend_'`.
- The close regression guard passed using canonical `test_before.log` and
  `test_after.log`: before 4/1/5, after 5/0/5, resolved
  `c_testsuite_aarch64_backend_src_00164_c`, no new failures.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00164.c`, `main`, `extend_brk`, one stack offset such as
  `sp+#148`, `sp+#152`, or `sp+#156`, one instruction index, one printed value,
  or one emitted instruction sequence instead of repairing a general backend
  initialization or publication rule;
- claims progress by changing expected output, unsupported classifications,
  runner behavior, timeout policy, CTest registration, or proof-log handling;
- only renames helpers, rewrites diagnostics, changes dump formatting, or
  adjusts classification notes while the same uninitialized stack-slot load
  pattern remains possible;
- broadens into frame-layout, byval/HFA/stdarg, MOVI, or large-offset work
  without generated-code evidence that such an owner is the first bad fact for
  this runtime mismatch;
- reintroduces or hides the closed scalar printer diagnostics from idea 334
  instead of preserving the repaired operand-fact path.

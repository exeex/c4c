# LIR/BIR Vararg and va_arg Contract Completion

Status: Open
Type: Upstream contract idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/open/358_rv64_object_route_abi_width_edges.md`

## Goal

Complete the LIR/BIR/prepared contract for variadic functions, vararg entry
state, and `va_arg` consumption before any target object emitter attempts to
lower those shapes.

## Why This Exists

The prepared-shape classification found a small vararg bucket while examining
RV64 object-route failures. That bucket is not primarily RV64 object-emission
work. Target emission can only follow a concrete ABI lowering plan after the
frontend/LIR/BIR/prepared pipeline has represented:

- which functions are variadic,
- how fixed and variable arguments enter the function,
- how `va_start`, `va_arg`, `va_copy`, and `va_end` are modeled,
- which argument-save or overflow areas exist,
- which value homes are read for each `va_arg`, and
- what target ABI hook is expected to materialize the next argument.

Patching RV64 `object_emission.cpp` directly for vararg cases would put
language and ABI contract discovery at the wrong layer.

## In Scope

- Audit current frontend, LIR, BIR, and prepared representations for
  variadic function signatures and `va_*` operations.
- Define a target-independent contract for prepared vararg entry state and
  `va_arg` consumption.
- Define the boundary between shared vararg semantics and target ABI hooks.
- Add focused tests for the shared contract using representative vararg cases.
- Ensure unsupported vararg shapes report precise upstream diagnostics instead
  of falling through to generic RV64 prepared-object rejection.

## Out of Scope

- Implementing RV64-specific vararg object emission before the shared contract
  exists.
- Full RV64 ABI cleanup beyond the hooks needed to consume the shared vararg
  contract.
- Globals/data sections, FPR ABI expansion, vector ABI, or full instruction
  coverage.
- Optimizing for the opt-in full RV64 gcc torture scan failure count.
- Marking vararg gcc torture cases unsupported to make scans greener.

## Representative Cases

- `src/920908-1.c`: vararg family.
- `src/20030914-2.c`: byval/vararg-adjacent aggregate entry shape.

Additional representative tests should be chosen from existing frontend or
backend coverage if they isolate `va_start`, `va_arg`, `va_copy`, or
variadic-call entry behavior more precisely than the torture cases.

## Required Contract Questions

- Where is variadic function identity represented before BIR?
- Does BIR distinguish fixed argument homes from variable argument state?
- Is `va_list` represented as an opaque object, a target ABI object, or a
  structured prepared state?
- Which layer owns argument-save-area and overflow-area layout?
- What does a target backend receive for each `va_arg` operation?
- How are unsupported argument classes or aggregate varargs diagnosed?

## Acceptance Criteria

- The LIR/BIR/prepared vararg contract is documented and covered by focused
  tests.
- Target backends receive explicit vararg entry and `va_arg` plans or precise
  unsupported diagnostics.
- RV64 object emission no longer needs to inspect source/LIR/BIR ad hoc to
  infer vararg semantics.
- Representative vararg cases either progress to a target ABI hook requirement
  or fail with a precise upstream vararg-contract diagnostic.
- Existing baseline tests for touched frontend/LIR/BIR/prepared surfaces remain
  green.

## Reviewer Reject Signals

- Reject if RV64 object emission implements vararg semantics directly before
  the shared LIR/BIR/prepared contract exists.
- Reject testcase-name shortcuts for `920908-1.c`, `20030914-2.c`, or any
  other torture case.
- Reject expectation downgrades, unsupported broadening, or scan filtering
  claimed as progress.
- Reject a contract that hides `va_arg` behind an opaque blob with no target
  ABI hook boundary or diagnostic path.
- Reject target ABI code that guesses argument locations without explicit
  prepared vararg entry and value-home information.

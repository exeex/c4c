# LIR Memory/VA Pointer Authority Convergence Runbook

Status: Active
Source Idea: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Activated from: exhausted 734 Step 7.31 selected stack-restore receiver
runbook.

## Purpose

Publish checked native pointer/object/lifetime authority for the bounded
memory/va producer family so a later, separately repaired 734 packet can
receive one exact selected row without presentation recovery.

## Core Rule

Native structured current-function authority is the sole semantic input. Do
not use builtin names, operand spelling, rendered LIR or LLVM, testcase shape,
`monostate`, or unclassified operands as value, pointer, object, owner,
lifetime, or row-selection authority.

## Read First

- `ideas/open/753_lir_memory_va_pointer_authority_convergence.md`
- `ideas/closed/752_lir_local_object_pointer_authority_convergence.md`
- `docs/lir_to_new_bir_remaining_coverage/successor_queue.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (post-Step 7.31
  exhaustion record)

## Non-Goals

- Raw-BIR destination/importer/receiver work, target lowering, MIR, emission,
  alias analysis, or full memory-model semantics;
- CFG/PHI, local/object substrate definition, aggregate/vector carrier work,
  opaque inline-assembly text, or a second 734 receiver row;
- presentation-derived recovery or weakening verifier/test contracts.

## Ordered Steps

### Step 1 - Establish the bounded native memory/VA authority boundary

Goal: derive the executable producer route from closed 752's substrate and
identify the selected representative memory/va forms that can carry checked
pointer/object/lifetime authority.

Actions:

- inspect only memcpy, memset, va_start, va_end, va_copy, and va_arg producer
  and verifier seams named by the source idea;
- retain the already accepted selected memcpy row as history, not as a claim
  that the residual family is received or fully converted;
- record the native field and malformed-authority boundary needed before any
  producer publication; keep unsupported forms fail closed.

Completion check: an implementation packet can add only source-authorized
structured fields and rejection rules without deriving facts from text.

### Step 2 - Publish and verify native pointer/object/lifetime authority

Goal: make the selected representative memory/va producer forms structurally
authoritative and fail closed.

Actions:

- consume closed 752's current-function pointer/object/lifetime substrate;
- publish the minimum structured pointer/object/lifetime and typed size/value
  facts required by the selected forms;
- verify invalid, foreign, type-mismatched, size-mismatched, and dead
  authority before downstream use; add nearby positive and negative coverage.
- Treat an expanded full-baseline failure set after a Step 2 packet as a
  blocking in-scope regression: reproduce and repair it through the native
  authority boundary before another implementation commit. Do not hide it with
  named-test exceptions, expectation downgrades, text recovery, or a weaker
  verifier contract. The current rejected delta is
  `clang_c_external_C_C23_n2900_n3011_2_c`,
  `cpp_positive_sema_constrained_template_method_call_frontend_cpp`,
  `llvm_gcc_c_torture_src_strcpy_2_c`,
  `llvm_gcc_c_torture_src_zero_struct_1_c`, and
  `llvm_gcc_c_torture_src_zero_struct_2_c` after `8f6f4f9c9`.

Completion check: selected representative producer forms use checked native
authority, while unconverted rows remain fail closed or explicitly
compatibility-only; after a fresh build and the focused authority proof, the
full baseline has no new failures relative to `test_baseline.log` (3037/3037)
before any further implementation commit.

### Step 3 - Prove the bounded producer slice and hand off one receiver row

Goal: meet the source proof gate and publish exactly one later 734 receiver
handoff without Raw-BIR receipt work.

Actions:

- run a fresh build and focused memory/va producer/verifier proof;
- run the source-required full baseline; closure requires 100% passing tests,
  and a lower baseline must be diagnosed through `log/*` by time/commit before
  continuing. This does not replace Step 2's pre-commit no-new-failures gate;
  the accepted comparison point remains `test_baseline.log` (3037/3037);
- document exactly one selected 734 handoff with native fields, guarantees,
  rejected forms, and accepted proof.

Completion check: 753 has source-complete producer authority evidence and one
receiver-ready handoff; 734 remains unmodified by implementation and can later
resume only for that matching row.

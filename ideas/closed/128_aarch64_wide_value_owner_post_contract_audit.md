# 128 AArch64 wide value owner post-contract audit

## Goal

Audit `src/backend/mir/aarch64/codegen/i128_ops.cpp` and
`src/backend/mir/aarch64/codegen/f128.cpp` after the recent BIR/prealloc,
call-boundary, carrier, aggregate transport, and memory-contract cleanup.

This idea is analysis-only. It should classify the remaining wide-value
lowering responsibilities and produce focused follow-up ideas only where there
is concrete evidence that shared BIR/prealloc facts should own behavior that is
still being rediscovered or duplicated in AArch64 codegen.

## Background

The AArch64 codegen layout is now much cleaner around calls, memory, ALU, and
comparison boundaries, but `i128_ops.cpp` and `f128.cpp` remain two large
special-width owners:

- `i128_ops.cpp` owns i128 pair operations, shifts, compares, runtime helper
  boundaries, transport record construction, and helper policy validation.
- `f128.cpp` owns f128 transport records, memory-backed carrier handling,
  runtime helper diagnostics, vector/Q-register spelling, and related
  helper-policy validation.

Earlier closure notes covered adjacent contracts:

- special carrier policy cleanup
- aggregate transport planning
- f128 call carrier ownership
- pointer-carrier provenance
- call argument/result publication
- memory-backed publication authority
- ALU post-contract audit

What is still missing is a post-contract pass over the wide-value owner files
themselves. The important question is not whether these files are large. The
question is whether they still contain shared lowering policy that should have
been prepared before AArch64 codegen, or whether their size is mostly target
local instruction emission and machine-record construction.

## Required analysis

Classify each major cluster in `i128_ops.cpp` and `f128.cpp` into one of these
owners:

1. Shared BIR/prealloc contract
   - target-neutral carrier choice
   - helper ABI/resource policy
   - preservation/publication facts
   - memory-backed transport authority
   - call-boundary wide-value facts
2. AArch64 codegen consumption
   - validation that prepared facts are complete and unambiguous
   - translation of prepared facts into AArch64 machine instructions
   - AArch64 register spelling, Q/X pair spelling, lane/shift opcode spelling
   - runtime helper call assembly after ownership is already selected
3. Local organization only
   - helper functions that are file-local and target-specific
   - printer or diagnostic formatting that does not decide lowering policy
   - candidate physical splits that do not change semantic ownership

The audit must explicitly inspect:

- i128 runtime helper ownership and div/rem ABI policy
- i128 preserved value and selected-call ownership checks
- i128 pair transport, shift, and compare record construction
- f128 full-width and memory-backed carrier facts
- f128 runtime helper resource/preservation checks
- f128 transport construction and printable-address handling
- overlap with `calls.cpp`, `memory.cpp`, `instruction.cpp`, and
  `machine_printer.cpp`

## Expected output

The closure note must contain:

- a short cluster map for `i128_ops.cpp`
- a short cluster map for `f128.cpp`
- a table of any remaining shared-policy rediscovery in AArch64 codegen
- a table of target-local code that should stay in AArch64
- zero or more follow-up ideas with precise filenames, proof routes, and reject
  signals

If no shared-policy gap remains, close with no follow-up ideas and explain why
the remaining size is target-local emission or local organization.

## Reject signals

- Rewriting or splitting the two files only because they are large.
- Moving AArch64 register spelling, Q-register spelling, lane/shift opcode
  spelling, or helper call assembly into shared BIR/prealloc code.
- Reopening closed carrier, transport, calls, or memory contracts without new
  evidence from these two files.
- Creating vague follow-up ideas such as "clean f128" or "shrink i128" without
  a concrete ownership boundary and proof route.
- Treating line count alone as evidence of architectural drift.
- Weakening tests or expectations to make the audit easier.

## Suggested proof route for follow-up ideas

This idea itself is analysis-only. Follow-up implementation ideas should choose
their own narrow proof, but likely candidates include:

- `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`
- targeted AArch64 backend internal tests covering i128/f128 transport and
  helper lowering
- a broader `^backend_` run when a shared BIR/prealloc contract changes

## Close Note

Closed on 2026-06-08. The analysis-only audit completed all six runbook steps
and found no remaining shared-policy rediscovery in AArch64 wide-value codegen.

### `i128_ops.cpp` Closure Cluster Map

| Cluster | Classification | Closure evidence |
| --- | --- | --- |
| Runtime helper resource, div/rem ABI, preservation, and selected-call facts | `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption` | Prealloc creates helper resource, clobber, div/rem ABI, live-preservation, and selected-call ownership facts; `i128_ops.cpp` rejects missing or inconsistent prepared facts and copies them into the machine helper record. |
| Runtime helper assembly | `aarch64-codegen-consumption` | Helper printing validates prepared provenance, emits lane marshal/unmarshal `mov` sequences, and emits `bl <callee>` from the prepared callee identity. |
| Pair carrier and copy transport | `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption` | Transport record construction requires complete prepared i128 carriers, lane roles, size/alignment, slot/register placement, and prepared source facts before building AArch64 machine payloads. |
| Pair arithmetic/logical records | `aarch64-codegen-consumption` | The file maps supported BIR i128 add/sub/and/or/xor operations into target pair record shape, defs/uses, and AArch64 printer opcodes. |
| Shift records | `aarch64-codegen-consumption` with prepared scalar-storage consumption | Shift construction consumes prepared pair operands and prepared shift-count value/storage facts, then owns target shift-kind, lane semantics, machine effects, and printer spelling. |
| Compare records | `aarch64-codegen-consumption` with prepared scalar-result consumption | Compare lowering consumes prepared i128 carriers and prepared scalar result placement, while AArch64 owns condition spelling, labels, `cmp`/`ccmp`/`cset`, and result materialization. |
| Diagnostics, enum names, register display, target print wrappers, instruction payload variants | `local-organization-only` | These helpers organize target MIR records and diagnostics; they do not select shared carrier, ABI, preservation, or publication policy. |

### `f128.cpp` Closure Cluster Map

| Cluster | Classification | Closure evidence |
| --- | --- | --- |
| Full-width carrier facts | `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption` | `f128.cpp` requires complete prepared F128 full-width carriers with 16-byte size/alignment, Vreg/vector class, contiguous width, and prepared register names before converting to Q-register operands. |
| Memory-backed carrier facts and memory operands | `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption` | Prepared carrier, prepared addressing, slot id, stack offset, size/alignment, and value-home facts are required and cross-checked; AArch64 does not invent stack homes or offsets. |
| Call-boundary f128 carrier overlap | `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption` | `calls.cpp` uses the same prepared full-width carrier gates and prepared call/memory/value-home facts for argument/result moves and stack stores. |
| Helper resource, ABI, clobber, preservation, selected-call, scalar bridge, and marshaling policy | `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption` | Prealloc creates resource, ABI, clobber, live-preservation, selected-call ownership, carrier/ABI bindings, scalar bridge, comparison-result consumption, and marshaling moves; `f128.cpp` validates and copies them. |
| Helper-boundary record construction | `aarch64-codegen-consumption` | Record construction rejects missing prepared helper policy and then builds target machine payloads with prepared callee, values, clobbers, bindings, marshaling, source/result types, and provenance. |
| Transport records and printable addresses | `aarch64-codegen-consumption` plus `local-organization-only` address spelling | Records carry prepared carrier and memory facts; printable-address helpers materialize frame-slot/direct/symbol addresses with AArch64 scratch registers for final assembly. |
| Q/vector/scalar register spelling and helper invocation printing | `local-organization-only` and `aarch64-codegen-consumption` | Q, `vN.16b`, W/S/D, `mov`, `fmov`, `cmp`, `cset`, `ldr`, `str`, and `bl` spelling are target assembly details over prepared facts. |
| Machine effects, payload dispatch, diagnostics, and validation wrappers | `aarch64-codegen-consumption` or `local-organization-only` | Instruction builders create operands, defs/uses, clobbers, side effects, selection status, and payloads; the machine printer dispatches payload variants back to target print helpers. |

### Remaining Shared-Policy Rediscovery

| Candidate | Exact files | Finding | Closure decision |
| --- | --- | --- | --- |
| i128 helper ABI/resource/preservation/selected-call validation | `src/backend/mir/aarch64/codegen/i128_ops.cpp`, `src/backend/prealloc/i128_runtime_helpers.cpp`, `src/backend/mir/aarch64/codegen/calls.cpp` | AArch64 validates and copies prepared policy; prealloc owns policy construction. The unused local `has_complete_i128_div_rem_abi_policy` helper is cleanup-only evidence, not behavior-moving policy. | No shared-policy gap. |
| i128 carrier transport, shifts, and compares | `src/backend/mir/aarch64/codegen/i128_ops.cpp`, `src/backend/mir/aarch64/codegen/instruction.cpp`, `src/backend/mir/aarch64/codegen/machine_printer.cpp`, `src/backend/mir/aarch64/codegen/comparison.cpp` | Carrier/result/count facts are prepared before lowering; AArch64 owns record shape and final opcode/register/label spelling. | No shared-policy gap. |
| f128 full-width and memory-backed carriers | `src/backend/mir/aarch64/codegen/f128.cpp`, `src/backend/mir/aarch64/codegen/memory.cpp`, `src/backend/mir/aarch64/codegen/calls.cpp` | Prepared carrier, addressing, value-home, and call-boundary facts are required and cross-checked before target records are accepted. | No shared-policy gap. |
| f128 helper resource, ABI, preservation, scalar bridge, transport, and printing | `src/backend/mir/aarch64/codegen/f128.cpp`, `src/backend/prealloc/f128_runtime_helpers.cpp`, `src/backend/mir/aarch64/codegen/instruction.cpp`, `src/backend/mir/aarch64/codegen/machine_printer.cpp` | Helper policy is prepared in prealloc; AArch64 rejects missing or inconsistent prepared facts and emits target machine records plus assembly. | No shared-policy gap. |

No shared-policy gap remains from this audit. The remaining size in
`i128_ops.cpp` and `f128.cpp` is explained by target-local emission, target MIR
record construction, defensive provenance/completeness validation, and local
organization. The evidence does not show AArch64 selecting target-neutral
carrier ownership, helper ABI/resource policy, preservation policy, selected
call ownership, stack homes, stack offsets, memory-backed publication authority,
or scalar bridge ownership without prepared input.

### Target-Local Code That Should Stay In AArch64

| Area | Files | Reason to keep target-local |
| --- | --- | --- |
| i128 helper invocation assembly | `src/backend/mir/aarch64/codegen/i128_ops.cpp` | Prepared helper facts are spelled as AArch64 lane moves and `bl` calls. |
| i128 pair arithmetic/logical printing | `src/backend/mir/aarch64/codegen/i128_ops.cpp` | `adds`/`adc`, `subs`/`sbc`, `and`, `orr`, and `eor` are target opcode sequences. |
| i128 shift and compare printing | `src/backend/mir/aarch64/codegen/i128_ops.cpp`, `src/backend/mir/aarch64/codegen/comparison.cpp` | Shift lane mechanics, condition spelling, local labels, `cmp`/`ccmp`/`cset`, and result materialization are target assembly details. |
| f128 Q/vector/scalar register spelling | `src/backend/mir/aarch64/codegen/f128.cpp` | Q, vector lane, W/S/D, and ABI-view conversions are AArch64 register spelling. |
| f128 printable-address materialization and transport printing | `src/backend/mir/aarch64/codegen/f128.cpp` | Frame-slot/direct/symbol address spelling and scratch-register use are target assembly details over prepared memory facts. |
| f128 helper invocation assembly | `src/backend/mir/aarch64/codegen/f128.cpp` | `mov`, `fmov`, `cmp`, `cset`, and `bl` emission consumes prepared helper policy and belongs to final target printing. |
| Target MIR records, payload variants, side effects, status diagnostics, and printer dispatch | `src/backend/mir/aarch64/codegen/instruction.cpp`, `src/backend/mir/aarch64/codegen/instruction.hpp`, `src/backend/mir/aarch64/codegen/machine_printer.cpp`, `src/backend/mir/aarch64/codegen/i128_ops.cpp`, `src/backend/mir/aarch64/codegen/f128.cpp` | These are AArch64 machine representation and diagnostic organization, not shared lowering policy. |

### Follow-Up Idea Payloads

No shared-policy follow-up idea is warranted by current evidence.

| Proposed filename | Owner boundary | Exact files | Proof route | Reject signals |
| --- | --- | --- | --- | --- |
| `ideas/open/129_aarch64_i128_shift_support_completeness.md` | AArch64 i128 shift lowering/printing completeness; not shared BIR/prealloc policy. | `src/backend/mir/aarch64/codegen/i128_ops.cpp`; likely tests under the backend i128 shift coverage area if enabled by supervisor/plan-owner. | Add or enable backend coverage for i128 shift-by-64-or-more and variable-count shifts, then prove selected machine output or an explicit unsupported diagnostic with the narrow backend subset chosen by the supervisor. | Reject if the idea tries to move AArch64 shift opcode spelling, lane mechanics, or register-pair spelling into shared BIR/prealloc code; reject if it weakens expectations or only masks a named testcase. |

No f128 follow-up idea is warranted by current evidence. A future f128 payload
would need to show AArch64 inventing helper ABI/resource/preservation policy,
selected-call ownership, carrier placement, scalar comparison consumption,
stack-home/offset authority, or memory-backed publication without prepared
input; this audit found the opposite.

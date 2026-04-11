# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 2 shared-header/runtime packet:
  continue expanding the already-built translated `calls.cpp` owner across the
  bounded direct-call helper families, while keeping `prologue.cpp` parked
  until the exporter-backed state surface is intentionally widened

## Immediate Target

- route one bounded direct-call/runtime slice through translated
  `X86Codegen::{emit_call_*}` ownership instead of the legacy direct helper path
- keep header surfacing limited to what that runtime cutover needs
- prove the cutover with one existing-or-narrowly-extended direct-call backend
  regression
- generate the next bounded implementation packet from this state instead of
  widening ad hoc

## Completed Context Carried Forward

- translated peephole optimization is already wired into the active x86 path
- `globals.cpp` is already compiled and carries bounded translated-owner
  behavior
- `returns.cpp` already has symbol/link coverage in the build, but its real
  translated bodies still need more public `X86Codegen` surface
- several direct-call spacing/suffix regressions were added in prior
  iterations; treat them as existing guardrails, not as the current work queue
- Step 1 audit outcome:
  `calls.cpp` is already compiled and only depends on the shared surface now
  present in `x86_codegen.hpp` (`CallAbiConfig`, `CallArgClass`,
  `X86CodegenState`, return-class state, and the accumulator/store helpers),
  so it is the smallest practical next runtime owner
- `returns.cpp` is narrower than `prologue.cpp`, but it is already parked as
  symbol/link coverage and does not reduce active `emit.cpp` ownership by
  itself
- `prologue.cpp` remains a later surface-expansion slice because it still
  depends on hidden exporter-backed state and members such as variadic/register
  allocation bookkeeping, `state.cf_protection_branch`,
  `state.function_return_thunk`, `state.param_classes`,
  `state.param_pre_stored`, `state.fresh_label(...)`, and `state.emit_fmt(...)`
- proof baseline is currently green for:
  `./build/backend_shared_util_tests test_x86_direct_dispatch_owner_handles_helper_backed_prepared_lir_slice`
  and
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice`
- accepted packet result:
  the bounded call-crossing helper slice now routes through translated
  `X86Codegen::emit_call_instruction_impl(...)` while preserving the expected
  Intel-syntax text shape
- accepted packet result:
  the bounded helper-backed local-arg slice now routes through translated
  `X86Codegen::emit_call_instruction_impl(...)` while preserving the expected
  Intel-syntax text shape
- accepted packet result:
  the bounded two-arg helper family now routes through translated
  `X86Codegen::emit_call_instruction_impl(...)` while preserving the expected
  Intel-syntax text shape
- accepted packet result:
  the bounded folded two-arg direct-call family now routes through translated
  `X86Codegen::emit_call_instruction_impl(...)` while preserving the expected
  Intel-syntax text shape
- post-review validation is green for:
  `./build/backend_shared_util_tests test_x86_direct_dispatch_owner_handles_helper_backed_prepared_lir_slice`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice_with_spacing`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice_with_suffix_spacing`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice_with_spacing`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice_with_suffix_spacing`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice_with_spacing`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice_with_suffix_spacing`
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_folded_two_arg_slice`

## Guardrails For This Activation

- do not add more direct-call testcase families unless the active owner slice
  cannot be proven without one specific new test
- do not widen into branch/select/shared-BIR cleanup during this activation
- if a gap is useful but not required for the owner transfer, record it back in
  the source idea instead of extending this todo
- do not choose `prologue.cpp` as the next packet just because it is adjacent;
  its state-surface gap is materially larger than the active `calls.cpp`
  runtime cutover

## Next Slice After Current Item

- Step 2/3 packet:
  either extract the repeated translated call-line helper inside
  `direct_calls.cpp` and keep extending bounded helper families, or stop the
  call-family expansion here and pivot to the next owner cluster
- if continuing calls, prefer the next smallest family that still exercises a
  distinct call-site shape
- otherwise reassess whether `returns.cpp` is now smaller than more call-family
  expansion

## Open Questions

- how much shared helper scaffolding should be extracted before repeated
  translated call-site wiring stops being copy-paste
- whether `returns.cpp` is now the next smallest real owner transfer after this
  bounded call-family batch

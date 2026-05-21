# Semantic BIR Pointer-Derived String Loads

Status: Open
Created: 2026-05-21
Split From: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md

## Goal

Preserve dynamic pointer-derived byte loads in semantic BIR instead of
collapsing them to fixed global string-byte loads.

## Why This Exists

During idea 355 Step 4 classification, `00173` remained a timeout after the
AArch64 address/pointer/pointee publication repairs. Read-only BIR evidence
showed the first bad fact before AArch64 lowering: loop expressions such as
`*b` and `*src` are already represented as fixed
`bir.load_global i8 @.str0` / `@.str0+1` accesses. The generated loop then
tests the first string byte repeatedly instead of reading through the current
incremented pointer value.

This is a semantic-BIR value modeling issue, not an AArch64 publication issue.

## In Scope

- Localize where semantic lowering loses the dynamic pointer base for
  pointer-derived string or array byte loads.
- Repair BIR construction so loads through an incremented pointer use that
  pointer value as the memory base.
- Add focused semantic or backend-route coverage that proves dynamic pointer
  loads remain dynamic before AArch64 codegen.
- Prove `c_testsuite_aarch64_backend_src_00173_c` advances or passes if the
  semantic repair reaches generated code.

## Out Of Scope

- AArch64 address-valued memory or call-argument publication already covered by
  idea 355.
- Timeout policy, runner behavior, CTest registration, proof-log handling, or
  expectation changes.
- Filename-specific handling for `00173`, one source string literal, one loop
  shape, or one generated instruction sequence.
- Same-module recursive pointer-formal/callee-saved-home publication for
  `00181`.
- Frontend admission work for `00005`.

## Acceptance Criteria

- The first bad fact is localized to a semantic-BIR lowering boundary that
  turns a pointer-derived load into a fixed global-byte load.
- Focused coverage fails before the repair and passes after it, proving the
  dynamic pointer base is preserved.
- `00173` advances or passes without expectation, timeout, runner, or
  filename-specific changes.
- Nearby string or pointer byte-load cases are checked enough to show the fix
  is semantic rather than testcase-shaped.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00173`, one string literal, one global symbol, one loop body,
  or one emitted AArch64 instruction pattern;
- claims progress by changing timeout policy, runner behavior, unsupported
  classifications, proof logs, or external test expectations;
- moves the failure later by renaming helpers or reshaping dumps while semantic
  BIR still contains fixed `@.str0` byte loads for pointer-derived accesses;
- broadens into AArch64 call publication, recursive formal homes, ABI work, or
  unrelated frontend admission without a lifecycle split;
- leaves dynamic `*p`/`*src`-style byte loads unable to use the current pointer
  value as the memory base.

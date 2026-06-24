# Current Packet

Status: Active
Source Idea Path: ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Local-Call And RegReg Object Rejections

## Just Finished

- Split idea 334 because its first resumed scan inspection found known-red
  target object-emitter blockers rather than harness-only scan additions.
- Activated focused child 337 for RV64 local-backed scalar call arguments and
  AArch64 register-register scalar/call-result object support.

## Suggested Next

- Inspect the rejected RV64 prepared records and AArch64 selected machine
  records, then record the exact Step 2 implementation seam and proof command
  in `todo.md`.

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- Keep RV64 globals/data, arrays, pointers, aggregates, broad memory/control
  flow, broad AArch64 frame/local-memory expansion, AArch64 runtime, x86 object
  output, object stdout, c-testsuite object defaults, and semantic-BIR object
  mode out of this child.
- Parent idea 334 should resume only after this child restores the next object
  scan candidates as green native object-output tests.

## Proof

- Lifecycle-only split/switch; no build required.

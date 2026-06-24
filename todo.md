# Current Packet

Status: Active
Source Idea Path: ideas/open/333_codegen_obj_cli_and_test_integration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect CLI And Backend Output-Form Seams

## Just Finished

- Activated `plan.md` from `ideas/open/333_codegen_obj_cli_and_test_integration.md`.

## Suggested Next

- Execute Step 1 of `plan.md`: inspect the CLI and backend output-form seams,
  then record the chosen object-route integration seam and first proof subset
  here.

## Watchouts

- Do not implement `--codegen obj` by printing `.s` and invoking or parsing an
  assembler.
- Do not remove, rename away, or weaken existing `--codegen asm` tests.
- Do not make c-testsuite default to object output in this child.
- Split unrelated backend semantic gaps into focused follow-up ideas instead
  of absorbing them into CLI wiring.

## Proof

- Lifecycle-only activation; no build proof required yet.

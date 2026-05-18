Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate the Shared Address Authority Break

# Current Packet

## Just Finished

Lifecycle switched from the umbrella inventory to focused source idea
`ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md`.
No implementation packet has run for this active plan yet.

## Suggested Next

Execute Step 1: inspect `src/00217.c`, `src/00032.c`, `src/00130.c`, and
`src/00180.c` plus their generated AArch64 assembly to locate the shared
address/lvalue authority break before editing implementation files.

## Watchouts

- Keep the focused idea about semantic address derivation and lvalue use, not
  testcase-shaped rewrites for `00217`.
- Include generated-assembly inspection in proof because several current
  failures publish uninitialized registers or wrong stack offsets as pointers.
- Preserve the post-293 separation for frontend, timeout,
  floating/conversion, string/library-only, aggregate initializer, and
  closed-owner overlap buckets.

## Proof

Lifecycle-only switch. No tests were run and no `test_after.log` was produced.

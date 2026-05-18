# AArch64 String/Global Address External Call Lowering Todo

Status: Active
Source Idea Path: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the Minimal Address Argument Failure

# Current Packet

## Just Finished

Lifecycle switched from the umbrella AArch64 c-testsuite inventory route to
the focused string/global address external-call lowering route. No
implementation work has started.

## Suggested Next

Execute Step 1 from `plan.md`: reproduce and localize the minimal
`src/00125.c` failure with the supervisor-selected narrow command, then record
the backend lowering/emission surface before editing implementation files.

## Watchouts

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, CTest registration, parser, or sema.
- Reject named-case shortcuts, literal-spelling shortcuts, and `printf`-only
  special cases.
- Keep `src/00132.c` out of first proof; it is timeout-sensitive and
  loop/local-store-compounded.
- Keep broad aggregate ABI/HFA returns, broad variadic ABI completeness,
  `_Generic`, wide chars, and function-pointer casts out of this route.

## Proof

Lifecycle-only switch. No build, test, scan, or implementation proof was run.

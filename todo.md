Status: Active
Source Idea Path: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Value-Kind Boundary

# Current Packet

## Just Finished

Lifecycle switch from umbrella inventory idea 295 to focused address-valued
memory/call-argument publication idea 355. No implementation work has started
under this owner.

## Suggested Next

Delegate Step 1 to localize the first concrete address/pointer/pointee
publication boundary, starting with `00020` plus one of `00170` or `00189`.

## Watchouts

- Do not edit expectations, unsupported classifications, runners, timeout
  policy, CTest registration, or proof-log behavior.
- Do not special-case c-testsuite filenames, source function names, stack
  offsets, symbol names, or emitted instruction neighborhoods.
- Keep scalar compare/select, floating variadic scalar, composite ABI,
  dynamic-stack/goto, and aggregate initializer residuals out of this owner
  unless fresh first-bad-fact evidence justifies a lifecycle split.

## Proof

Lifecycle-only switch. `git diff --check` is the required validation for this
packet.

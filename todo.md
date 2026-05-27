Status: Active
Source Idea Path: ideas/open/57_aarch64_hfa_aggregate_return_result_consumption_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Trace The HFA Return-Result Consumer

# Current Packet

## Just Finished

Lifecycle switched away from idea 56 because its Step 1 classification found a
non-owned first bad fact: `fr_hfa12()` returns through `s0`/`s1`, but the caller
after `bl fr_hfa12` stores or converts stale `s9`/`s13`, producing `2.0 12.1`
instead of `12.1 12.2`.

## Suggested Next

Start Step 1 of `plan.md`: reproduce or inspect the focused proof for the
`fr_hfa12()` mismatch, trace the post-call caller-side consumer, and identify
which prepared HFA call-result lane fact should select `s0`/`s1`.

## Watchouts

- Treat `00204`, `ret`, `fr_hfa12`, `s9`, and `s13` as diagnostics only.
- Do not route implementation through idea 56 edge/terminator preservation
  while the first bad fact is HFA aggregate return-result consumption.
- Do not change variadic va_list layout, by-value argument publication, or
  callee-side return publication unless tracing proves they are the active
  first bad fact.
- Do not weaken expectations or mark supported probes unsupported.

## Proof

Lifecycle-only switch; no build or test command was run by the plan owner.

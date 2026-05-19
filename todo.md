Status: Active
Source Idea Path: ideas/open/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and localize the focused printer failures

# Current Packet

## Just Finished

Lifecycle switch completed from umbrella idea 295 to focused idea 299. No
implementation packet has run for this active plan yet.

## Suggested Next

Start Step 1 by reproducing the focused scalar immediate machine-printer
failures for `00031`, `00104`, `00143`, `00207`, `00213`, `00214`, `00215`,
and `00218`, then record the current diagnostic and operand shapes here.

## Watchouts

- Progress must repair scalar add/sub/bitwise immediate materialization or
  encoding fallback semantics, not match filenames or emitted instruction
  strings.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not fold scalar cast, mul/div/rem, call-boundary, memory store/symbol,
  runtime, or timeout residuals into this owner without new generated-code or
  diagnostic evidence.
- Do not reopen closed owners 285 through 298 from residual counts alone.

## Proof

No implementation proof has run for this focused owner yet. The split is based
on the committed post-298 inventory recorded in the previous `todo.md`.

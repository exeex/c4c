Status: Active
Source Idea Path: ideas/open/377_aarch64_external_libc_call_result_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Call-Result Publication

# Current Packet

## Just Finished

Lifecycle switched from umbrella inventory idea 295 to focused idea 377 for
`00187` external/libc call-result publication. No implementation work has
started under this focused owner.

## Suggested Next

Start Step 1: inspect prepared/selected AArch64 state and generated
`00187.c.s` around the `fread` call, then identify where the return count in
`x0` stops reaching the scalar comparison against `6`.

## Watchouts

Do not implement under idea 295. Do not special-case `00187`, `fread`,
`[sp, #96]`, or one emitted instruction neighborhood. Keep recursive argument
preservation, direct argument/formal publication, function return epilogue
clobbering, scalar FP materialization, aggregate relocation, timeout work,
expectations, runners, CTest registration, and proof-log policy out of scope
unless fresh first-bad-fact evidence proves otherwise.

## Proof

Lifecycle-only handoff. No build or test command was run.

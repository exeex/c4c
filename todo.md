Status: Active
Source Idea Path: ideas/open/361_rv64_vararg_abi_hook_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit RV64 Vararg Target Requirements

# Current Packet

## Just Finished

Lifecycle activation only. No implementation packet has run for Step 1 yet.

## Suggested Next

Start Step 1 by inspecting the current RV64 object-admission logs for
`src/20030914-2.c` and `src/920908-1.c`, then map each missing target ABI fact
to its RV64 hook/admission owner and choose the first narrow proof command.

## Watchouts

Do not repair the representative cases through testcase-name matching,
expectation downgrades, or shared-contract rewrites. This follow-up starts from
the prepared vararg facts already published by idea 360.

## Proof

Lifecycle-only activation; no implementation validation required for this
packet.

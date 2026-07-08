Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Representative Store-Family Proof Rows

# Current Packet

## Just Finished

Activated `ideas/open/603_bir_local_memory_store_semantics.md` into `plan.md` and initialized this execution state for Step 1.

## Suggested Next

Execute Step 1: select representative local-memory store proof rows and nearby guard rows, then record the exact supervisor-delegated proof command here.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Do not touch implementation files during lifecycle activation.
- Do not change tests, expectations, unsupported markers, allowlists, runtime, timeout, or accounting behavior.
- Treat selected rows as probes only; reject named-case or testcase-shaped fixes.
- Preserve the completed load-semantics boundary from idea 602.

## Proof

Lifecycle activation only; no build or compiler test proof required.

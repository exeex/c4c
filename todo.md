# Current Packet

Status: Active
Source Idea Path: ideas/open/704_bir_semantic_handoff_views.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Audit the boundary and run integration proof

## Just Finished

- Completed Step 6's boundary audit: all seven named public headers and result
  payloads are free of `RouteN`, `RouteIndex`, `route_index`, and complete
  route records, and expose source-semantic facts without prepared placement
  or target authority.
- Classified remaining route vocabulary as legacy/private BIR compatibility
  (`bir.hpp`, `bir_route*.cpp`, route facade/index headers), deferred common-MIR
  and prepared consumers owned by idea 705, or debug/proof coverage; no Step 6
  violation was found.
- Compared compatibility-adapter callers from plan activation (`339e38a8a`) to
  `HEAD` using symbol/caller queries and call-site searches: the legacy Route 6
  call-boundary and Route 4 publication adapters gained no new downstream
  callers.
- Identified focused positive and unavailable/incomplete/ambiguous proof for
  all seven contracts in the four `backend_bir_*_view_contract` tests, plus
  common-MIR producer query proof in `backend_x86_shared_producer_query`.

## Suggested Next

- Ask the plan owner to evaluate lifecycle closure; the runbook and source-idea
  acceptance criteria appear complete.

## Watchouts

- Route-backed helpers remain in `query.cpp` for deferred memory, publication,
  select-chain, and prepared-consumer migration; keep that work in idea 705.
- `bir.hpp` remains the legacy aggregate compatibility surface and therefore
  still contains route declarations; the seven ownership-named headers are the
  guarded public semantic boundary.

## Proof

- Passed the supervisor-selected integration proof (308/308 backend tests):
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`.
- The suite includes `backend_bir_producer_view_contract`,
  `backend_bir_memory_publication_view_contract`,
  `backend_bir_call_comparison_view_contract`,
  `backend_bir_return_control_flow_view_contract`, and
  `backend_x86_shared_producer_query`.
- Canonical proof log: `test_after.log`.

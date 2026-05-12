Status: Active
Source Idea Path: ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Route Downstream Memory Or Prepared Handoff Through Structured Facts

# Current Packet

## Just Finished

Completed Step 3 by checking downstream consumers of the selected dynamic
global scalar-array route.

Downstream check:
- The selected route reaches BIR at
  `load_dynamic_global_scalar_array_value(...)`, which emits each materialized
  scalar load as `bir::LoadGlobalInst`.
- `LoadGlobalInst` carries `global_name_id` separately from final
  `global_name` spelling, so the structured identity survives the BIR boundary.
- `bir_validate.cpp` validates a present `global_name_id`, finds the declared
  global by `LinkNameId` before spelling, and rejects visible-name/id conflicts
  when both are declared.
- `bir_printer.cpp` keeps final display spelling only; it does not feed
  semantic global identity back into lowering for this route.
- The remaining raw/no-id fallback is visibly separated: only
  `kInvalidLinkName` accesses use the compatibility lookup by final global
  spelling.

No implementation edit was needed for this packet.

## Suggested Next

Execute Step 4 by recording or adding focused proof for the selected route:
matching structured success despite drifted display spelling, stale or missing
`LinkNameId` failure, and raw/no-id compatibility.

## Watchouts

- Do not re-prove only the addressed-global pointer route already closed by
  idea 187.
- Do not treat local route names, local slots, SSA temporaries, or block labels
  as semantic global symbols.
- The fail-closed check still lives at materialization time; the BIR boundary
  preserves the selected route's carried id rather than re-deriving identity
  from spelling.
- Public LIR fixtures already cover matching structured success, missing
  link-name spelling rejection, and raw/no-id compatibility. A mismatched
  carried-id fixture may need a consumer-level harness because normal producers
  derive the id from the same `GlobalInfo` entry as final spelling.
- Raw/no-id fallback remains available only when the selected access carries
  `kInvalidLinkName`; that fallback is compatibility lookup, not semantic
  global identity.

## Proof

Downstream-check packet only; no implementation files were changed, so the
delegated proof did not require a new command or a new `test_after.log`.

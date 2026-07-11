#include "src/backend/mir/prepared_view.hpp"
#include "src/backend/prealloc/module.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;
namespace prepared = c4c::backend::mir::prepared;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named(bir::TypeKind type, const char* name) {
  return bir::Value::named(type, name);
}

prepare::PreparedBirModule make_fixture() {
  prepare::PreparedBirModule module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);
  module.module.target_triple = "x86_64-test-c4c";

  const auto function_name = module.names.function_names.intern("main");
  const auto declaration_name = module.names.function_names.intern("extern_fn");
  const auto missing_fact_name = module.names.function_names.intern("missing_facts");
  const auto entry_label = module.names.block_labels.intern("entry");
  const auto value_name = module.names.value_names.intern("%v");
  const auto global_name = module.module.names.link_names.intern("global_i32");
  const auto block_label_id = module.module.names.block_labels.intern("entry");

  module.module.globals.push_back(bir::Global{
      .name = "global_i32",
      .link_name_id = global_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "abc",
  });

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = named(bir::TypeKind::I32, "%v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::ReturnTerminator{.value = named(bir::TypeKind::I32, "%v")};

  bir::Function main;
  main.name = "main";
  main.return_type = bir::TypeKind::I32;
  main.blocks.push_back(std::move(entry));
  module.module.functions.push_back(std::move(main));

  bir::Function declaration;
  declaration.name = "extern_fn";
  declaration.return_type = bir::TypeKind::I32;
  declaration.is_declaration = true;
  module.module.functions.push_back(std::move(declaration));

  bir::Function missing_facts;
  missing_facts.name = "missing_facts";
  missing_facts.return_type = bir::TypeKind::I32;
  missing_facts.blocks.push_back(bir::Block{.label = "entry"});
  module.module.functions.push_back(std::move(missing_facts));

  module.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  module.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{1},
          .function_name = function_name,
          .value_name = value_name,
          .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
          .immediate_i32 = 3,
      }},
  });
  module.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
  });

  module.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = missing_fact_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = entry_label}},
  });

  module.completed_phases.push_back("phase-sentinel-not-core");
  module.notes.push_back(prepare::PrepareNote{
      .phase = "diagnostic-phase-sentinel",
      .message = "prepare-note-sentinel-not-core",
  });
  if (declaration_name == c4c::kInvalidFunctionName) {
    module.notes.push_back(prepare::PrepareNote{
        .phase = "test",
        .message = "unreachable marker to keep declaration id live",
    });
  }
  return module;
}

int verify_core_view() {
  const auto module = make_fixture();
  const prepared::PreparedMirCoreView view(module);

  if (view.target_profile().arch != c4c::TargetArch::X86_64 ||
      view.target_triple() != "x86_64-test-c4c") {
    return fail("expected target identity to come from prepared module facts");
  }
  if (view.functions().size() != 3 || view.defined_functions().size() != 2) {
    return fail("expected all functions and declaration-filtered traversal");
  }
  if (view.globals().size() != 1 || view.string_constants().size() != 1) {
    return fail("expected module data accessors to expose read-only BIR data");
  }

  const auto main_id = view.resolve_function_name("main");
  if (!main_id.has_value() || view.function_name(*main_id) != "main") {
    return fail("expected prepared function-name resolution");
  }
  if (view.resolve_function_name("absent").has_value() ||
      view.function_view("absent").has_value()) {
    return fail("expected missing names to fail closed");
  }
  if (view.function_view("extern_fn").has_value()) {
    return fail("expected declarations to be excluded from function views");
  }
  if (view.function_view("missing_facts").has_value()) {
    return fail("expected function views to require core prepared facts");
  }

  const auto function_view = view.function_view(*main_id);
  if (!function_view.has_value() || !*function_view) {
    return fail("expected complete defined function to produce a function view");
  }
  if (&function_view->bir_function() != view.bir_function(*main_id) ||
      &function_view->control_flow() != view.control_flow(*main_id)) {
    return fail("expected function view to reference prepared module facts");
  }
  if (function_view->function_name_text() != "main" ||
      function_view->value_locations().value_homes.size() != 1 ||
      function_view->addressing().function_name != *main_id) {
    return fail("expected per-function core facts through the view");
  }
  if (function_view->blocks().size() != 1 ||
      function_view->blocks().front().block_label == c4c::kInvalidBlockLabel ||
      function_view->blocks().front().block == nullptr ||
      function_view->blocks().front().control_flow == nullptr) {
    return fail("expected block view to bind BIR and prepared control-flow facts");
  }

  const auto cursor = function_view->instruction(0, 0);
  if (!cursor.has_value() ||
      cursor->function_name != *main_id ||
      cursor->block == nullptr ||
      cursor->instruction == nullptr ||
      cursor->prepared_block == nullptr) {
    return fail("expected instruction cursor to bind BIR and prepared coordinates");
  }
  if (function_view->instruction(0, 1).has_value() ||
      function_view->instruction(1, 0).has_value()) {
    return fail("expected out-of-range instruction lookups to fail closed");
  }

  const auto value_name = module.names.value_names.find("%v");
  const auto* value_home =
      prepare::find_indexed_prepared_value_home(&function_view->prepared_lookups().value_homes,
                                                nullptr,
                                                &function_view->value_locations(),
                                                value_name);
  if (value_home == nullptr || value_home->immediate_i32 != 3) {
    return fail("expected view-owned prepared lookup cache to agree with value homes");
  }

  return 0;
}

bool contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

bool has_blocking_difference(const prepared::PreparedMirCoreComparisonReport& report,
                             std::string_view expected) {
  for (const auto& difference : report.blocking_differences) {
    if (difference == expected) {
      return true;
    }
  }
  return false;
}

int verify_core_view_canonical_dump() {
  const auto module = make_fixture();
  const prepared::PreparedMirCoreView view(module);
  const std::string dump = view.canonical_dump();
  const std::string repeat_dump = view.canonical_dump();

  if (dump != repeat_dump) {
    return fail("expected canonical core view dump to be deterministic");
  }
  if (!contains(dump, "prepared_mir_core_view schema=1\n") ||
      !contains(dump, "target triple=x86_64-test-c4c") ||
      !contains(dump, "arch=x86_64") ||
      !contains(dump, "module functions=3 defined_functions=2 globals=1 strings=1\n")) {
    return fail("expected canonical dump to include target and module core summary");
  }
  if (!contains(dump, "function index=0 name=main") ||
      !contains(dump, "declaration=0 blocks=1 view=1") ||
      !contains(dump, "function index=1 name=extern_fn") ||
      !contains(dump, "declaration=1 blocks=0 view=0") ||
      !contains(dump, "function index=2 name=missing_facts") ||
      !contains(dump, "declaration=0 blocks=1 view=0")) {
    return fail("expected canonical dump to include all/defined traversal and view admission");
  }
  if (!contains(dump, "global index=0 name=global_i32") ||
      !contains(dump, "string index=0 name=.str0 bytes=3 bytes_hex=616263 align=1") ||
      !contains(dump, "function_view name=main") ||
      !contains(dump, "control_flow function=main blocks=1") ||
      !contains(dump, "value_locations function=main homes=1 move_bundles=0") ||
      !contains(dump, "value_home function=main value_id=1 value_name=%v") ||
      !contains(dump, "addressing function=main accesses=0 materializations=0") ||
      !contains(dump, "lookups function=main") ||
      !contains(dump, "value_homes=1") ||
      !contains(dump, "block function=main index=0 label=entry") ||
      !contains(dump, "cursor function=main block_index=0 instruction_index=0 label=entry bound=1")) {
    return fail("expected canonical dump to include view-exposed core function facts");
  }
  if (contains(dump, "prepare-note-sentinel-not-core") ||
      contains(dump, "diagnostic-phase-sentinel") ||
      contains(dump, "phase-sentinel-not-core") ||
      contains(dump, "semantic_bir_shared")) {
    return fail("expected canonical dump to exclude diagnostic and prepared-history fields");
  }

  return 0;
}

int verify_core_view_structural_comparison() {
  const auto lhs_module = make_fixture();
  const auto rhs_module = make_fixture();
  const prepared::PreparedMirCoreView lhs_view(lhs_module);
  const prepared::PreparedMirCoreView rhs_view(rhs_module);

  const auto equal_report = prepared::compare_prepared_mir_core_views(lhs_view, rhs_view);
  if (!equal_report.equal() ||
      equal_report.status != prepared::PreparedMirCoreComparisonStatus::Equal ||
      !equal_report.blocking_differences.empty()) {
    return fail("expected identical core view snapshots to compare equal");
  }

  auto diagnostic_only_module = make_fixture();
  diagnostic_only_module.completed_phases.push_back("another-phase-not-core");
  diagnostic_only_module.notes.push_back(prepare::PrepareNote{
      .phase = "another-diagnostic-phase",
      .message = "another-prepare-note-not-core",
  });
  const prepared::PreparedMirCoreView diagnostic_only_view(diagnostic_only_module);
  const auto diagnostic_report =
      prepared::compare_prepared_mir_core_views(lhs_view, diagnostic_only_view);
  if (!diagnostic_report.equal()) {
    return fail("expected diagnostic and prepared-history changes to be excluded");
  }

  auto mutated_module = make_fixture();
  mutated_module.module.string_constants.front().bytes[1] = 'z';
  const prepared::PreparedMirCoreView mutated_view(mutated_module);
  const auto difference_report =
      prepared::compare_prepared_mir_core_views(lhs_view, mutated_view);
  if (difference_report.equal() ||
      difference_report.status != prepared::PreparedMirCoreComparisonStatus::Different ||
      !has_blocking_difference(difference_report, "string_constants")) {
    return fail("expected typed string-constant core fact mutation to block equality");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = verify_core_view(); status != 0) {
    return status;
  }
  if (const int status = verify_core_view_canonical_dump(); status != 0) {
    return status;
  }
  return verify_core_view_structural_comparison();
}

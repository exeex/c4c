#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

c4c::hir::Module lower_hir_module(std::string_view source) {
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::C));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::C,
                     "frontend_lir_label_address_rvalue_probe_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));
  expect_true(result.validation.ok, "fixture source should parse and validate");
  expect_true(result.hir_module.has_value(), "fixture source should lower to HIR");
  return *result.hir_module;
}

const c4c::codegen::lir::LirFunction& require_function(
    const c4c::codegen::lir::LirModule& module, std::string_view name) {
  const auto it = std::find_if(module.functions.begin(), module.functions.end(),
                               [&](const c4c::codegen::lir::LirFunction& function) {
                                 return function.name == name;
                               });
  expect_true(it != module.functions.end(), "fixture function should lower into LIR");
  return *it;
}

c4c::codegen::lir::LirFunction& require_function(c4c::codegen::lir::LirModule& module,
                                                  std::string_view name) {
  return const_cast<c4c::codegen::lir::LirFunction&>(
      require_function(static_cast<const c4c::codegen::lir::LirModule&>(module), name));
}

const c4c::codegen::lir::LirStoreOp& require_pointer_initializer_store(
    const c4c::codegen::lir::LirFunction& function) {
  const c4c::codegen::lir::LirStoreOp* result = nullptr;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst);
      if (store != nullptr && store->type_str == "ptr") {
        expect_true(result == nullptr,
                    "fixture should have one immediate pointer initializer store");
        result = store;
      }
    }
  }
  expect_true(result != nullptr,
              "automatic local label address should reach a structured pointer store");
  return *result;
}

void expect_verifier_rejects(c4c::codegen::lir::LirModule module,
                             const std::string& message) {
  try {
    c4c::codegen::lir::verify_module(module);
    fail(message);
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }
}

c4c::codegen::lir::LirStoreOp& require_pointer_initializer_store(
    c4c::codegen::lir::LirFunction& function) {
  return const_cast<c4c::codegen::lir::LirStoreOp&>(
      require_pointer_initializer_store(static_cast<const c4c::codegen::lir::LirFunction&>(function)));
}

}  // namespace

int main() {
  const c4c::hir::Module hir_module = lower_hir_module(R"c(
int automatic_local_label_address(void) {
  void *target = &&dispatch;
  return target != 0;
dispatch:
  return 1;
}
)c");
  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  const c4c::codegen::lir::LirFunction& function =
      require_function(lir_module, "automatic_local_label_address");
  const c4c::codegen::lir::LirStoreOp& store =
      require_pointer_initializer_store(function);

  expect_true(store.type_str == "ptr",
              "automatic local label-address initializer should reach a pointer store seam");
  expect_true(store.val.kind() == c4c::codegen::lir::LirOperandKind::DirectConstant,
              "label-address initializer must use a native direct constant");
  const c4c::codegen::lir::LirValueId* produced_value = store.val.value_id();
  expect_true(produced_value != nullptr && produced_value->valid(),
              "label-address initializer must retain a valid produced-value identity");
  expect_true(function.direct_label_address_constants.size() == 1,
              "label-address initializer must publish one direct constant");
  const auto& direct = function.direct_label_address_constants.front();
  expect_true(direct.owner == function.link_name_id,
              "direct label address must retain its current function owner");
  expect_true(direct.target.valid(),
              "direct label address must retain a valid target label");
  expect_true(direct.type == c4c::codegen::lir::LirTypeRef(c4c::codegen::lir::LirBuiltinType::Pointer),
              "direct label address must be typed as a pointer");
  expect_true(direct.value == *produced_value,
              "direct label address must retain the produced-value identity used by its store");
  expect_true(std::any_of(function.blocks.begin(), function.blocks.end(), [&](const auto& block) {
                return block.id == direct.target;
              }),
              "direct label address target must belong to its current function");
  c4c::codegen::lir::verify_module(lir_module);

  auto malformed = lir_module;
  require_pointer_initializer_store(require_function(malformed, "automatic_local_label_address")).val =
      c4c::codegen::lir::LirOperand::raw("blockaddress(@automatic_local_label_address, %ulbl_dispatch)");
  expect_true(require_pointer_initializer_store(
                  require_function(malformed, "automatic_local_label_address"))
                      .val.kind() != c4c::codegen::lir::LirOperandKind::DirectConstant,
              "raw label-address text must not satisfy the frontend direct-constant producer contract");

  malformed = lir_module;
  require_function(malformed, "automatic_local_label_address")
      .direct_label_address_constants.front().owner = c4c::kInvalidLinkName;
  expect_verifier_rejects(std::move(malformed),
                          "verifier must reject a direct label address without its current function owner");

  malformed = lir_module;
  require_function(malformed, "automatic_local_label_address")
      .direct_label_address_constants.front().owner = malformed.link_names.intern("foreign_owner");
  expect_verifier_rejects(std::move(malformed),
                          "verifier must reject a foreign direct label-address function owner");

  malformed = lir_module;
  require_function(malformed, "automatic_local_label_address")
      .direct_label_address_constants.front().target = c4c::codegen::lir::LirBlockId::invalid();
  expect_verifier_rejects(std::move(malformed),
                          "verifier must reject a direct label address without a target");

  malformed = lir_module;
  require_function(malformed, "automatic_local_label_address")
      .direct_label_address_constants.front().type = c4c::codegen::lir::LirTypeRef::integer(32);
  expect_verifier_rejects(std::move(malformed),
                          "verifier must reject a non-pointer direct label-address constant");

  malformed = lir_module;
  require_function(malformed, "automatic_local_label_address").direct_label_address_constants.clear();
  expect_verifier_rejects(std::move(malformed),
                          "verifier must reject a direct label-address store with no producer");

  malformed = lir_module;
  require_function(malformed, "automatic_local_label_address")
      .direct_label_address_constants.front().value = c4c::codegen::lir::LirValueId::invalid();
  expect_verifier_rejects(std::move(malformed),
                          "verifier must reject an invalid direct label-address produced identity");

  malformed = lir_module;
  require_pointer_initializer_store(require_function(malformed, "automatic_local_label_address")).val =
      c4c::codegen::lir::LirOperand::direct_constant(c4c::codegen::lir::LirValueId{999});
  expect_verifier_rejects(std::move(malformed),
                          "verifier must reject a foreign direct label-address produced identity");

  const c4c::hir::Module table_hir_module = lower_hir_module(R"c(
int automatic_local_label_address_table_initializer(void) {
  void *table[] = { &&first, &&second };
  return table[0] != 0 && table[1] != 0;
first:
  return 1;
second:
  return 2;
}
)c");
  const c4c::codegen::lir::LirModule table_lir_module =
      c4c::codegen::lir::lower(table_hir_module);
  const c4c::codegen::lir::LirFunction& table_function = require_function(
      table_lir_module, "automatic_local_label_address_table_initializer");
  std::size_t direct_store_count = 0;
  for (const auto& block : table_function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst);
      if (store == nullptr || store->type_str != "ptr" ||
          store->val.kind() != c4c::codegen::lir::LirOperandKind::DirectConstant) {
        continue;
      }
      expect_true(store->val.value_id() != nullptr && store->val.value_id()->valid(),
                  "each automatic local table element must retain a produced direct identity");
      expect_true(store->ptr.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
                      store->ptr.value_id() != nullptr && store->ptr.value_id()->valid(),
                  "each automatic local table element must store through its indexed local slot");
      const auto direct = std::find_if(
          table_function.direct_label_address_constants.begin(),
          table_function.direct_label_address_constants.end(), [&](const auto& item) {
            return item.value == *store->val.value_id();
          });
      expect_true(direct != table_function.direct_label_address_constants.end() &&
                      direct->owner == table_function.link_name_id && direct->target.valid() &&
                      direct->type == c4c::codegen::lir::LirTypeRef(
                                          c4c::codegen::lir::LirBuiltinType::Pointer),
                  "each automatic local table element must retain pointer/current-function direct authority");
      ++direct_store_count;
    }
  }
  expect_true(direct_store_count == 2,
              "two automatic local table elements must reach generic indexed stores as direct constants");
  c4c::codegen::lir::verify_module(table_lir_module);

  std::cout << "PASS: frontend_lir_label_address_rvalue_probe\n";
  return 0;
}

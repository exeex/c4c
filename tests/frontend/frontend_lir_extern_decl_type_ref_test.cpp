#include "ir.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

c4c::codegen::lir::LirModule make_struct_module() {
  c4c::codegen::lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  c4c::codegen::lir::LirStructDecl pair_decl;
  pair_decl.name_id = pair_id;
  pair_decl.fields.push_back({c4c::codegen::lir::LirTypeRef("i32")});
  module.type_decls.push_back("%struct.Pair = type { i32 }");
  module.record_struct_decl(std::move(pair_decl));
  return module;
}

void push_recorded_extern_decl(c4c::codegen::lir::LirModule& module,
                               std::string_view name) {
  const auto it = module.extern_decl_name_map.find(std::string(name));
  expect_true(it != module.extern_decl_name_map.end(),
              "record_extern_decl should populate the raw-name dedup map");

  c4c::codegen::lir::LirExternDecl decl;
  decl.name = it->second.name;
  decl.return_type_str = it->second.return_type_str;
  decl.return_type = it->second.return_type;
  decl.link_name_id = it->second.link_name_id;
  module.extern_decls.push_back(std::move(decl));
}

}  // namespace

int main() {
  c4c::codegen::lir::LirModule module = make_struct_module();

  module.record_extern_decl("extern_pair", "%struct.Pair");
  push_recorded_extern_decl(module, "extern_pair");

  const auto& decl = module.extern_decls.front();
  expect_eq(decl.return_type_str, "%struct.Pair",
            "extern declaration should preserve return_type_str text");
  expect_eq(decl.return_type.str(), "%struct.Pair",
            "extern declaration return mirror text should match");
  expect_true(decl.return_type.has_struct_name_id(),
              "extern declaration return mirror should carry StructNameId");
  expect_eq(module.struct_names.spelling(decl.return_type.struct_name_id()),
            "%struct.Pair",
            "extern declaration StructNameId should resolve to return text");

  expect_eq(std::to_string(module.type_decls.size()), "1",
            "struct fixture should retain one legacy type_decls shadow");
  expect_eq(module.type_decls.front(), "%struct.Pair = type { i32 }",
            "legacy type_decls shadow should stay populated");

  c4c::codegen::lir::verify_module(module);
  const std::string llvm_ir = c4c::codegen::lir::print_llvm(module);
  expect_true(llvm_ir.find("%struct.Pair = type { i32 }") != std::string::npos,
              "printer should emit matching struct_decls/type_decls declaration");
  expect_true(llvm_ir.find("declare %struct.Pair @extern_pair(...)") !=
                  std::string::npos,
              "printer should keep using return_type_str for extern declarations");

  c4c::codegen::lir::LirModule mismatched_shadow = module;
  mismatched_shadow.type_decls.front() = "%struct.Pair = type { i64 }";
  try {
    c4c::codegen::lir::verify_module(mismatched_shadow);
    fail("verifier should reject mismatched struct_decls/type_decls shadows");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule missing_mirror = module;
  missing_mirror.extern_decls.front().return_type =
      c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(missing_mirror);
    fail("verifier should reject a known struct extern return without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  std::cout << "PASS: frontend_lir_extern_decl_type_ref\n";
  return 0;
}

#include "src/backend/bir/bir.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bir = c4c::backend::bir;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

bir::FunctionId function(bir::ModuleEpoch epoch, bir::SlotIndex slot,
                         bir::Generation generation = 1) {
  return bir::FunctionId{epoch, slot, generation};
}

std::vector<bir::FunctionRevisionEntry> canonical_entries(
    bir::ModuleEpoch epoch) {
  return {{function(epoch, 0), bir::FunctionRevision{3}},
          {function(epoch, 2), bir::FunctionRevision{8}},
          {function(epoch, 7), bir::FunctionRevision{13}}};
}

bir::FunctionRevisionDigest digest(
    bir::ModuleEpoch epoch,
    const std::vector<bir::FunctionRevisionEntry>& entries) {
  auto result = bir::compute_function_revision_digest(epoch, entries);
  expect(result.has_value(),
         "canonical revision entries should produce a digest");
  return result.value();
}

void test_stable_digest_and_typed_revision_axes() {
  static_assert(!std::is_same_v<bir::ModuleRevision, bir::FunctionRevision>);

  constexpr bir::ModuleEpoch epoch = 41;
  const auto entries = canonical_entries(epoch);
  const auto first = digest(epoch, entries);
  const auto second = digest(epoch, entries);
  const auto copied =
      digest(epoch, std::vector<bir::FunctionRevisionEntry>(entries));

  expect(first == second && first == copied,
         "repeated digest construction must be stable and deterministic");
  expect(first.fingerprint != bir::Fingerprint128{},
         "a populated function digest must not use the empty fingerprint value");

  auto changed_revision = entries;
  changed_revision[1].revision.value++;
  expect(digest(epoch, changed_revision) != first,
         "changing one function revision must change the digest");

  auto changed_generation = entries;
  changed_generation[1].function.generation++;
  expect(digest(epoch, changed_generation) != first,
         "changing one stable function identity must change the digest");

  const auto empty_first = digest(epoch, {});
  const auto empty_second = digest(epoch, {});
  expect(empty_first == empty_second && empty_first != first,
         "empty canonical function order must have its own stable digest");

  auto arbitrary_order = entries;
  std::swap(arbitrary_order[0], arbitrary_order[2]);
  const auto arbitrary_first = digest(epoch, arbitrary_order);
  const auto arbitrary_second = digest(epoch, arbitrary_order);
  expect(arbitrary_first == arbitrary_second && arbitrary_first != first,
         "caller-supplied canonical module order must be preserved and "
         "order-sensitive rather than numerically sorted");
}

void test_pipeline_stamp_checks_every_axis() {
  constexpr bir::ModuleEpoch epoch = 19;
  const auto original_digest = digest(epoch, canonical_entries(epoch));
  const bir::PipelineStageStamp original{
      epoch, bir::ModuleRevision{5}, original_digest};

  expect(original == original,
         "identical stage stamps must compare equal deterministically");

  auto changed_epoch = original;
  changed_epoch.epoch++;
  expect(changed_epoch != original,
         "module epoch must participate in stage freshness");

  auto changed_module_revision = original;
  changed_module_revision.module_revision.value++;
  expect(changed_module_revision != original,
         "module revision must participate in stage freshness");

  auto changed_entries = canonical_entries(epoch);
  changed_entries.back().revision.value++;
  auto changed_functions = original;
  changed_functions.function_revisions = digest(epoch, changed_entries);
  expect(changed_functions != original,
         "ordered function revisions must participate in stage freshness");
}

void expect_digest_error(
    bir::ModuleEpoch epoch,
    const std::vector<bir::FunctionRevisionEntry>& entries,
    bir::FunctionRevisionDigestErrorCode code, std::size_t index,
    const std::string& message) {
  auto result = bir::compute_function_revision_digest(epoch, entries);
  expect(!result.has_value(), message + " must not return a partial digest");
  expect(result.error().code == code && result.error().index == index,
         message + " must return its typed error and exact entry index");
}

void test_malformed_digest_input_is_rejected() {
  constexpr bir::ModuleEpoch epoch = 73;

  expect_digest_error(0, {},
                      bir::FunctionRevisionDigestErrorCode::InvalidModuleEpoch,
                      0, "zero module epoch");

  auto invalid = canonical_entries(epoch);
  invalid[1].function.generation = 0;
  expect_digest_error(epoch, invalid,
                      bir::FunctionRevisionDigestErrorCode::InvalidFunctionId,
                      1, "invalid function identity");

  auto foreign = canonical_entries(epoch);
  foreign[2].function.epoch = epoch + 1;
  expect_digest_error(epoch, foreign,
                      bir::FunctionRevisionDigestErrorCode::ForeignFunctionId,
                      2, "foreign function identity");

  auto duplicate_slot = canonical_entries(epoch);
  duplicate_slot[1].function.slot = duplicate_slot[0].function.slot;
  duplicate_slot[1].function.generation++;
  expect_digest_error(
      epoch, duplicate_slot,
      bir::FunctionRevisionDigestErrorCode::DuplicateFunctionSlot, 1,
      "duplicate live function slot");

  expect(digest(epoch, canonical_entries(epoch)) ==
             digest(epoch, canonical_entries(epoch)),
         "failed construction must not perturb later deterministic results");
}

bir::RawBir publish(bir::ModuleBuilder& builder, const std::string& context) {
  auto published = std::move(builder).publish();
  expect(published.has_value(), context + " must publish valid RawBir");
  return std::move(published).value();
}

bir::FunctionId add_void_function(bir::ModuleBuilder& builder,
                                  const std::string& name,
                                  bool is_declaration) {
  auto created = builder.create_function(
      bir::FunctionSignature{bir::Type{bir::TypeKind::Void}, {}, false}, name,
      is_declaration);
  expect(created.has_value(), "function construction must succeed");
  if (!is_declaration) {
    auto edited = builder.with_function(
        created.value(), [](bir::FunctionBuilder& function_builder) {
          auto block = function_builder.create_block("entry");
          if (!block)
            return bir::Result<void, bir::BuildError>::failure(block.error());
          return function_builder.set_terminator(block.value(),
                                                 bir::ReturnTerm{});
        });
    expect(edited.has_value(), "definition construction must succeed");
  }
  return created.value();
}

void test_published_storage_owns_exact_initial_stamp() {
  bir::ModuleBuilder empty_builder;
  auto empty = publish(empty_builder, "empty module");
  const auto empty_view = empty.view();
  const auto empty_digest =
      bir::compute_function_revision_digest(empty_view.epoch(), {});
  expect(empty_digest.has_value(), "empty module digest must be computable");
  expect(empty.stage_stamp() ==
             bir::PipelineStageStamp{empty_view.epoch(), bir::ModuleRevision{0},
                                     empty_digest.value()},
         "empty RawBir must freeze the exact initial storage stamp");
  expect(empty_view.revision() == bir::ModuleRevision{0},
         "builder publication must not count construction as a transform");

  bir::ModuleBuilder populated_builder;
  const auto declaration =
      add_void_function(populated_builder, "decl_first", true);
  const auto definition =
      add_void_function(populated_builder, "definition_second", false);
  auto raw = publish(populated_builder, "populated module");
  const auto raw_view = raw.view();
  expect(raw_view.functions() ==
             std::vector<bir::FunctionId>{declaration, definition},
         "declarations and definitions must retain explicit module order");

  std::vector<bir::FunctionRevisionEntry> revisions;
  for (const auto id : raw_view.functions()) {
    auto function_view = raw_view.function(id);
    expect(function_view.has_value(), "ordered function must resolve");
    expect(function_view.value().revision() == bir::FunctionRevision{0},
           "construction edits must leave each function revision at zero");
    revisions.push_back({id, function_view.value().revision()});
  }
  auto expected_digest =
      bir::compute_function_revision_digest(raw_view.epoch(), revisions);
  expect(expected_digest.has_value(), "live ordered revisions must digest");
  expect(raw.stage_stamp().function_revisions == expected_digest.value(),
         "RawBir digest must use explicit function order and live revisions");
  expect(raw.stage_stamp().epoch == raw_view.epoch() &&
             raw.stage_stamp().module_revision == raw_view.revision(),
         "RawBir stamp axes must exactly match owned storage");

  const auto raw_stamp = raw.stage_stamp();
  auto canonicalized = bir::canonicalize(std::move(raw));
  expect(canonicalized.has_value(), "valid RawBir must canonicalize");
  auto canonical = std::move(canonicalized).value();
  expect(canonical.stage_stamp() == raw_stamp,
         "no-op canonicalization must preserve the exact storage stamp");
  expect(canonical.view().revision() == bir::ModuleRevision{0},
         "no-op canonicalization must not bump module revision");
}

void test_separate_builders_receive_distinct_epochs() {
  bir::ModuleBuilder first_builder;
  bir::ModuleBuilder second_builder;
  add_void_function(first_builder, "same_shape", true);
  add_void_function(second_builder, "same_shape", true);
  auto first = publish(first_builder, "first same-shape module");
  auto second = publish(second_builder, "second same-shape module");
  expect(first.stage_stamp().epoch != second.stage_stamp().epoch,
         "same-shape modules from separate builders need distinct epochs");
  expect(first.stage_stamp() != second.stage_stamp(),
         "module epoch must distinguish otherwise identical initial storage");
}

}  // namespace

int main() {
  test_stable_digest_and_typed_revision_axes();
  test_pipeline_stamp_checks_every_axis();
  test_malformed_digest_input_is_rejected();
  test_published_storage_owns_exact_initial_stamp();
  test_separate_builders_receive_distinct_epochs();
  return 0;
}

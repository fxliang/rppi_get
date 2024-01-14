#pragma once

#include "alias.hpp"
#include <optional>
#include <string>
#include <vector>

#define DEFINE_OPTIONAL_FIELD_FROM_JSON(Name, Variable, Type)                  \
  if (j.contains(Name)) {                                                      \
    Variable = j.at(Name).get<Type>();                                         \
  }

namespace spec {

struct Recipe {
  std::string repo;
  std::optional<std::string> branch;
  std::string name;
  std::optional<StrVec> labels;
  StrVec schemas;
  std::optional<StrVec> dependencies;
  std::optional<StrVec> reverse_dependencies;
  std::optional<std::string> license;
  std::string local_path = "";
};

struct Category {
  std::string key;
  std::string name;
};

struct ParentIndex {
  std::vector<Category> categories;
};

struct ChildIndex {
  std::vector<Recipe> recipes;
};

void from_json(const json &j, Recipe &r);
void from_json(const json &j, Category &c);
void from_json(const json &j, ParentIndex &pi);
void from_json(const json &j, ChildIndex &ci);

using RecipeVec = std::vector<Recipe>;

/**
 * Gets all the recipes from RPPI
 *
 * @param root, the name of the directory where lies a `index.json`
 */
void extract_recipes_from_rppi(const std::string &root, RecipeVec &recipes);

void print_recipes(const RecipeVec &recipes);

// Filters recipes with `keyword`, for recipe searching
RecipeVec filter_recipes(const RecipeVec &recipes, const std::string &keyword,
                         bool strict);

} // namespace spec

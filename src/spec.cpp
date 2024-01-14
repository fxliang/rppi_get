#include "spec.hpp"
#include "json.hpp"
#include <iostream>
#include <regex>

namespace spec {

void from_json(const json &j, Recipe &r) {
  j.at("repo").get_to(r.repo);
  j.at("name").get_to(r.name);
  j.at("schemas").get_to(r.schemas);

  DEFINE_OPTIONAL_FIELD_FROM_JSON("branch", r.branch, std::string)
  DEFINE_OPTIONAL_FIELD_FROM_JSON("labels", r.labels, StrVec)
  DEFINE_OPTIONAL_FIELD_FROM_JSON("dependencies", r.dependencies, StrVec)
  DEFINE_OPTIONAL_FIELD_FROM_JSON("reverseDependencies", r.reverse_dependencies,
                                  StrVec)
  DEFINE_OPTIONAL_FIELD_FROM_JSON("license", r.license, std::string)
}

void from_json(const json &j, Category &c) {
  j.at("key").get_to(c.key);
  j.at("name").get_to(c.name);
}

void from_json(const json &j, ParentIndex &pi) {
  j.at("categories").get_to(pi.categories);
}

void from_json(const json &j, ChildIndex &ci) {
  j.at("recipes").get_to(ci.recipes);
}

void extract_recipes_from_rppi(const std::string &root, RecipeVec &recipes) {
  json j = jsonutils::load_from_file(root + "/index.json");
  if (j.contains("categories")) {
    spec::ParentIndex pi = j;
    RecipeVec temp;
    for (const auto &category : pi.categories) {
      extract_recipes_from_rppi(root + "/" + category.key, temp);
      recipes.insert(recipes.end(), temp.begin(), temp.end());
    }
  } else if (j.contains("recipes")) {
    spec::ChildIndex ci = j;
    recipes = ci.recipes;
  }
}

void print_recipes(const RecipeVec &recipes) {
  for (const auto &r : recipes)
    std::cout << "name: " << r.name << ", repo: "
              << r.repo
              //<< ", category: " << r.category
              //<< ", schemas: " << join_string_vector(r.schemas)
              //<< ", dependencies: " << join_string_vector(r.dependencies)
              //<< ", reverseDependencies: " <<
              // join_string_vector(r.reverseDependencies)
              << std::endl;
}

RecipeVec filter_recipes(const RecipeVec &recipes, const std::string &keyword,
                         bool strict = false) {
  std::vector<spec::Recipe> res;
  const auto regex =
      std::regex(".*" + keyword + ".*", std::regex_constants::icase);
  for (const auto &r : recipes) {
    if ((!strict && (std::regex_match(r.name, regex) ||
                     std::regex_match(r.repo, regex)) ||
         (strict && (r.repo == keyword || r.name == keyword))))
      res.emplace_back(r);
  }
  return res;
}

} // namespace spec

#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <set>
#include <sstream>

using namespace std::literals;


FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const { return category_; }

bool FormulaError::operator==(FormulaError rhs) const { return GetCategory() == rhs.GetCategory(); }

std::string_view FormulaError::ToString() const {
    using namespace std::literals;
    switch (category_) {
    case Category::Ref:
        return "#REF!"s;
        break;
    case Category::Value:
        return "#VALUE!"s;
        break;
    case Category::Arithmetic:
        return "#ARITHM!"s;
        break;
    
    default:
        break;
    }
    return ""sv;
}
std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression)) {}
    catch (const FormulaException& exc) {
        throw exc;
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        Value val;
        try {
            val = ast_.Execute(sheet);
        } catch (const FormulaError& exc) {
            val = exc;
        }
        return val;
    }
    
    std::string GetExpression() const override {
        std::ostringstream os;
        ast_.PrintFormula(os);
        return os.str();
    }

    // Возвращает список ячеек, которые непосредственно задействованы в вычислении
    // формулы. Список отсортирован по возрастанию и не содержит повторяющихся
    // ячеек.
    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> list{ast_.GetCells().begin(), ast_.GetCells().end()};
        list.erase(std::unique(list.begin(),list.end()),list.end());
        std::sort(list.begin(), list.end());
        return list;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Parsing formula from expression was failure"s);
    }
}
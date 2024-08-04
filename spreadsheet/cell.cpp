#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue(const SheetInterface&) const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class Cell::EmptyImpl final : public Impl {
public:
    EmptyImpl() = default;
    CellInterface::Value GetValue(const SheetInterface&) const override {
        return double(0);
    }
    std::string GetText() const override {
        return "";
    }
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
};

class Cell::TextImpl final : public Impl {
public:
    TextImpl(std::string text) 
        : text_(std::move(text)) {
    }
    CellInterface::Value GetValue(const SheetInterface&) const override {
        if (text_.empty()) {
            return "";
        }
        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1, text_.size() - 1);
        }
        return text_;
    }
    std::string GetText() const override {
        return text_;
    }
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
private:
    std::string text_;
};

class Cell::FormulaImpl final : public Impl {
public:
    FormulaImpl(std::string text) 
        : formula_(ParseFormula(text)) {
    }
    CellInterface::Value GetValue(const SheetInterface& sheet) const override {

        FormulaInterface::Value formula_val = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(formula_val)) {
            return std::get<double>(formula_val);
        }
        return std::get<FormulaError>(formula_val);
    }

    std::string GetText() const override {
        using namespace std::literals;
        return "="s + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }


private:
    std::unique_ptr<FormulaInterface> formula_;
};

// Реализуйте следующие методы
Cell::Cell(SheetInterface& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

Cell::~Cell() = default;

Cell* GetCell(SheetInterface& sheet, Position pos) {
    auto cell_it = sheet.GetCell(pos);
    return cell_it == nullptr ? nullptr : dynamic_cast<Cell*>(cell_it);
}

Cell* GetOrCreate(SheetInterface& sheet, Position pos) {
    if (!sheet.GetCell(pos)) {
        sheet.SetCell(pos, "");
    }
    return GetCell(sheet, pos);
}

void Cell::SwapImpl(std::unique_ptr<Impl>&& src) {
    impl_ = std::move(src);
    for (const Position pos : impl_->GetReferencedCells()) {
        Cell* cell = GetOrCreate(sheet_, pos);
        referenced_cells_.insert(cell);
        cell->dependent_cells_.insert(this);
    }
}

void Cell::MakeFormula(std::string text) {
    std::unique_ptr<Impl> tmp_impl = std::make_unique<FormulaImpl>(text.substr(1));
    std::vector<Position> tmp_ref_cells = std::move(tmp_impl->GetReferencedCells());
    if (!tmp_ref_cells.empty()) {
        CheckCircularDependency(tmp_ref_cells);
    }
    SwapImpl(std::move(tmp_impl));
}

void Cell::Set(std::string text) {
    InvalidateCache();
    if (text.empty()) {
        Clear();
    } else if (text.at(0) == FORMULA_SIGN && text.size() > 1) {
        MakeFormula(text);
    } else {
        impl_ = std::make_unique<TextImpl>(text);
    }
}

void Cell::CheckCircularDependency(std::vector<Position>& ref_cells) {
    std::unordered_set<Cell *> visited_cells;
    CheckCircularDependencyRef(ref_cells, visited_cells);
}

void Cell::CheckCircularDependencyRef(std::vector<Position>& ref_cells
                                , std::unordered_set<Cell*>& visited) {
    for (const Position pos : ref_cells) {

        Cell* cell_ptr = GetCell(sheet_, pos);
        if (cell_ptr == this) {
            throw CircularDependencyException{"circular dependency"};
        }
        if (cell_ptr && visited.count(cell_ptr) == 0) {
            auto new_ref_cells = cell_ptr->GetReferencedCells();
            if (!new_ref_cells.empty()) {
                CheckCircularDependencyRef(new_ref_cells, visited);
            }
            visited.insert(this);
        }
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = impl_->GetValue(sheet_);
    }
    return cache_.value();
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

void Cell::InvalidateCache() {
    cache_.reset();
    for (const auto cell : dependent_cells_) {
        cell->InvalidateCache();
    }
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

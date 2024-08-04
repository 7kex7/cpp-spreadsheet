#pragma once

#include "common.h"
#include "formula.h"
#include <memory>
#include <optional>
#include <unordered_set>

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

private:
    void InvalidateCache();

    /* cycle dependency checkers */
    void CheckCircularDependency(std::vector<Position>& ref_cells);
    void CheckCircularDependencyRef(std::vector<Position>& ref_cells
                                , std::unordered_set<Cell*>& visited);

    void MakeFormula(std::string text);
    void SwapImpl(std::unique_ptr<Impl>&& src);

private:
    std::unique_ptr<Impl> impl_;
    std::unordered_set<Cell*> dependent_cells_; /* cache invalidation */
    std::unordered_set<Cell*> referenced_cells_; /* cycle deps checking */
    SheetInterface& sheet_;
    mutable std::optional<Value> cache_;
};
#pragma once

#include "common.h"
#include "formula.h"
#include <memory>
#include <optional>
#include <unordered_set>

class Cell : public CellInterface {
public:
    Cell();
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    bool IsCircularDependency(const std::unordered_set<const Cell*>& visited) const;
    void InvalidateCache();

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual CellInterface::Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual std::optional<FormulaInterface::Value> GetCache() const = 0;
        virtual void InvalidateCache() = 0;
    };

    class EmptyImpl final : public Impl {
    public:
        EmptyImpl() = default;
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl final : public Impl {
    public:
        TextImpl(std::string text);
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
    private:
        std::string text_;
    };

    class FormulaImpl final : public Impl {
    public:
        FormulaImpl(std::string text);
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
        virtual std::vector<Position> GetReferencedCells() const override;
        void InvalidateCache() override;
        std::optional<FormulaInterface::Value> GetCache() const override;

    private:
        std::unordered_set<const Cell*> referenced_cells_;
        mutable std::optional<Value> cache_;
        std::unique_ptr<FormulaInterface> formula_;
    };

private:
    std::unique_ptr<Impl> impl_;
    std::unordered_set<const Cell*> depended_cells_;
    SheetInterface& sheet_;
};
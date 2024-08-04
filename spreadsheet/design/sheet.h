#pragma once

#include "cell.h"
#include "common.h"

#include <list>
#include <functional>
#include <iterator>
#include <memory>


class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override; 

private:
    void CheckPushSize(Position pos); // with SetCell
    void EraseSize(Position pos); // with ClearCell

private:
    struct Impl {
        Position top_left_pos_{0, 0};
        Size size_{0, 0};
        std::list<Cell> contents_;

        // contain indices to access (get, erase, create new) cells by O(1)
        // in format row_indices_ <x1: <y1: deque_idx1,...>, <x2: ...>>
        // in format col_indices_ <y1: <x1: deque_idx1,...>, <y2: ...>>
        using cell_iterator = std::list<Cell>::iterator;
        using const_cell_iterator = std::list<Cell>::const_iterator;
        using IndexTable = std::unordered_map<int,
                            std::unordered_map<int, cell_iterator>>;
        // using IndexTable = std::std::unordered_map<int, std::unordered_map<int, int>>;
        IndexTable rows_indices_;
        IndexTable cols_indices_;

    public:
        // returns end iterator if there is no cell with the position
        cell_iterator FindIterator(Position pos);
        const_cell_iterator FindIterator(Position pos) const;
        
        const_cell_iterator EndContents() const;
        cell_iterator EndContents();
    };
    std::unique_ptr<Impl> impl_ = nullptr;
};


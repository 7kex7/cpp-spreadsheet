#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <cassert>
#include <vector>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

struct Sheet::Impl {
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
    cell_iterator FindIterator(Position pos) {
        auto row_it = rows_indices_.find(pos.row);
        if (row_it == rows_indices_.end()) {
            return EndContents();
        }
        auto cell_idx = row_it->second.find(pos.col);
        if (cell_idx == row_it->second.end()) {
            return EndContents();
        }

        return cell_idx->second;
    }
    const_cell_iterator FindIterator(Position pos) const {
        return static_cast<const_cell_iterator>(FindIterator(pos));
    }
    
    const_cell_iterator EndContents() const {
        return contents_.end();
    }
    cell_iterator EndContents() {
        return contents_.end();
    }
};

Sheet::Sheet() : impl_(std::make_unique<Impl>()) {}
Sheet::~Sheet() {}

void Sheet::CheckPushSize(Position pos) {
    if(pos.row + 1 > impl_->size_.rows) {
        impl_->size_.rows = pos.row + 1;
    }
    if(pos.col + 1 > impl_->size_.cols) {
        impl_->size_.cols = pos.col + 1;
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position {" + pos.row + ',' + pos.col + '}');
    }
    if (auto it = impl_->FindIterator(pos); it != impl_->EndContents()) {
        it->Set(text);
    } else {
        impl_->contents_.emplace_back(*this);
        it = std::prev(impl_->EndContents());
        impl_->rows_indices_[pos.row][pos.col] = it;
        impl_->cols_indices_[pos.col][pos.row] = it;
        impl_->contents_.back().Set(text);
    }

    CheckPushSize(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position {" + pos.row + ',' + pos.col + '}');
    }
    const auto it = impl_->FindIterator(pos);
    return it == impl_->EndContents() ? nullptr : &(*it);
}   

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(const_cast<const Sheet*>(this)->GetCell(pos));
}

using IndexTable = std::unordered_map<int, std::unordered_map<int, std::list<Cell>::iterator>>;
void RemoveFromIndexTable(int i, int j
        , IndexTable& table) {
    auto i_it = table.find(i);
    assert(i_it != table.end() && "RemoveFromIndexTable err: wrong i idx");
    if (i_it->second.size() == 1) {
        // as example, if it is last element on row -> remove whole row from table;
        table.erase(i_it);
    } else {
        auto j_it = i_it->second.find(j);
        assert(j_it != i_it->second.end() && "RemoveFromIndexTable err: wrong j idx");
        i_it->second.erase(j_it);
    }
}

// -1 if no next index
int FindSameOrNextIndex(int idx, IndexTable& table, bool backward = false) {
    if (backward) {
        for (; table.find(idx) == table.end() && idx > 0; --idx) {}
        return idx;
    } else {
        for (; table.find(idx) == table.end() && idx < 20000; ++idx) {}
        return idx;
    }
}

// after removal
void Sheet::EraseSize(Position pos) {

    Size& size_ref = impl_->size_;
    if (size_ref == Size{1, 1}) {
        size_ref = Size{0, 0};
        return;
    }

    Position brp{size_ref.rows - 1, size_ref.cols - 1};
    if (brp.row == pos.row) {
        brp.row = FindSameOrNextIndex(pos.row, impl_->rows_indices_, true);
        size_ref.rows -= pos.row - brp.row;
    }
    if (brp.col == pos.col) {
        brp.col = FindSameOrNextIndex(pos.col, impl_->cols_indices_, true);
        size_ref.cols -= pos.col - brp.col;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position {" + pos.row + ',' + pos.col + '}');
    }
    auto it = impl_->FindIterator(pos);
    if (it == impl_->EndContents()) {
        return;
    }
    impl_->contents_.erase(it);
    RemoveFromIndexTable(pos.row, pos.col, impl_->rows_indices_);
    RemoveFromIndexTable(pos.col, pos.row, impl_->cols_indices_);
    EraseSize(pos);
}

Size Sheet::GetPrintableSize() const {
    return impl_->size_;
}

void PrintVal(std::ostream& output, CellInterface::Value val) {
    if (std::holds_alternative<double>(val)) {
        output << std::get<double>(val);
    } else if (std::holds_alternative<std::string>(val)) {
        output << std::get<std::string>(val);
    } else if (std::holds_alternative<FormulaError>(val)) {
        output << std::get<FormulaError>(val);
    }
}
void Sheet::PrintValues(std::ostream& output) const {
    for (int row = 0; row < impl_->size_.rows; ++row) {
        for (int col = 0; col < impl_->size_.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (auto it = impl_->FindIterator({row, col}); it != impl_->EndContents()) {
                PrintVal(output, it->GetValue());
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    for (int row = 0; row < impl_->size_.rows; ++row) {
        for (int col = 0; col < impl_->size_.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (auto it = impl_->FindIterator({row, col}); it != impl_->EndContents()) {
                output << it->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
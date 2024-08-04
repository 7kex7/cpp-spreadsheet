#pragma once

#include "common.h"
#include "cell.h"

#include <list>
#include <functional>
#include <unordered_map>
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
    struct Impl;
    std::unique_ptr<Impl> impl_ = nullptr;
};


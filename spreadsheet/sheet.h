#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

#include <vector>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;
    Size GetPrintableSize() const override;
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;
    bool HasCyclic(Position pos, const Cell& cell);

private:
    std::vector<std::vector<std::unique_ptr<Cell>> > cells_;
    Size size_ = {0, 0};

    std::unordered_set<Position, PositionHasher> fill_positions_;
    
    void CheckValidPos(Position pos) const;
    void ProcessEmptyCell(Position pos, std::unique_ptr<Cell>& cell);
    void ProcessNotEmptyCell(Position pos, std::unique_ptr<Cell>& cell);
    
    void ChangeRowSize();
    void ChangeColSize();
};

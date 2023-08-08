#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <set>
#include <optional>

class Sheet;

class PositionHasher {
public:
    size_t operator()(const Position position) const {
        return static_cast<size_t>(hasher_(position.row) + 37 * hasher_(position.col));
    }

    std::hash<int> hasher_;
};

class Cell : public CellInterface {
public:
    Cell(const SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

    void SetParents(SheetInterface& sheet);
    const std::unordered_set<Position, PositionHasher>& GetParents() const;
    void SetChildren(const SheetInterface& sheet, Position add_pos);
    void ClearChildren(Position parent, const SheetInterface& sheet, Position del_pos);

    void InvalidCash(const SheetInterface& sheet);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;
    const SheetInterface& sheet_;

    std::unordered_set<Position, PositionHasher> parents_; // ячейки, от которых зависит выражение
    std::unordered_set<Position, PositionHasher> children_; // ячейки, которые зависят от данной

    mutable std::optional<double> cash_;
};

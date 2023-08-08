#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


class Cell::Impl {
public:
    using Value = std::variant<std::string, double, FormulaError>;

    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return "";
    }
    std::string GetText() const override {
        return "";
    }
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
};

class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string text)
    : text_(std::move(text)) {
    }

    Value GetValue() const override {
        if (text_.at(0) != '\'') {
            return GetText();
        } else {
            return text_.substr(1, text_.size() - 1);
        }
    }
    std::string GetText() const override {
        return text_;
    }
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
    void Set(std::string text) {
        text_ = std::move(text);
    }
    void Clear() {
        text_.clear();
    }

    ~TextImpl() {
        Clear();
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(std::string text, const SheetInterface& sheet)
    : sheet_(sheet) {
        try {
            text_ = ParseFormula(std::move(text));
        } catch (...) {
            throw FormulaException("");
        }
    }

    Value GetValue() const override {

        if (std::holds_alternative<double>(text_->Evaluate(sheet_))) {
            return std::get<double>(text_->Evaluate(sheet_));
        } else {
            return std::get<FormulaError>(text_->Evaluate(sheet_));
        }
    }
    std::string GetText() const override {
        return "=" + text_->GetExpression();
    }
    std::vector<Position> GetReferencedCells() const override {
        return text_->GetReferencedCells();
    }
    void Set(std::string text) {
        text_ = ParseFormula(std::move(text));
    }
    void Clear() {
        text_.reset(nullptr);
    }

    ~FormulaImpl() {
        Clear();
    }

private:
    std::unique_ptr<FormulaInterface> text_;
    const SheetInterface& sheet_;
};

Cell::Cell(const SheetInterface& sheet) 
: sheet_(sheet) {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::~Cell() {
    impl_.reset(nullptr);
}

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.at(0) == '=' && text.size() != 1) {
        try {
            impl_ = std::make_unique<FormulaImpl>(text.substr(1, text.size() - 1), sheet_);
        } catch (const FormulaException& fer) {
            throw fer;
        }
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    if (cash_ != std::nullopt) {
        return *cash_;
    }
    Cell::Value value = impl_->GetValue();
    if (std::holds_alternative<double>(value)) {
        cash_ = std::make_optional<double>( std::get<double>(value) );
    }

    return value;
}
std::string Cell::GetText() const {
    return impl_->GetText();
}
std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !GetReferencedCells().empty();
}

void Cell::SetParents(SheetInterface& sheet) {
    for (const auto& p: GetReferencedCells()) {
        if (!p.IsValid()) {
            throw FormulaError(FormulaError::Category::Ref);
        }

        if (sheet.GetCell(p) == nullptr) {
            sheet.SetCell(p, "");
        }

        parents_.insert(p);
    }
}
const std::unordered_set<Position, PositionHasher>& Cell::GetParents() const {
    return parents_;
}
void Cell::SetChildren(const SheetInterface& sheet, Position add_pos) {
    for (const auto& p: parents_) {
        if (sheet.GetCell(p) == nullptr) {
            return;
        } else {
            Cell* cell = dynamic_cast<Cell*>(const_cast<CellInterface*>(sheet.GetCell(p)));
            cell->children_.insert(add_pos);
        }
    }
}
void Cell::ClearChildren(Position parent, const SheetInterface& sheet, Position del_pos) {
    Cell* p = dynamic_cast<Cell*>(const_cast<CellInterface*>(sheet.GetCell(parent)));
    p->children_.erase(del_pos);
}

void Cell::InvalidCash(const SheetInterface& sheet) {
    cash_.reset();
    for (const auto& p: children_) {
        Cell* cell = dynamic_cast<Cell*>(const_cast<CellInterface*>(sheet.GetCell(p)));
        cell->InvalidCash(sheet);
    }
}

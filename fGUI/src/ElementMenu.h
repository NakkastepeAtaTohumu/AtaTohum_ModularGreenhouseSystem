#pragma once

#ifndef ElementMenu_h
#define ElementMenu_h

class Menu;
class ElementMenu;

class MenuElement {
public:
    virtual void OnEnter() {
    }

    virtual void Draw() {
    }

    virtual void OnExit() {
    }

    virtual bool isSelectable() {
        return false;
    }
    virtual bool isHighlightable() {
        return false;
    }
    virtual bool isPhysicalButton() {
        return false;
    }

    TFT_eSprite* d = nullptr;
    ElementMenu* m;
};

class HighlightableMenuElement : public MenuElement {
public:
    virtual void OnHighlight() {

    }

    virtual void OnUnhighlight() {

    }

    virtual void OnClick() {

    }

    bool isHighlighted = false;
    String tooltip = "";

private:
    bool isHighlightable() {
        return true;
    };
};

class PhysicalButtonInteractiveMenuElement : public MenuElement {
public:
    virtual void OnClick() {

    }

    int btn;
private:
    bool isPhysicalButton() override {
        return true;
    };
};

class SelectableMenuElement : public MenuElement {
public:
    virtual void OnSelect() {

    }

    virtual void OnDeselect() {

    }

    virtual void OnHighlight() {

    }

    virtual void OnUnhighlight() {

    }

    virtual void OnScroll(bool direction) {

    }

    bool isHighlighted = false;
    bool isSelected = false;

private:
    bool isHighlightable() {
        return true;
    };
    bool isSelectable() {
        return true;
    };
};

class ElementMenu : public Menu {
public:
    uint32_t bg_color = TFT_BLACK;

    void Draw() override {
        if (bg_color != TFT_TRANSPARENT && bg)
            d->fillScreen(bg_color);

        for (int i = 0; i < elementNum; i++)
            Elements[i]->Draw();
    }

    HighlightableMenuElement* GetHighlightedElement() {
        if (HighlightedElement != -1)
            return (HighlightableMenuElement*)Elements[HighlightedElement];
        else
            return nullptr;
    }

protected:
    int AddElement(MenuElement* e) {
        e->d = d;
        e->m = this;
        Elements[elementNum] = e;
        return elementNum++;
    }

    void RemoveElement(int index) {
        delete Elements[index];

        for (int i = index; i < elementNum - 1; i++) {
            Elements[i] = Elements[i + 1];
        }

        elementNum--;
    }

    virtual void InitializeElements() {
    }

    int width, height;

protected:
    MenuElement* Elements[64];
    int elementNum = 0;

    int HighlightedElement = -1;

    void Enter() override {
        for (int i = 0; i < elementNum; i++)
            Elements[i]->OnEnter();
    }

    void Exit() override {
        for (int i = 0; i < elementNum; i++)
            Elements[i]->OnExit();
    }

    void Initialize() override {
        width = d->width();
        height = d->height();

        InitializeElements();

        int first_id = GetFirstHighlightableElement();

        if (first_id != -1)
            Highlight(first_id);
    }

    void Deinitialize() override {
        for (int i = 0; i < elementNum; i++)
            delete Elements[i];

        elementNum = 0;
    }

    int GetFirstHighlightableElement() {
        for (int i = 0; i < elementNum; i++) {
            if (Elements[i]->isHighlightable())
                return i;
        }

        return -1;
    }

    int GetNextElement() {
        for (int i = HighlightedElement + 1; i < elementNum; i++) {
            if (Elements[i]->isHighlightable())
                return i;
        }
        return -1;
    }

    int GetPrevElement() {
        for (int i = HighlightedElement - 1; i >= 0; i--) {
            if (Elements[i]->isHighlightable())
                return i;
        }

        return -1;
    }

    void HighlightNextElement() {
        int next_id = GetNextElement();

        if (next_id != -1) {
            Unhighlight();
            Highlight(next_id);
        }
    }

    void HighlightPrevElement() {
        int prev_id = GetPrevElement();

        if (prev_id != -1)
            Highlight(prev_id);
    }

    void Unhighlight() {
        MenuElement* e = Elements[HighlightedElement];

        if (e->isSelectable()) {
            SelectableMenuElement* s = (SelectableMenuElement*)e;
            Deselect();
            s->OnUnhighlight();
            s->isHighlighted = false;
        }
        else if (e->isHighlightable()) {
            HighlightableMenuElement* h = (HighlightableMenuElement*)e;
            h->OnUnhighlight();
            h->isHighlighted = false;
        }

        HighlightedElement = -1;
    }

    void Deselect() {
        MenuElement* e = Elements[HighlightedElement];
        if (!e->isSelectable())
            return;

        SelectableMenuElement* s = (SelectableMenuElement*)e;

        if (s->isSelected) {
            s->OnDeselect();
            s->isSelected = false;
        }
    }

    void Highlight(int i) {
        MenuElement* e = Elements[i];

        if (HighlightedElement != i && HighlightedElement != -1)
            Unhighlight();

        if (e->isSelectable()) {
            SelectableMenuElement* s = (SelectableMenuElement*)e;
            s->isHighlighted = true;
            s->OnHighlight();

            HighlightedElement = i;
        }
        else if (e->isHighlightable()) {
            HighlightableMenuElement* h = (HighlightableMenuElement*)e;
            h->isHighlighted = true;
            h->OnHighlight();

            HighlightedElement = i;
        }
    }

    void Select(int i) {
        MenuElement* e = Elements[i];

        if (!e->isSelectable())
            return;

        SelectableMenuElement* s = (SelectableMenuElement*)e;

        if (!s->isHighlighted)
            Highlight(i);

        s->isSelected = true;
        s->OnSelect();
    }

    bool CanMoveHighlight() {
        if (HighlightedElement == -1)
            return false;

        MenuElement* e = Elements[HighlightedElement];

        if (!e->isSelectable())
            return true;

        SelectableMenuElement* s = (SelectableMenuElement*)e;

        if (s->isSelected)
            return false;

        return true;
    }

    void OnEncoderTickForward() {
        if (CanMoveHighlight())
            HighlightNextElement();
        else {
            if (HighlightedElement == -1)
                return;

            MenuElement* e = Elements[HighlightedElement];

            if (!e->isSelectable())
                return;

            SelectableMenuElement* s = (SelectableMenuElement*)e;

            if (s->isSelected)
                s->OnScroll(true);
        }
    }

    void OnEncoderTickBackward() {
        if (CanMoveHighlight())
            HighlightPrevElement();
        else {
            if (HighlightedElement == -1)
                return;

            MenuElement* e = Elements[HighlightedElement];

            if (!e->isSelectable())
                return;

            SelectableMenuElement* s = (SelectableMenuElement*)e;

            if (s->isSelected)
                s->OnScroll(false);
        }
    }

    void OnClick() {
        if (HighlightedElement == -1)
            return;

        MenuElement* e = Elements[HighlightedElement];

        if (e->isSelectable()) {
            SelectableMenuElement* s = (SelectableMenuElement*)e;

            if (!s->isSelected)
                Select(HighlightedElement);
            else
                Deselect();
        }
        else if (e->isHighlightable()) {
            HighlightableMenuElement* h = (HighlightableMenuElement*)e;
            h->OnClick();
        }
    }

    void OnButtonClicked(int b) {
        if (b == -1)
            OnClick();
        else
        {
            for (int i = 0; i < elementNum; i++) {
                if (Elements[i]->isPhysicalButton())
                {
                    PhysicalButtonInteractiveMenuElement* s = (PhysicalButtonInteractiveMenuElement*)Elements[i];

                    if (s->btn == b)
                        s->OnClick();
                }
            }
        }
    }

protected:
    bool bg = true;
};



class TextElement : public MenuElement {
public:
    TextElement(String text, int x, int y, uint8_t font, uint8_t textSize, uint16_t color) {
        t = text;
        x_p = x;
        y_p = y;

        f = font;
        tS = textSize;
        c = color;
    }

    void Draw() {
        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);

        DrawTextCentered(d, t, x_p, y_p - (d->fontHeight() / 2));

        //fGUI::DrawInstructions += "t_c" + String(t.length()) + t + String(x_p) + String(y_p - (d->fontHeight() / 2)) + String(f) + String(c, 16) + String(tS) + "|";
    }

    String t;
protected:
    int x_p, y_p;
    uint8_t f, tS;
    uint16_t c;
};

class UncenteredTextElement : public MenuElement {
public:
    UncenteredTextElement(String text, int x, int y, uint8_t font, uint8_t textSize, uint16_t color) {
        t = text;
        x_p = x;
        y_p = y;

        f = font;
        tS = textSize;
        c = color;
    }

    void Draw() {
        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);
        d->setCursor(x_p, y_p);
        d->print(t);

        //fGUI::DrawInstructions += "t" + String(t.length()) + t + String(x_p) + String(y_p) + String(f) + String(c, 16) + String(tS) + "|";
    }

    String t;
protected:
    int x_p, y_p;
    uint8_t f, tS;
    uint16_t c;
};

class BoxElement : public MenuElement {
public:
    BoxElement(int x, int y, int w, int h, uint32_t c) {
        x_p = x;
        y_p = y;
        wd = w;
        ht = h;

        cl = c;
    }

    void Draw() override {
        d->fillRect(x_p - wd / 2, y_p - ht / 2, wd, ht, cl);
    }

protected:
    int x_p, y_p, wd, ht;
    uint32_t cl;
};

class HighlightableTextElement : public HighlightableMenuElement {
public:
    HighlightableTextElement(String text, int x, int y, uint8_t font, uint8_t textSize, uint16_t color) {
        t = text;
        x_p = x;
        y_p = y;

        f = font;
        tS = textSize;
        c = color;
    }

    void Draw() {
        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);
        DrawTextCentered(d, t, x_p, y_p - (d->fontHeight() / 2));
    }

    void OnHighlight() override {
        prev_c = c;
        c = TFT_WHITE;
    }

    void OnUnhighlight() override {
        c = prev_c;
    }

protected:
    uint16_t prev_c;
    int x_p, y_p;
    uint8_t f, tS;
    uint16_t c;
    String t;
};

class Button : public HighlightableMenuElement {
public:
    Button(String text, int x, int y, int w, int h, uint8_t font, uint8_t textSize, uint16_t color, uint16_t bgcolor, void(*click)(), String tooltip_) {
        t = text;
        x_p = x;
        y_p = y;

        wd = w;
        ht = h;

        f = font;
        tS = textSize;
        c = color;
        bgc = bgcolor;

        clk = click;

        tooltip = tooltip_;
    }

    void Draw() override {
        if (isHighlighted)
            d->fillRect(x_p - (wd + 10) / 2, y_p - (ht + 10) / 2, (wd + 10), (ht + 10), TFT_WHITE);

        if (millis() - lastClickedMs > 250)
            d->fillRect(x_p - wd / 2, y_p - ht / 2, wd, ht, bgc);
        else
            d->fillRect(x_p - wd / 2, y_p - ht / 2, wd, ht, TFT_WHITE);

        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);
        DrawTextCentered(d, t, x_p, y_p - (d->fontHeight() / 2));
    }

    void OnClick() override {
        lastClickedMs = millis();

        clk();
    }
    String t;
    uint16_t c, bgc;

protected:
    int x_p, y_p, wd, ht;
    uint8_t f, tS;
    void(*clk)();

private:
    long lastClickedMs = 0;

    bool hlp = false, uhlp = false;
};

class PhysicalButton : public PhysicalButtonInteractiveMenuElement {
public:
    PhysicalButton(String text, int btn_id, int x, int y, int w, int h, uint8_t font, uint8_t textSize, uint16_t color, uint16_t bgcolor, void(*click)()) {
        t = text;
        x_p = x;
        y_p = y;

        wd = w;
        ht = h;

        f = font;
        tS = textSize;
        c = color;
        bgc = bgcolor;

        clk = click;

        btn = btn_id;
    }

    void Draw() override {
        if (millis() - lastClickedMs > 250)
            d->fillRect(x_p - wd / 2, y_p - ht / 2, wd, ht, bgc);
        else
            d->fillRect(x_p - wd / 2, y_p - ht / 2, wd, ht, TFT_WHITE);

        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);
        DrawTextCentered(d, t, x_p, y_p - (d->fontHeight() / 2));
    }

    void OnClick() override {
        lastClickedMs = millis();

        clk();
    }

protected:
    int x_p, y_p, wd, ht;
    uint8_t f, tS;
    uint16_t c, bgc;
    String t;
    void(*clk)();

private:
    long lastClickedMs;
};

class NumberInputElement : public SelectableMenuElement {
public:
    NumberInputElement(String text, int x, int y, uint8_t font, uint8_t textSize, uint16_t color, uint16_t bgcolor) {
        t = text;
        x_p = x;
        y_p = y;

        f = font;
        tS = textSize;
        c = color;
        bgc = bgcolor;
    }

    void Draw() {
        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);
        int w = d->textWidth(t + ": " + String(Value));
        int w_off = 0;// d->textWidth(t + ": ");
        int h = d->fontHeight();

        if (isHighlighted)
            d->fillRect(x_p - w_off / 2 - 4, y_p - h / 2 - 4, w + 8, h + 8, TFT_WHITE);

        if (isSelected)
            d->drawRect(x_p - w_off / 2 - 3, y_p - h / 2 - 3, w + 6, h + 6, TFT_BLACK);

        d->fillRect(x_p - w_off / 2 - 2, y_p - h / 2 - 2, w + 4, h + 4, bgc);
        d->setCursor(x_p - w_off / 2, y_p - h / 2);
        d->print(t + ": " + String(Value) + postfix);
    }

    void OnScroll(bool dir) override {
        Value += dir ? 1 : -1;
    }

    int Value = 0;
    String postfix = "";

protected:
    int x_p, y_p;
    uint8_t f, tS;
    uint16_t c, bgc;
    String t;
};

class FloatInputElement : public SelectableMenuElement {
public:
    FloatInputElement(String text, int x, int y, uint8_t font, uint8_t textSize, uint16_t color, uint16_t bgcolor) {
        t = text;
        x_p = x;
        y_p = y;

        f = font;
        tS = textSize;
        c = color;
        bgc = bgcolor;
    }

    void Draw() {
        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);
        int w = d->textWidth(t + ": " + String(Value));
        int w_off = 0;// d->textWidth(t + ": ");
        int h = d->fontHeight();

        if (isHighlighted)
            d->fillRect(x_p - w_off / 2 - 4, y_p - h / 2 - 4, w + 8, h + 8, TFT_WHITE);

        if (isSelected)
            d->drawRect(x_p - w_off / 2 - 3, y_p - h / 2 - 3, w + 6, h + 6, TFT_BLACK);

        d->fillRect(x_p - w_off / 2 - 2, y_p - h / 2 - 2, w + 4, h + 4, bgc);
        d->setCursor(x_p - w_off / 2, y_p - h / 2);
        d->print(t + ": " + String(Value));
    }

    void OnScroll(bool dir) override {
        Value += (dir ? 1 : -1) * step;
    }

    float Value = 0;
    float step = 0.1;

protected:
    int x_p, y_p;
    uint8_t f, tS;
    uint16_t c, bgc;
    String t;
};

class TextChoiceElement : public SelectableMenuElement {
public:
    TextChoiceElement(String texts[], int texts_num, int x, int y, uint8_t font, uint8_t textSize, uint16_t color, uint16_t bgcolor) {
        t = texts[0];
        ts = texts;
        numT = texts_num;

        x_p = x;
        y_p = y;

        f = font;
        tS = textSize;
        c = color;
        bgc = bgcolor;
    }

    void Draw() {
        d->setTextFont(f);
        d->setTextColor(c);
        d->setTextSize(tS);
        int w = d->textWidth(t + ": " + ts[Value]);
        int w_off = 0;// d->textWidth(t + ": ");
        int h = d->fontHeight();

        if (isHighlighted)
            d->fillRect(x_p - w_off / 2 - 4, y_p - h / 2 - 4, w + 8, h + 8, TFT_WHITE);

        if (isSelected)
            d->drawRect(x_p - w_off / 2 - 3, y_p - h / 2 - 3, w + 6, h + 6, TFT_BLACK);

        d->fillRect(x_p - w_off / 2 - 2, y_p - h / 2 - 2, w + 4, h + 4, bgc);
        d->setCursor(x_p - w_off / 2, y_p - h / 2);
        d->print(t + ": " + ts[Value]);
    }

    void UpdateTextNum(int n) {
        numT = n;
    }

    void OnScroll(bool dir) override {
        Value += dir ? 1 : -1;

        Value = min(numT - 1, max(Value, 1));
    }

    int Value = 0;

protected:
    int x_p, y_p;
    uint8_t f, tS;
    uint16_t c, bgc;
    String t;
    String* ts;
    int numT;
};

class TooltipDisplay : public MenuElement {
public:
    TooltipDisplay(int x, int y, uint8_t font, uint8_t textSize, uint16_t color, uint16_t bgcolor) {
        x_p = x;
        y_p = y;

        f = font;
        tS = textSize;
        c = color;
        bgc = bgcolor;
    }

    void Draw() {
        HighlightableMenuElement* e = m->GetHighlightedElement();
        String text = "";

        if (e != nullptr)
            text = e->tooltip;

        d->setTextFont(f);
        d->setTextColor(c, bgc, true);
        d->setTextSize(tS);

        d->fillRect(0, y_p - (d->fontHeight() / 2), d->width(), d->fontHeight(), bgc);
        DrawTextCentered(d, text, x_p, y_p - (d->fontHeight() / 2));
    }

protected:
    int x_p, y_p;
    uint8_t f, tS;
    uint16_t c, bgc;
};

class ListMenu;

class ListElement {
public:
    virtual int Draw(int x, int y) {
        return 0;
    }

    virtual void OnClick() {

    }

    virtual void OnSelect() {

    }

    virtual void OnDeselect() {

    }

    virtual void OnScroll(bool dir) {

    }

    virtual void OnHighlight() {

    }
    virtual void OnUnhighlight() {

    }

    bool isHighlighted = false;
    bool isSelected = false;
    bool isSelectable = false;

    TFT_eSprite* d = nullptr;
    ListMenu* m = nullptr;
};

class ListMenu : public Menu {
public:
    void Draw() override {
        if (bg)
            d->fillScreen(bg_color);

        DrawElements();
        DrawScrollbar();
    }

    uint16_t bg_color = TFT_BLACK;
    bool bg = true;
protected:
    void AddElement(ListElement* e) {
        e->d = d;
        e->m = this;
        elements[numElements] = e;
        numElements++;
    }

    void RemoveElement(int index) {
        delete elements[index];

        for (int i = index; i < numElements - 1; i++) {
            elements[i] = elements[i + 1];
        }

        numElements--;
    }

    virtual void InitializeElements() {

    }

protected:
    void OnButtonClicked(int b) override {
        if (b == -1)
            OnEncoderClick();
    }

    void OnEncoderTickBackward() override {
        OnScroll(false);
    }

    void OnEncoderTickForward() override {
        OnScroll(true);
    }

    void Initialize() override {
        InitializeElements();

        if (numElements > 0) {
            elements[0]->isHighlighted = true;
            highlightedElementIndex = 0;
        }

    }

    void Enter() override {
        if (numElements > 0)
            elements[0]->isHighlighted = true;

        highlightedElementIndex = 0;
    }

    void DrawScrollbar() {
        double scrollbarSize = max(min((double)(d->height() - start_y) / (double)(menuSize + 16), 1.0), 0.1);
        double scrollbarStart = (double)scroll / (double)(menuSize + 16);

        d->fillRect(0, start_y, 12, d->height() - start_y - 16, TFT_LIGHTGREY);
        d->fillRect(0, scrollbarStart * (d->height() - start_y - 16) + start_y, 12, scrollbarSize * (d->height() - start_y - 16), TFT_DARKGREY);
    }

    void DrawElements() {
        int currentY = (-scroll) + start_y;
        menuSize = 0;

        for (int i = 0; i < numElements; i++) {
            ListElement* e = elements[i];

            if (e->isHighlighted && currentY < start_y)
                scroll = (scroll + currentY) - start_y;

            int h = e->Draw(start_x, currentY);

            currentY += h;
            menuSize += h;

            if (e->isHighlighted && currentY > d->height() - 16)
                scroll += currentY - (d->height() - 16);
        }

        if (menuSize < d->height() - start_y - 16 && scroll != 0)
            scroll = 0;
    }

    void OnScroll(bool dir) {
        if (numElements <= highlightedElementIndex)
            return;

        if (elements[highlightedElementIndex]->isSelected)
        {
            elements[highlightedElementIndex]->OnScroll(dir);
            return;
        }
        elements[highlightedElementIndex]->OnUnhighlight();
        elements[highlightedElementIndex]->isHighlighted = false;

        highlightedElementIndex = min(numElements - 1, max(0, highlightedElementIndex + (dir ? 1 : -1)));

        elements[highlightedElementIndex]->OnHighlight();
        elements[highlightedElementIndex]->isHighlighted = true;
    }

    void OnEncoderClick() {
        if (numElements <= highlightedElementIndex)
            return;

        if (elements[highlightedElementIndex]->isSelectable) {
            if (elements[highlightedElementIndex]->isSelected) {
                elements[highlightedElementIndex]->isSelected = false;
                elements[highlightedElementIndex]->OnDeselect();
            }
            else {
                elements[highlightedElementIndex]->isSelected = true;
                elements[highlightedElementIndex]->OnSelect();
            }
        }

        elements[highlightedElementIndex]->OnClick();
    }

protected:
    ListElement* elements[64];
    int numElements = 0;

    int start_y = 24;
    int start_x = 4;

    int highlightedElementIndex = 0;
private:
    int scroll = 0;
    int menuSize;
};

class ListTextElement : public ListElement {
public:
    ListTextElement(String text, uint8_t font, uint8_t textSize, uint16_t color, uint16_t bgcolor) {
        t = text;
        f = font;
        tS = textSize;
        c = color;
        bgc = bgcolor;
    }

    int Draw(int x, int y) override {
        d->setTextFont(f);

        if (isHighlighted)
            d->setTextColor(c, TFT_DARKGREY, true);
        else
            d->setTextColor(c, bgc, true);

        d->setTextSize(tS);
        //DrawTextCentered(d, t, x, y - (d->fontHeight() / 2));
        d->setCursor(x, y);

        d->print(t);

        return d->fontHeight() + 2;
    }

protected:
    uint8_t f, tS;
    uint16_t c, bgc;
    String t;
};
#endif
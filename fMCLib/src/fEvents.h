#ifndef fEvents_h
#define fEvents_h

#include "Arduino.h"

class EventHandler_d {
public:
    virtual void Handle(void* args) {
        Serial.println("Error");
    }
};

template<class T>
class EventHandler : public EventHandler_d {
public:
    EventHandler(T* t_, void (*r)(T*, void*)) {
        t = t_;
        run = r;
    }


    void Handle(void* args) override {
        run(t, args);
    }

private:
    T* t;
    void (*run)(T*, void*);
};

class Event {
public:
    Event() {

    }

    virtual void AddHandler(EventHandler_d* handler) {
        handlers[numHandlers++] = handler;
    }

    virtual void Invoke(void* args) {
        for (int i = 0; i < numHandlers; i++)
            handlers[i]->Handle(args);
    }

    virtual void Invoke() {
        Invoke(nullptr);
    }

protected:
    EventHandler_d* handlers[128];
    int numHandlers;
};

#endif
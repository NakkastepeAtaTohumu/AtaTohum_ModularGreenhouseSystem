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

class EventHandlerFunc : public EventHandler_d {
public:
    EventHandlerFunc(void (*r)(void*)) {
        run = r;
        //xTaskCreate(task, "handler", 2048, NULL, 0, NULL);
    }


    void Handle(void* args) override {
        //current_param = args;
        //vTaskResume(handle);
        run(args);
    }
    /*
    void task(void* param) {
        while (true) {
            vTaskSuspend(NULL);
            run(current_param);
        }
    }*/

private:
    void (*run)(void*);
    //TaskHandle_t handle;
    //void* current_param;
};

class Event {
public:
    Event() {

    }

    virtual void AddHandler(EventHandler_d* handler) {
        handlers[numHandlers++] = handler;
    }

    virtual void Invoke(void* args) {
        for (int i = 0; i < numHandlers; i++) {
            delay(0);
            handlers[i]->Handle(args);
        }
    }

    virtual void Invoke() {
        Invoke(nullptr);
    }

protected:
    EventHandler_d* handlers[128];
    int numHandlers = 0;
};

#endif
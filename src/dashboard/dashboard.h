
#pragma once


namespace AT {

    class event;

    class dashboard {
    public:

        dashboard();
        ~dashboard();

        bool init();
        void update(f32 delta_time);
        void draw(f32 delta_time);
        bool shutdown();

        void on_event(event& event);

    private:


    };
}

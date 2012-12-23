#include "util/init.h"
#include "util/events.h"
#include "util/input/input-manager.h"
#include "util/debug.h"
#include "util/exceptions/exception.h"
#include "util/exceptions/shutdown_exception.h"

class Main: public Util::Logic, public Util::Draw {
public:
    enum Input{
        Quit
    };

    Main():
    quit(false),
    handler(*this){
        map.set(Keyboard::Key_ESC, Quit);
    }

    void draw(const Graphics::Bitmap & screen){
    }

    void run(){
        InputManager::handleEvents(map, InputSource(0, 0), handler);
    }

    bool done(){
        return quit;
    }

    double ticks(double time){
        return time;
    }

    class Handler: public InputHandler<Input> {
    public:
        Handler(Main & main):
        main(main){
        }

        void press(const Input & out, Keyboard::unicode_t unicode){
            switch (out){
                case Quit: {
                    main.quit = true;
                    break;
                }
            }
        }

        void release(const Input & out, Keyboard::unicode_t unicode){
        }

        Main & main;
    };

    bool quit;
    Handler handler;
    InputMap<Input> map;
};

static void run(){
        Main main;
    Util::standardLoop(main, main);
}

int main(int argc, char ** argv){
    Global::init(Global::WINDOWED);
    InputManager input;
    try{
        run();
    } catch (const ShutdownException & fail){
        Global::debug(0) << "Shutdown" << std::endl;
    }

    Global::close();
    Global::debug(0) << "Bye!" << std::endl;
}

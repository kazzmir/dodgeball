#include "util/graphics/bitmap.h"
#include "util/init.h"
#include "util/events.h"
#include "util/pointer.h"
#include "util/font.h"
#include "util/input/input-manager.h"
#include "util/debug.h"
#include "util/exceptions/exception.h"
#include "util/exceptions/shutdown_exception.h"

#include "world.h"

/* Unit is a foot or something */

static double unitsToPixels(double units){
    return units * 10;
}

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
        screen.clear();
        world.draw(screen);
        screen.BlitToScreen();
    }

    void run(){
        InputManager::handleEvents(map, InputSource(0, 0), handler);
        world.run();
    }

    bool done(){
        return quit || world.isDone();
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
    Dodgeball::World world;
};

static void run(){
    Keyboard::pushRepeatState(false);
    Main main;
    Util::standardLoop(main, main);
    Keyboard::popRepeatState();
}

int main(int argc, char ** argv){
    Global::init(Global::WINDOWED);
    Util::Parameter<Graphics::Bitmap*> use(Graphics::screenParameter, Graphics::getScreenBuffer());
    Util::Parameter<Util::ReferenceCount<Path::RelativePath> > font(Font::defaultFont, Util::ReferenceCount<Path::RelativePath>(new Path::RelativePath("arial.ttf")));
    InputManager input;
    try{
        run();
    } catch (const ShutdownException & fail){
        Global::debug(0) << "Shutdown" << std::endl;
    }

    Dodgeball::SoundManager::destroy();
    Dodgeball::AnimationManager::destroy();
    Global::close();
    Global::debug(0) << "Bye!" << std::endl;
}

#include "util/init.h"
#include "util/events.h"
#include "util/input/input-manager.h"
#include "util/debug.h"
#include "util/exceptions/exception.h"
#include "util/exceptions/shutdown_exception.h"

static void run(){
    class Main: public Util::Logic, public Util::Draw {
    public:
        void draw(const Graphics::Bitmap & screen){
        }

        void run(){
        }

        bool done(){
            return false;
        }

        double ticks(double time){
            return time;
        }
    };

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
}

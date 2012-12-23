#include "util/graphics/bitmap.h"
#include "util/init.h"
#include "util/events.h"
#include "util/input/input-manager.h"
#include "util/debug.h"
#include "util/exceptions/exception.h"
#include "util/exceptions/shutdown_exception.h"

class Camera{
public:

    Camera():
    x(0),
    y(0){
    }

    void moveTo(double x, double y){
        this->x = x;
        this->y = y;
    }

    void moveRight(int much){
        this->x += much;
    }
    
    void moveLeft(int much){
        this->x -= much;
    }

    double getX() const {
        return x;
    }

    double getY() const {
        return y;
    }

protected:

    double x, y;
};

class World{
public:
    static const int GFX_X = 640;
    static const int GFX_Y = 480;

    World():
    fieldWidth(1000){
        camera.moveTo(300, 0);
    }

    void moveLeft(){
        camera.moveLeft(5);
    }

    void moveRight(){
        camera.moveRight(5);
    }

    void drawMiddle(const Graphics::Bitmap & work){
        int where = fieldWidth / 2;
        int position = where - camera.getX();
        work.vLine(0, position, GFX_Y, Graphics::makeColor(255, 255, 255));
    }

    void draw(const Graphics::Bitmap & screen){
        Graphics::StretchedBitmap work(GFX_X, GFX_Y, screen);
        work.start();

        drawMiddle(work);

        work.finish();
    }

    Camera camera;
    int fieldWidth;
};

class Main: public Util::Logic, public Util::Draw {
public:
    enum Input{
        Left,
        Right,
        Quit
    };

    Main():
    quit(false),
    handler(*this){
        map.set(Keyboard::Key_ESC, Quit);
        map.set(Keyboard::Key_LEFT, Left);
        map.set(Keyboard::Key_RIGHT, Right);
    }

    void draw(const Graphics::Bitmap & screen){
        screen.clear();
        world.draw(screen);
        screen.BlitToScreen();
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
                case Left: {
                    main.world.moveLeft();
                    break;
                }
                case Right: {
                    main.world.moveRight();
                    break;
                }
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
    World world;
};

static void run(){
    Keyboard::pushRepeatState(true);
    Main main;
    Util::standardLoop(main, main);
    Keyboard::popRepeatState();
}

int main(int argc, char ** argv){
    Global::init(Global::WINDOWED);
    Util::Parameter<Graphics::Bitmap*> use(Graphics::screenParameter, Graphics::getScreenBuffer());
    InputManager input;
    try{
        run();
    } catch (const ShutdownException & fail){
        Global::debug(0) << "Shutdown" << std::endl;
    }

    Global::close();
    Global::debug(0) << "Bye!" << std::endl;
}

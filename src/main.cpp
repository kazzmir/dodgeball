#include "util/graphics/bitmap.h"
#include "util/init.h"
#include "util/events.h"
#include "util/pointer.h"
#include "util/input/input-manager.h"
#include "util/debug.h"
#include "util/exceptions/exception.h"
#include "util/exceptions/shutdown_exception.h"

#include <vector>

using std::vector;

class Camera{
public:
    Camera():
    x(0),
    y(0),
    zoom(1){
    }

    static const int width = (int)(320 * 1.2);
    static const int height = (int)(240 * 1.2);

    void zoomIn(double amount){
        zoom += amount;
    }

    void zoomOut(double amount){
        zoom -= amount;
        if (zoom < 0.1){
            zoom = 0.1;
        }
    }

    void normalZoom(){
        zoom = 1;
    }

    int getWidth() const {
        return 2 * width / getZoom();
    }

    int getHeight() const {
        return 2 * height / getZoom();
    }

    double getX1() const {
        double x1 = getX() - width / getZoom();
        if (x1 < 0){
            return 0;
        }
        return x1;
    }
    
    double getX2() const {
        double x2 = getX() + width / getZoom();
        if (x2 > 1000){
            return 1000 - getWidth();
        }
        return x2;
    }
    
    double getY1() const {
        double y1 = getY() - height / getZoom();
        if (y1 < 0){
            return 0;
        }
        return y1;
    }
    
    double getY2() const {
        double y2 = getY() + height / getZoom();
        if (y2 > 1000){
            return y2 - getHeight();
        }
        return y2;
    }

    double computeX(double x) const {
        return x - getX1();
    }

    double computeY(double y) const {
        return y - getY1();
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
    
    void moveUp(int much){
        this->y -= much;
    }
    
    void moveDown(int much){
        this->y += much;
    }

    double getX() const {
        return x;
    }

    double getY() const {
        return y;
    }

    double getZoom() const {
        return zoom;
    }

protected:

    double x, y;
    double zoom;
};

class Player{
public:
    Player(double x, double y):
    x(x),
    y(y){
    }

    void draw(const Graphics::Bitmap & work, const Camera & camera){
        work.circleFill((int) camera.computeX(x), (int) camera.computeY(y - 10), 10, Graphics::makeColor(0, 0, 255));
    }

    double getX() const {
        return x;
    }
    
    double getY() const {
        return y;
    }

protected:
    double x;
    double y;
};

class Team{
public:
    Team(){
        players.push_back(Util::ReferenceCount<Player>(new Player(100, 100)));
    }

    void draw(const Graphics::Bitmap & work, const Camera & camera){
        for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
            const Util::ReferenceCount<Player> & player = *it;
            player->draw(work, camera);
        }
    }

protected:

    vector<Util::ReferenceCount<Player> > players;
};

class Field{
public:
    Field(int width, int height):
    width(width),
    height(height){
    }

    int getWidth() const {
        return width;
    }

    int getHeight() const {
        return height;
    }

    void draw(const Graphics::Bitmap & work, const Camera & camera){
        int margin = 10;
        int x1 = 0 + margin;
        int x2 = width - margin;
        int y1 = 0 + margin;
        int y2 = height - margin;

        int middle = (x1 + x2) / 2;

        Graphics::Color white = Graphics::makeColor(255, 255, 255);

        /* center line */
        work.rectangleFill(camera.computeX(middle - 5), camera.computeY(y1),
                           camera.computeX(middle + 5), camera.computeY(y2),
                           white);

        /* left side */
        work.rectangleFill(camera.computeX(x1), camera.computeY(y1),
                           camera.computeX(x1 + 10), camera.computeY(y2),
                           white);

        /* right side */
        work.rectangleFill(camera.computeX(x2), camera.computeY(y1),
                           camera.computeX(x2 - 10), camera.computeY(y2),
                           white);

        /* top side */
        work.rectangleFill(camera.computeX(x1), camera.computeY(y1),
                           camera.computeX(x2), camera.computeY(y1 + 10),
                           white);

        /* bottom side */
        work.rectangleFill(camera.computeX(x1), camera.computeY(y2),
                           camera.computeX(x2), camera.computeY(y2 - 10),
                           white);
    }

    const int width;
    const int height;
};

class World{
public:
    static const int GFX_X = 640;
    static const int GFX_Y = 480;

    enum Input{
        Left,
        Right,
        Up,
        Down,
        ZoomIn,
        ZoomOut
    };

    World():
    field(1200, 600),
    handler(*this){
        camera.moveTo(field.getWidth() / 2, field.getHeight() / 2);
        map.set(Keyboard::Key_LEFT, Left);
        map.set(Keyboard::Key_RIGHT, Right);
        map.set(Keyboard::Key_UP, Up);
        map.set(Keyboard::Key_DOWN, Down);
        map.set(Keyboard::Key_EQUALS, ZoomIn);
        map.set(Keyboard::Key_MINUS, ZoomOut);
    }

    void run(){
        InputManager::handleEvents(map, InputSource(0, 0), handler);
    }

    class Handler: public InputHandler<Input> {
    public:
        Handler(World & world):
        world(world){
        }

        void press(const Input & out, Keyboard::unicode_t unicode){
            switch (out){
                case Left: {
                    world.moveLeft();
                    break;
                }
                case Right: {
                    world.moveRight();
                    break;
                }
                case Up: {
                    world.moveUp();
                    break;
                }
                case Down: {
                    world.moveDown();
                    break;
                }
                case ZoomIn: {
                    world.camera.zoomIn(0.02);
                    break;
                }
                case ZoomOut: {
                    world.camera.zoomOut(0.02);
                    break;
                }
            }
        }

        void release(const Input & out, Keyboard::unicode_t unicode){
        }

        World & world;
    };

    void moveLeft(){
        camera.moveLeft(5);
    }

    void moveRight(){
        camera.moveRight(5);
    }
    
    void moveUp(){
        camera.moveUp(5);
    }
    
    void moveDown(){
        camera.moveDown(5);
    }

    void drawPlayers(const Graphics::Bitmap & work){
        team1.draw(work, camera);
        team2.draw(work, camera);
    }

    void draw(const Graphics::Bitmap & screen){
        Graphics::StretchedBitmap work(camera.getWidth(), camera.getHeight(), screen);
        work.start();

        field.draw(work, camera);
        drawPlayers(work);

        work.finish();
    }

    Camera camera;
    Field field;
    Team team1;
    Team team2;
    Handler handler;
    InputMap<Input> map;
};

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

#ifndef _dodgeball_world_h
#define _dodgeball_world_h

#include <vector>
#include "util/input/input-map.h"
#include "util/pointer.h"

namespace Graphics{
    class Bitmap;
}

namespace Dodgeball{

class World;
class Ball;

class Camera{
public:
    Camera();

    static const int width = (int)(320 * 1.2);
    static const int height = (int)(240 * 1.2);

    void zoomIn(double amount);
    void zoomOut(double amount);
    void normalZoom();

    int getWidth() const;
    int getHeight() const;

    double getX1() const;
    double getX2() const;
    double getY1() const;
    double getY2() const;

    double computeX(double x) const;
    double computeY(double y) const;

    void moveTowards(double x, double y);
    void moveTo(double x, double y);

    void moveRight(int much);
    void moveLeft(int much);
    void moveUp(int much);
    void moveDown(int much);

    double getX() const;
    double getY() const;

    double getZoom() const;

protected:

    double x, y;
    double zoom;
};

class Field{
public:
    Field(int width, int height);

    int getWidth() const;
    int getHeight() const;

    double getFriction() const;

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    const int width;
    const int height;
};

class Player{
public:
    enum Facing{
        FaceLeft,
        FaceRight,
        FaceUp,
        FaceDown,
        FaceUpLeft,
        FaceUpRight,
        FaceDownLeft,
        FaceDownRight
    };

    enum Input{
        Left,
        Right,
        Up,
        Down,
        Action
    };

    Player(double x, double y);

    struct Hold{
        Hold():
            left(false), right(false),
            up(false), down(false){
            }

        bool left;
        bool right;
        bool up;
        bool down;
    } hold;

    void act(World & world);
    void setControl(bool what);

    void doInput(World & world);

    void doAction(World & world);
    void throwBall(Ball & ball);

    void moveLeft(double speed);
    void moveRight(double speed);
    void moveUp(double speed);
    void moveDown(double speed);

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    double getX() const;
    double getY() const;

protected:
    double x;
    double y;
    /* true if the human player is controlling this guy */
    bool control;
    bool hasBall;
    Facing facing;
    InputMap<Input> map;
};

class Team{
public:
    enum Side{
        LeftSide,
        RightSide
    };

    Team(Side side, const Field & field);

    void enableControl();

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    void act(World & world);

protected:
    void populateLeft(const Field & field);
    void populateRight(const Field & field);

    std::vector<Util::ReferenceCount<Player> > players;
    Side side;
};

class Ball{
public:
    Ball(double x, double y);

    double getX() const;
    double getY() const;
    double getZ() const;

    void grab(Player * holder);
    void ungrab();

    void act(const Field & field);

    void doThrow(double velocityX, double velocityY, double velocityZ);

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    double x;
    double y;
    /* z will be the in-air coordinate */
    double z;
    double angle;

    double velocityX;
    double velocityY;
    double velocityZ;

    /* true if being grabbed */
    bool grabbed;
    bool moving;
    Player * holder;
};

class World{
public:
    enum Input{
        Left,
        Right,
        Up,
        Down,
        ZoomIn,
        ZoomOut
    };

    World();

    void run();

    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    void drawPlayers(const Graphics::Bitmap & work);
    void draw(const Graphics::Bitmap & screen);

    Ball & getBall();

    Camera camera;
    Field field;
    Ball ball;
    Team team1;
    Team team2;
    InputMap<Input> map;
};

}

#endif

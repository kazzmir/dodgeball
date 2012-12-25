#ifndef _dodgeball_world_h
#define _dodgeball_world_h

#include <vector>
#include "util/input/input-map.h"
#include "util/graphics/color.h"
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

/* Used to limit a players movement to the given rectangle */
struct Box{
    Box(int x1, int y1, int x2, int y2):
    x1(x1), y1(y1),
    x2(x2), y2(y2){
    }

    int x1;
    int y1;
    int x2;
    int y2;
};

struct Hold{
    enum Last{
        Pressed,
        Release
    };

    Hold():
    count(0),
    time(0),
    last(Release){
    }

    unsigned int count;
    unsigned int time;
    Last last;

    bool isPressed() const;
    unsigned int getCount() const;

    void act();
    void press();
    void release();
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
        Jump,
        Action
    };

    Player(double x, double y, const Graphics::Color & color, const Box & box);
    Hold hold;

    void act(World & world);
    void setControl(bool what);
    bool hasControl() const;

    void doInput(World & world);

    void doAction(World & world);
    void throwBall(World & world, Ball & ball);
    
    Box collisionBox() const;

    void moveLeft(double speed);
    void moveRight(double speed);
    void moveUp(double speed);
    void moveDown(double speed);
    void runRight(double speed);
    void runLeft(double speed);

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    int getFacingAngle() const;

    /* Height of where the hands are */
    double getHandPosition() const;
    double getX() const;
    double getY() const;
    void setX(double x);
    void setY(double y);

protected:
    void doJump();

    double x;
    double y;
    double z;

    double velocityX;
    double velocityY;
    double velocityZ;

    static const double jumpVelocity = 15;
    static double maxRunSpeed;

    bool runningLeft;
    bool runningRight;

    /* true if the human player is controlling this guy */
    bool control;
    bool hasBall;
    Facing facing;
    InputMap<Input> map;
    Box limit;
    Graphics::Color color;

    Hold left;
    Hold right;
    Hold up;
    Hold down;
};

class Team{
public:
    enum Side{
        LeftSide,
        RightSide
    };

    enum Input{
        Cycle
    };

    Team(Side side, const Field & field);

    void enableControl();
    void cycleControl();

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    void act(World & world);
    const std::vector<Util::ReferenceCount<Player> > & getPlayers() const;

protected:
    void populateLeft(const Field & field);
    void populateRight(const Field & field);

    std::vector<Util::ReferenceCount<Player> > players;
    Side side;
    InputMap<Input> map;
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

    Box collisionBox() const;

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

    int timeInAir;

    /* true if being grabbed */
    bool grabbed;
    bool thrown;
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
    
    Util::ReferenceCount<Player> getTarget(Player & who);

    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    void drawPlayers(const Graphics::Bitmap & work);
    void draw(const Graphics::Bitmap & screen);

    unsigned int getTime() const;

    bool onTeam(const Team & team, Player & who);

    const Field & getField() const;
    Ball & getBall();

    Camera camera;
    Field field;
    Ball ball;
    Team team1;
    Team team2;
    InputMap<Input> map;
    unsigned int time;
};

}

#endif

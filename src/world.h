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
class Player;

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

    void setX(double x);
    void setY(double y);

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

class Behavior{
public:
    Behavior();
    virtual ~Behavior();
    virtual void act(World & world, Player & player) = 0;
    virtual void setControl(bool what) = 0;
    virtual bool hasControl() const = 0;
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

    Player(double x, double y, const Graphics::Color & color, const Box & box, const Util::ReferenceCount<Behavior> & behavior, bool sideline);

    void act(World & world);
    void setControl(bool what);
    bool hasControl() const;

    void doAction(World & world);
    void doCatch(int time = 30);
    void doPass(World & world);
    
    void collided(Ball & ball);

    bool onGround() const;
    
    Box collisionBox() const;

    double walkingSpeed() const;

    void moveLeft(double speed);
    void moveRight(double speed);
    void moveUp(double speed);
    void moveDown(double speed);
    void runRight(double speed);
    void runLeft(double speed);

    void grabBall(Ball & ball);

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    int getFacingAngle() const;

    bool hasBall() const;

    bool isCatching() const;

    /* upper left x/y for collision detection */
    double getX1() const;
    double getY1() const;

    double getWidth() const;
    double getHeight() const;
    
    bool onSideline() const;

    /* Height of where the hands are */
    double getHandPosition() const;
    double getX() const;
    double getY() const;
    double getZ() const;
    void setX(double x);
    void setY(double y);

    void setVelocityX(double x);
    void setVelocityY(double y);

    Box getLimit() const;

    void setFacing(Facing face);
    void doJump();

protected:
    void throwBall(World & world, Ball & ball);

    double x;
    double y;
    double z;

    double velocityX;
    double velocityY;
    double velocityZ;

    static const double jumpVelocity = 15;
    static double maxRunSpeed;

    bool hasBall_;
    Facing facing;
    Box limit;
    Graphics::Color color;
    /* true if on the sideline */
    bool sideline;
    /* positive if catching */
    int catching;

    Util::ReferenceCount<Behavior> behavior;
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
    void cycleControl(World & world);
    void giveControl(const Util::ReferenceCount<Player> & who);

    Side getSide() const;
            
    void collisionDetection(Ball & ball);

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

    double getX1() const;
    double getY1() const;

    double getVelocityX() const;
    double getVelocityY() const;

    void collided(Player & player);

    void grab(Player * holder);
    void ungrab();

    void act(const Field & field);

    bool isThrown() const;

    /* true if the ball is thrown (either power or pass) and hasn't touched the ground */
    bool inAir() const;

    Box collisionBox() const;

    void doThrow(World & world, Player & player, double velocityX, double velocityY, double velocityZ);
    void doPass(World & world, Player & player, double velocityX, double velocityY, double velocityZ);

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
    bool air;
    Player * holder;
    Team::Side thrownBy;
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
    Util::ReferenceCount<Player> getTarget(const std::vector<Util::ReferenceCount<Player> > & players, Player & who);

    Util::ReferenceCount<Player> passTarget(const std::vector<Util::ReferenceCount<Player> > & players, Player & who);
    Util::ReferenceCount<Player> passTarget(Player & who);

    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    void drawPlayers(const Graphics::Bitmap & work);
    void draw(const Graphics::Bitmap & screen);

    void collisionDetection();
    
    void giveControl(const Util::ReferenceCount<Player> & enemy);

    Team::Side findTeam(const Player & player);

    unsigned int getTime() const;

    bool onTeam(const Team & team, const Player & who);

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

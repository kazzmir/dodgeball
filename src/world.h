#ifndef _dodgeball_world_h
#define _dodgeball_world_h

#include <vector>
#include <map>
#include "util/input/input-map.h"
#include "util/graphics/color.h"
#include "util/pointer.h"
#include "util/file-system.h"
#include "util/sound/sound.h"

class Token;

namespace Graphics{
    class Bitmap;
}

namespace Dodgeball{

class World;
class Ball;
class Player;

class Animation;
class AnimationEvent{
public:
    AnimationEvent();
    virtual void invoke(Animation & animation) = 0;
    virtual ~AnimationEvent();
};

class Animation{
public:
    Animation(const Filesystem::AbsolutePath & directory, const Token * token, unsigned int id);
    Animation(const Animation & copy);
    Animation & operator=(const Animation & copy);
    virtual ~Animation();

    bool operator==(const Animation & who) const;
    bool operator!=(const Animation & who) const;

    bool isDone() const;

    void setBaseDirectory(const Filesystem::AbsolutePath & path);
    const Filesystem::AbsolutePath & getBaseDirectory() const;

    void draw(const Graphics::Bitmap & work, int x, int y, bool faceRight);

    void setOffset(int x, int y);
    void setFrame(const Graphics::Bitmap & bitmap);
    void setDelay(int delay);
    void setLoop(bool what);

    void act();

    Util::ReferenceCount<Animation> clone();

protected:
    void copy(const Animation & animation);

    std::vector<Util::ReferenceCount<AnimationEvent> > events;
    std::vector<Util::ReferenceCount<AnimationEvent> >::iterator current;
    Filesystem::AbsolutePath baseDirectory;

    int x, y;
    Graphics::Bitmap frame;
    int delay;
    int counter;
    bool loop;
    /* globally unique id keeps track of animations for equality */
    unsigned int id;
};

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

class Behavior{
public:
    Behavior();
    virtual ~Behavior();
    virtual void act(World & world, Player & player) = 0;
    virtual void setControl(bool what) = 0;
    virtual bool hasControl() const = 0;
    virtual void resetInput() = 0;
    virtual void gotBall(Ball & ball) = 0;
};

class Drawable{
public:
    Drawable();
    virtual ~Drawable();

    virtual double getX() const = 0;
    virtual double getY() const = 0;
    virtual void draw(const Graphics::Bitmap & work, const Camera & camera) = 0;

    static bool order(Drawable * a, Drawable * b);
};

class Player: public Drawable {
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

    Player(double x, double y, const Graphics::Color & color, const Box & box, const Util::ReferenceCount<Behavior> & behavior, bool sideline, double health);

    void act(World & world);
    void setControl(bool what);
    bool hasControl() const;

    void doAction(World & world);
    void doCatch(int time = 30);
    void doPass(World & world);
    
    void collided(Ball & ball);

    bool isDying() const;
    bool isFalling() const;

    bool onGround() const;

    double getHealth() const;
    
    Box collisionBox() const;

    double walkingSpeed() const;

    void moveLeft(double speed);
    void moveRight(double speed);
    void moveUp(double speed);
    void moveDown(double speed);
    void runRight(double speed);
    void runLeft(double speed);

    void faceTowards(double x, double y);

    void grabBall(Ball & ball);

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    void dropBall(Ball & ball);

    int getFacingAngle() const;

    bool hasBall() const;

    bool isFacingRight() const;

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

    void setWalkingAnimation();
    void setRunAnimation();
    void setThrowAnimation();
    void setGrabAnimation();
    void setPainAnimation();
    void setFallAnimation();
    void setCatchAnimation();
    void setIdleAnimation();
    void setRiseAnimation();

protected:
    void throwBall(World & world, Ball & ball);
    Util::ReferenceCount<Animation> getAnimation(const std::string & what);

    double x;
    double y;
    double z;

    double velocityX;
    double velocityY;
    double velocityZ;

    double health;

    static const double jumpVelocity = 15;
    static double maxRunSpeed;

    bool hasBall_;
    Facing facing;
    Box limit;
    Graphics::Color color;
    /* go back to the idle animation if the current one is done */
    bool backToIdle;
    /* true if on the sideline */
    bool sideline;
    /* positive if catching */
    int catching;

    /* if the player goes out of bounds they will walk back in bounds */
    bool forceMove;
    double wantX;
    double wantY;

    int falling;

    Util::ReferenceCount<Behavior> behavior;
    Util::ReferenceCount<Animation> animation;
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
            
    void collisionDetection(World & world, Ball & ball);

    void draw(const Graphics::Bitmap & work, const Camera & camera);

    void removeDead(World & world);

    /* number of non-sideline players left */
    int mainPlayers() const;

    bool onTeam(const Player * who) const;

    void act(World & world);
    const std::vector<Util::ReferenceCount<Player> > & getPlayers() const;

protected:
    void populateLeft(const Field & field);
    void populateRight(const Field & field);

    std::vector<Util::ReferenceCount<Player> > players;
    Side side;
    InputMap<Input> map;
};

class Ball: public Drawable {
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
    Player * getHolder() const;

    int getPower() const;

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

    int power;
    int timeInAir;

    /* true if being grabbed */
    bool grabbed;
    bool thrown;
    bool air;
    Player * holder;
    Team::Side thrownBy;
};

class FloatingText: public Drawable {
public:
    FloatingText(const std::string & text, double x, double y, double z);

    double getX() const;
    double getY() const;

    void act();
    void draw(const Graphics::Bitmap & work, const Camera & camera);
    bool alive();

protected:
    double x, y, z;
    int life;
    int angle;
    const std::string text;
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

    /* game over? */
    bool isDone();

    void addFloatingText(const std::string & text, double x, double y, double z);

    void drawPlayers(const Graphics::Bitmap & work);
    void draw(const Graphics::Bitmap & screen);
    void drawText(const Graphics::Bitmap & work, const Camera & camera);

    void collisionDetection();
    
    void giveControl(const Util::ReferenceCount<Player> & enemy);

    Team::Side findTeam(const Player & player);

    unsigned int getTime() const;

    std::vector<Drawable*> getDrawables();

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
    std::vector<Util::ReferenceCount<FloatingText> > floatingText;
};

class SoundManager{
protected:
    SoundManager();
    std::map<Path::RelativePath, Util::ReferenceCount<Sound> > sounds;
    static Util::ReferenceCount<SoundManager> manager;

public:
    virtual ~SoundManager();
    static Util::ReferenceCount<SoundManager> instance();
    static void destroy();
    Util::ReferenceCount<Sound> getSound(const Path::RelativePath & path);
};

class AnimationManager{
public:
    virtual ~AnimationManager();

    static Util::ReferenceCount<AnimationManager> instance();
    static void destroy();

    Util::ReferenceCount<Animation> getAnimation(const std::string & path, const std::string & animation);

protected:
    AnimationManager();
    std::map<std::string, Util::ReferenceCount<Animation> > loadAnimations(const std::string & path);

    static Util::ReferenceCount<AnimationManager> manager; 
    std::map<std::string, std::map<std::string, Util::ReferenceCount<Animation> > > sets;
    unsigned int id;
};

}

#endif

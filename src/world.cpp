#include "world.h"
#include "util/graphics/bitmap.h"
#include "util/input/input-manager.h"
#include "util/funcs.h"
#include "util/font.h"
#include "util/sound/sound.h"
#include "util/tokenreader.h"
#include "util/token.h"

#include <map>
#include <sstream>
#include <math.h>

using std::vector;
using std::string;
using std::map;

namespace Dodgeball{

static const double gravity = 0.9;

Camera::Camera():
x(0),
y(0),
zoom(0.8){
}

void Camera::zoomIn(double amount){
    zoom += amount;
}

void Camera::zoomOut(double amount){
    zoom -= amount;
    if (zoom < 0.1){
        zoom = 0.1;
    }
}
    
void Camera::setX(double x){
    this->x = x;
}

void Camera::setY(double y){
    this->y = y;
}

void Camera::normalZoom(){
    zoom = 1;
}

int Camera::getWidth() const {
    return 2 * width / getZoom();
}

int Camera::getHeight() const {
    return 2 * height / getZoom();
}

double Camera::getX1() const {
    double x1 = getX() - width / getZoom();
    /*
       if (x1 < 0){
       return 0;
       }
       */
    return x1;
}
    
double Camera::getX2() const {
    double x2 = getX() + width / getZoom();
    /*
       if (x2 > 1000){
       return 1000 - getWidth();
       }
       */
    return x2;
}
    
double Camera::getY1() const {
    double y1 = getY() - height / getZoom();
    /*
       if (y1 < 0){
       return 0;
       }
       */
    return y1;
}
    
double Camera::getY2() const {
    double y2 = getY() + height / getZoom();
    /*
       if (y2 > 1000){
       return y2 - getHeight();
       }
       */
    return y2;
}

double Camera::computeX(double x) const {
    return x - getX1();
}

double Camera::computeY(double y) const {
    return y - getY1();
}

void Camera::moveTowards(double x, double y){
    this->x = (x + this->x) / 2;
    this->y = (y + this->y) / 2;
}

void Camera::moveTo(double x, double y){
    this->x = x;
    this->y = y;
}

void Camera::moveRight(int much){
    this->x += much;
}
    
void Camera::moveLeft(int much){
    this->x -= much;
}
    
void Camera::moveUp(int much){
    this->y -= much;
}

void Camera::moveDown(int much){
    this->y += much;
}

double Camera::getX() const {
    return x;
}

double Camera::getY() const {
    return y;
}

double Camera::getZoom() const {
    return zoom;
}

Field::Field(int width, int height):
width(width),
height(height){
}
    
double Field::getFriction() const {
    return 1.0;
}

int Field::getWidth() const {
    return width;
}

int Field::getHeight() const {
    return height;
}

void Field::draw(const Graphics::Bitmap & work, const Camera & camera){
    int margin = 10;

    /* the outside edge of the line is the boundary */
    int x1 = 0;
    int x2 = width;
    int y1 = 0;
    int y2 = height;

    int middle = (x1 + x2) / 2;

    work.fill(Graphics::makeColor(0, 255, 0));

    Graphics::Color white = Graphics::makeColor(255, 255, 255);

    /* center line */
    work.rectangleFill(camera.computeX(middle - margin / 2), camera.computeY(y1),
                       camera.computeX(middle + margin / 2), camera.computeY(y2),
                       white);

    /* left side */
    work.rectangleFill(camera.computeX(x1), camera.computeY(y1),
                       camera.computeX(x1 + margin), camera.computeY(y2),
                       white);

    /* right side */
    work.rectangleFill(camera.computeX(x2), camera.computeY(y1),
                       camera.computeX(x2 - margin), camera.computeY(y2),
                       white);

    /* top side */
    work.rectangleFill(camera.computeX(x1), camera.computeY(y1),
                       camera.computeX(x2), camera.computeY(y1 + margin),
                       white);

    /* bottom side */
    work.rectangleFill(camera.computeX(x1), camera.computeY(y2),
                       camera.computeX(x2), camera.computeY(y2 - margin),
                       white);
}
    
Behavior::Behavior(){
}

Behavior::~Behavior(){
}
    
Drawable::Drawable(){
}
    
bool Drawable::order(Drawable * a, Drawable * b){
    return a->getY() < b->getY();
}

Drawable::~Drawable(){
}

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

    void reset(){
        count = 0;
        time = 0;
        last = Release;
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

class HumanBehavior: public Behavior {
public:
    enum Input{
        Left,
        Right,
        Up,
        Down,
        Jump,
        Catch,
        Pass,
        /* Either throw or pick up ball */
        Action
    };

    HumanBehavior():
    control(false),
    runningLeft(false),
    runningRight(false){
        map.set(Keyboard::Key_LEFT, Left);
        map.set(Keyboard::Key_RIGHT, Right);
        map.set(Keyboard::Key_UP, Up);
        map.set(Keyboard::Key_DOWN, Down);
        map.set(Keyboard::Key_A, Action);
        map.set(Keyboard::Key_S, Catch);
        map.set(Keyboard::Key_D, Pass);
        map.set(Keyboard::Key_SPACE, Jump);
    }
    
    virtual void resetInput(){
        left.reset();
        right.reset();
        up.reset();
        down.reset();
    }

    void gotBall(Ball & ball){
    }

    void act(World & world, Player & player){
        if (control){
            doInput(world, player);
        } else {
            Ball & ball = world.getBall();
            player.faceTowards(ball.getX(), ball.getY());
        }
    }

    void doInput(World & world, Player & player){
        class Handler: public InputHandler<Input> {
        public:
            Handler(HumanBehavior & human):
            human(human),
            action(false),
            catching(false),
            pass(false),
            jump(false){
            }

            HumanBehavior & human;
            bool action;
            bool catching;
            bool pass;
            bool jump;

            void press(const Input & out, Keyboard::unicode_t unicode){
                switch (out){
                    case Left: {
                        human.left.press();
                        break;
                    }
                    case Right: {
                        human.right.press();
                        break;
                    }
                    case Up: {
                        human.up.press();
                        break;
                    }
                    case Down: {
                        human.down.press();
                        break;
                    }
                    case Catch: {
                        catching = true;
                        break;
                    }
                    case Action: {
                        action = true;
                        break;
                    }
                    case Pass: {
                        pass = true;
                        break;
                    }
                    case Jump: {
                        jump = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }

            }

            void release(const Input & out, Keyboard::unicode_t unicode){
                switch (out){
                    case Left: {
                        human.left.release();
                        break;
                    }
                    case Right: {
                        human.right.release();
                        break;
                    }
                    case Up: {
                        human.up.release();
                        break;
                    }
                    case Down: {
                        human.down.release();
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        };

        left.act();
        right.act();
        up.act();
        down.act();

        Handler handler(*this);
        InputManager::handleEvents(map, InputSource(0, 0), handler);

        if (!player.isFalling() && player.getZ() <= 0){
            if (handler.jump){
                runningLeft = false;
                runningRight = false;
                player.doJump();
            } else {
                if (!runningLeft && !runningRight){
                    if (left.getCount() >= 2){
                        runningLeft = true;
                    } else if (right.getCount() >= 2){
                        runningRight = true;
                    } else {
                        /* just walking */

                        double speed = player.walkingSpeed();

                        bool holdLeft = left.isPressed();
                        bool holdRight = right.isPressed();
                        bool holdUp = up.isPressed();
                        bool holdDown = down.isPressed();

                        if (holdLeft){
                            player.moveLeft(speed);
                        }

                        if (holdRight){
                            player.moveRight(speed);
                        }

                        if (holdUp){
                            player.moveUp(speed);
                        }

                        if (holdDown){
                            player.moveDown(speed);
                        }

                        if (holdLeft && !holdRight && !holdUp && !holdDown){
                            player.setFacing(Player::FaceLeft);
                        }

                        if (holdLeft && holdUp && !holdRight && !holdDown){
                            player.setFacing(Player::FaceUpLeft);
                        }

                        if (holdLeft && holdDown && !holdRight && !holdUp){
                            player.setFacing(Player::FaceDownLeft);
                        }

                        if (holdRight && !holdLeft && !holdUp && !holdDown){
                            player.setFacing(Player::FaceRight);
                        }

                        if (holdRight && holdUp && !holdLeft && !holdDown){
                            player.setFacing(Player::FaceUpRight);
                        }

                        if (holdRight && holdDown && !holdLeft && !holdUp){
                            player.setFacing(Player::FaceDownRight);
                        }

                        if (holdUp && !holdDown && !holdLeft && !holdRight){
                            player.setFacing(Player::FaceUp);
                        }

                        if (holdDown && !holdUp && !holdLeft && !holdRight){
                            player.setFacing(Player::FaceDown);
                        }
                    }
                } else {
                    double runSpeed = 1.5;
                    if (runningLeft){
                        if (!left.isPressed()){
                            runningLeft = false;
                        } else {
                            player.runLeft(runSpeed);
                        }
                    } else if (runningRight){
                        if (!right.isPressed()){
                            runningRight = false;
                        } else {
                            player.runRight(runSpeed);
                        }
                    }
                }
            }
        } else {
            /* Player can turn in the air */
            if (left.isPressed()){
                player.setFacing(Player::FaceDownLeft);
            }

            if (right.isPressed()){
                player.setFacing(Player::FaceRight);
            }
        }

        if (handler.action){
            player.doAction(world);
        } else if (handler.catching){
            player.doCatch();
        } else if (handler.pass && player.hasBall()){
            player.doPass(world);
        }
    }
    
    void setControl(bool what){
        this->control = what;
    }

    bool hasControl() const {
        return this->control;
    }
    
    InputMap<Input> map;
    bool control;

    Hold left;
    Hold right;
    Hold up;
    Hold down;

    bool runningLeft;
    bool runningRight;
};

class DummyBehavior: public Behavior {
public:
    void act(World & world, Player & player){
    }

    void setControl(bool what){
    }

    bool hasControl() const {
        return false;
    }
};

static bool insideBox(double x, double y, const Box & box){
    return x >= box.x1 &&
           x <= box.x2 &&
           y >= box.y1 &&
           y <= box.y2;
} 

class AIBehavior: public Behavior {
public:
    AIBehavior():
    wait(0),
    wantX(0),
    wantY(0),
    want(false){
    }

    int wait;
    int wantX;
    int wantY;
    bool want;

    static bool near(double x1, double y1, double x2, double y2){
        return Util::distance(x1, y1, x2, y2) < 20;
    }

    virtual void resetInput(){
    }

    void gotBall(Ball & ball){
        wait = 20;
    }

    /* If the player has the ball then throw it at an enemy.
     * If the player doesn't have the ball then move towards it. Once close enough
     * pick up the ball.
     */
    void act(World & world, Player & player){
        Ball & ball = world.getBall();

        player.faceTowards(ball.getX(), ball.getY());

        if (wait > 0){
            player.setIdleAnimation();
            wait -= 1;
            return;
        }

        if (player.hasBall()){
            player.doAction(world);
        } else {
            if (player.onGround()){
                if (!ball.isThrown() && insideBox(ball.getX(), ball.getY(), player.getLimit())){
                    if (near(ball.getX(), ball.getY(), player.getX(), player.getY())){
                        player.doAction(world);
                    } else {
                        moveTowards(player, ball.getX(), ball.getY());
                    }
                } else {
                    double sidelineX = (player.getLimit().x1 + player.getLimit().x2) / 2;
                    double sidelineY = (player.getLimit().y1 + player.getLimit().y2) / 2;
                    if (player.onSideline() && Util::distance(player.getX(), player.getY(), sidelineX, sidelineY) > player.walkingSpeed()){
                        moveTowards(player, sidelineX, sidelineY);
                    } else {
                        if (wantX == 0 || wantY == 0 || Util::rnd(200) == 0){
                            wantX = Util::rnd(player.getLimit().x1, player.getLimit().x2);
                            wantY = Util::rnd(player.getLimit().y1, player.getLimit().y2);
                            want = true;
                        } else if (Util::rnd(120) == 0){
                            player.doCatch();
                        }
                        if (want && Util::distance(player.getX(), player.getY(), wantX, wantY) > player.walkingSpeed()){
                            moveTowards(player, wantX, wantY);
                        } else {
                            want = false;
                        }
                    }
                }
            }
        }
    }

    void moveTowards(Player & player, double x, double y){
        double angle = atan2(y - player.getY(), x - player.getX());
        double speed = player.walkingSpeed();
        player.setVelocityX(cos(angle) * speed);
        player.setVelocityY(sin(angle) * speed);
        player.setWalkingAnimation();
    }

    void setControl(bool what){
    }

    bool hasControl() const {
        return true;
    }
};

static string randomName(){
    switch (Util::rnd(10)){
        case 0: return "Bob";
        case 1: return "Randy";
        case 2: return "Sam";
        case 3: return "Mike";
        case 4: return "Tom";
        case 5: return "Alex";
        case 6: return "Bill";
        case 7: return "Greg";
        case 8: return "Chris";
        case 9: return "Derrick";
    }
    return "Guy";
}

Player::Player(double x, double y, const Graphics::Color & color, const Box & box, const Util::ReferenceCount<Behavior> & behavior, bool sideline, double health):
x(x),
y(y),
z(0),
velocityX(0),
velocityY(0),
velocityZ(0),
health(health),
hasBall_(false),
facing(FaceRight),
limit(box),
color(color),
backToIdle(false),
sideline(sideline),
catching(0),
forceMove(false),
wantX(0),
wantY(0),
falling(0),
behavior(behavior),
animation(getAnimation("idle")){
    name = randomName();
}

const string & Player::getName() const {
    return name;
}
    
bool Player::isDying() const {
    return getHealth() < 0 && falling > 0;
}
    
bool Player::isFalling() const {
    return falling > 0;
}

double Player::getHealth() const {
    return health;
}

bool Player::isCatching() const {
    return catching > 0;
}

bool Player::onSideline() const {
    return sideline;
}

bool Player::hasBall() const {
    return hasBall_;
}
    
void Player::faceTowards(double x, double y){
    /*
    double angle = Util::degrees(atan2(y - getY(), x - getX()));
    if (angle < 90 || angle > 360 - 90){
        setFacing(FaceRight);
    } else {
        setFacing(FaceLeft);
    }
    */

    if (x > getX()){
        setFacing(FaceRight);
    } else {
        setFacing(FaceLeft);
    }
}

void Player::doPass(World & world){
    if (hasBall()){
        Util::ReferenceCount<Player> target = world.passTarget(*this);
        double angle = atan2(target->getY() - getY(), target->getX() - getX());
        double speed = 10;

        double distance = Util::distance(getX(), getY(), target->getX(), target->getY());
        double vx = cos(angle) * speed;
        double vy = sin(angle) * speed;
        double vz = 0;
        double time = distance / speed;

        if (onGround() && target->onGround()){
            /* d_z = 1/2 at^2 + v0 * t
             *
             * v0 = d - 1/2 at^2
             *
             * a = gravity
             *
             * d = vt
             * t = d/v
             *
             * d_z = 0 when t_z = t
             *
             * -1/2at^2 = v0 * t
             *  v0 = -1/2at
             */

            /* it would be -gravity except gravity is already negated */
            vz = gravity * time / 2;
        }

        /*
        Global::debug(0) << "Pass from " << getX() << ", " << getY()
                         << " to " << target->getX() << ", " << target->getY()
                         << " distance " << distance << " speed " << speed
                         << " time " << time << " vx " << vx << " vy " << vy << " vz " << vz << std::endl;
         */

        world.getBall().doPass(world, *this, vx, vy, vz);

        hasBall_ = false;
        /* attempt to catch while the ball is in the air */
        target->doCatch(time + 5);
        world.giveControl(target);
    }
}
    
Box Player::getLimit() const {
    return limit;
}
    
bool Player::onGround() const {
    return getZ() <= gravity;
}
    
double Player::walkingSpeed() const {
    return 4.5;
}

void Player::dropBall(Ball & ball){
    if (hasBall()){
        hasBall_ = false;
        ball.ungrab();
    }
}

void Player::act(World & world){
    animation->act();

    if (backToIdle && animation->isDone()){
        backToIdle = false;
        setIdleAnimation();
    }

    if (catching > 0){
        catching -= 1;
        if (catching == 0){
            behavior->resetInput();
            setIdleAnimation();
        }
    }

    if (forceMove && onGround()){
        if (hasBall()){
            dropBall(world.getBall());
        }

        if (Util::distance(getX(), getY(), wantX, wantY) < 3){
            forceMove = false;
            behavior->resetInput();
        } else {
            double angle = atan2(wantY - getY(), wantX - getX());
            velocityX = cos(angle) * walkingSpeed();
            velocityY = sin(angle) * walkingSpeed();
            setWalkingAnimation();
            if (wantX < getX()){
                setFacing(FaceLeft);
            } else {
                setFacing(FaceRight);
            }
        }
    } else {
        if (catching == 0 && falling == 0 && *animation != *getAnimation("rise")){
            behavior->act(world, *this);
        }
    }

    if (z > 0){
        velocityZ -= gravity;
    } else {
        z = 0;
        velocityZ = 0;

        if (falling > 0){
            falling -= 1;
            if (falling == 0){
                setRiseAnimation();
            }
        }

        const Field & field = world.getField();
        if (velocityX > field.getFriction()){
            velocityX -= field.getFriction();
        } else if (velocityX < -field.getFriction()){
            velocityX += field.getFriction();
        } else {
            velocityX = 0;
        }

        if (velocityY > field.getFriction()){
            velocityY -= field.getFriction();
        } else if (velocityY < -field.getFriction()){
            velocityY += field.getFriction();
        } else {
            velocityY = 0;
        }
        
        if (velocityX == 0 && velocityY == 0 && falling == 0 && !backToIdle && !isCatching()){
            setIdleAnimation();
        }
    }

    x += velocityX;
    y += velocityY;

    double oldZ = z;
    z += velocityZ;

    if (falling == 0 && oldZ > 0 && z <= 0){
        setIdleAnimation();
    }

    if (!forceMove && falling == 0){
        /* sidelined players cannot go out of their limit */
        if (onSideline()){
            if (getX() < limit.x1){
                setX(limit.x1);
            }

            if (getX() > limit.x2){
                setX(limit.x2);
            }

            if (getY() < limit.y1){
                setY(limit.y1);
            }

            if (getY() > limit.y2){
                setY(limit.y2);
            }
        } else {
            /* hack alert: we assume the width and height of the limit
             * is larger than 20
             */
            wantX = getX();
            wantY = getY();
            int margin = 30;

            if (getX() < limit.x1){
                forceMove = true;
                wantX = limit.x1 + margin;
            }

            if (getX() > limit.x2){
                forceMove = true;
                wantX = limit.x2 - margin;
            }

            if (getY() < limit.y1){
                forceMove = true;
                wantY = limit.y1 + margin;
            }

            if (getY() > limit.y2){
                forceMove = true;
                wantY = limit.y2 - margin;
            }
        }
    }
}

void Player::setPainAnimation(){
    animation = getAnimation("pain");
    backToIdle = true;
}

void Player::setFallAnimation(){
    animation = getAnimation("fall");
    backToIdle = false;
}

void Player::collided(Ball & ball){
    setFallAnimation();
    catching = 0;
    falling = 40;
    velocityX = 8;
    if (!onSideline()){
        health -= ball.getPower();
    }
    if (ball.getVelocityX() < 0){
        velocityX *= -1;
    }
    velocityY = 0;
    velocityZ = 9;
    z = 0.1;
}
    
double Player::getWidth() const {
    return 60;
}

double Player::getHeight() const {
    return 130;
}

void Player::setFacing(Facing face){
    this->facing = face;
}

double Player::getX1() const {
    return getX() - getWidth() / 2;
}

double Player::getY1() const {
    return getY() - getZ() - getHeight();
}

void Player::setControl(bool what){
    behavior->setControl(what);
    behavior->resetInput();
}
    
void Player::setVelocityX(double x){
    this->velocityX = x;
}

void Player::setVelocityY(double y){
    this->velocityY = y;
}
    
bool Player::hasControl() const {
    return behavior->hasControl();
}
    
double Player::getHandPosition() const {
    return getZ() + 50;
}
    
Box Player::collisionBox() const {
    int width = 30;
    int height = 130;
    return Box(0, 0, width, height);
}

void Hold::act(){
    if (time > 0){
        time -= 1;
    } else {
        if (last == Pressed){
            if (count >= 1){
                count = 1;
            }
        } else {
            count = 0;
        }
    }
}
    
unsigned int Hold::getCount() const {
    return count;
}
    
bool Hold::isPressed() const {
    return last == Pressed;
}

void Hold::press(){
    count += 1;
    time = 10;
    last = Pressed;
}

void Hold::release(){
    last = Release;
}


void Player::doJump(){
    animation = getAnimation("jump");
    velocityZ = jumpVelocity;
    /* set the z to some initial value above 0 so that it doesn't look like we
     * are hitting the ground.
     */
    z = 0.1;
}

double findAngle(double x1, double y1, double x2, double y2){
    return atan2(y2 - y1, x2 - x1);
}

Util::ReferenceCount<Animation> Player::getAnimation(const string & what){
    return AnimationManager::instance()->getAnimation("alex", what)->clone();
}

void Player::setThrowAnimation(){
    animation = getAnimation("punch");
    backToIdle = true;
}

void Player::throwBall(World & world, Ball & ball){
    SoundManager::instance()->getSound(Filesystem::RelativePath("throw.wav"))->play();
    setThrowAnimation();
    Util::ReferenceCount<Player> enemy = world.getTarget(*this);
    world.giveControl(enemy);

    double angle = findAngle(getX(), getY(), enemy->getX() + Util::rnd(-5, 5), enemy->getY() + Util::rnd(-5, 5));

    double speed = 9 + sqrt(velocityX * velocityX + velocityY * velocityY);

    double vx = 0;
    if (z > 0){
        /* distance = velocity * time
         * d = vt
         *
         * The distance traveled in the x-y plane (when z = 0) is
         * the distance from the player's x,y to the enemy's x,y.
         * Velocity is the speed variable above. So the time is
         *   t = d/v
         *
         * We want the ball to start at some z height and reach 0 when the
         * x-y distance is traveled so
         *   zdist = zv * zt
         * zt is the same as the x-y t
         *   zt = t
         *
         * So we can equate the distances and the times. The z distance is exactly
         * the z height of where the ball starts.
         *
         *   d/v = zd/zv
         *
         * Solve for zv
         * 
         *   zv = zd*v/d
         */

        double ground = Util::distance(getX(), getY(), enemy->getX(), enemy->getY());

        vx = -(getHandPosition() + Util::rnd(-5, 5)) * speed / ground;
    }
    
    ball.doThrow(world, *this, cos(angle) * speed, sin(angle) * speed, vx);
}
    
void Player::setCatchAnimation(){
    /* FIXME: bad animation here */
    animation = getAnimation("upper-cut");
    animation->setLoop(true);
}
    
void Player::doCatch(int time){
    catching = time;
    setCatchAnimation();
}

void Player::doAction(World & world){
    if (hasBall()){
        Ball & ball = world.getBall();
        throwBall(world, ball);
        hasBall_ = false;
    } else {
        if (world.getBall().getZ() < 1 &&
            Util::distance(getX(), getY(), world.getBall().getX(), world.getBall().getY()) < 20){
            grabBall(world.getBall());
        }
    }
}

void Player::setGrabAnimation(){
    animation = getAnimation("get");
    backToIdle = true;
}

void Player::grabBall(Ball & ball){
    behavior->gotBall(ball);
    setGrabAnimation();
    catching = 0;
    ball.grab(this);
    hasBall_ = true;
}

void Player::moveLeft(double speed){
    setWalkingAnimation();
    this->velocityX = -speed;
    // this->x -= speed;
}

void Player::setWalkingAnimation(){
    Util::ReferenceCount<Animation> walk = getAnimation("walk");
    if (*animation != *walk){
        animation = walk->clone();
        animation->setLoop(true);
    }
}
    
void Player::setIdleAnimation(){
    animation = getAnimation("idle");
}
    
void Player::setRiseAnimation(){
    animation = getAnimation("rise");
    backToIdle = true;
}

void Player::setRunAnimation(){
    Util::ReferenceCount<Animation> run = getAnimation("run");
    if (*animation != *run){
        animation = run->clone();
        animation->setLoop(true);
    }
}


void Player::moveRight(double speed){
    setWalkingAnimation();
    this->velocityX = speed;
    // this->x += speed;
}
    
double Player::maxRunSpeed = 9;
    
void Player::runRight(double speed){
    setRunAnimation();
    this->velocityX += speed;
    if (this->velocityX > maxRunSpeed){
        this->velocityX = maxRunSpeed;
    }
}

void Player::runLeft(double speed){
    setRunAnimation();
    this->velocityX -= speed;
    if (this->velocityX < -maxRunSpeed){
        this->velocityX = -maxRunSpeed;
    }
}

void Player::moveUp(double speed){
    setWalkingAnimation();
    this->velocityY = -speed;
    // this->y -= speed;
}

void Player::moveDown(double speed){
    setWalkingAnimation();
    this->velocityY = speed;
    // this->y += speed;
}

int Player::getFacingAngle() const {
    switch (facing){
        case FaceRight: return 0;
        case FaceUp: return 90;
        case FaceLeft: return 180;
        case FaceDown: return 270;
        case FaceUpLeft: return (90 + 180) / 2;
        case FaceUpRight: return (90 + 0) / 2;
        case FaceDownLeft: return (270 + 180) / 2;
        case FaceDownRight: return (270 + 360) / 2;
    }
    return 0;
}

bool Player::isFacingRight() const {
    switch (facing){
        case FaceUp:
        case FaceUpLeft:
        case FaceDownLeft:
        case FaceLeft: return false;

        case FaceRight:
        case FaceUpRight:
        case FaceDown:
        case FaceDownRight: return true;
    }

    return true;
}

void Player::draw(const Graphics::Bitmap & work, const Camera & camera){
    int width = getWidth();
    int height = getHeight();
    // work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 10, 5, Graphics::makeColor(32, 32, 32));

    /*
    double facingX = cos(Util::radians(getFacingAngle())) * 40;
    double facingY = -sin(Util::radians(getFacingAngle())) * 40;
    work.line((int) camera.computeX(x), (int) camera.computeY(y),
              (int) camera.computeX(x + facingX), (int) camera.computeY(y + facingY),
              Graphics::makeColor(255, 0, 0));
              */

    if (hasControl()){
        work.translucent(0, 0, 0, 128).ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 40, 20, Graphics::makeColor(0xff, 0xda, 0x00));
    }
    /*
    if (hasBall()){
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 20, 10, Graphics::makeColor(255, 255, 0));
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 21, 11, Graphics::makeColor(255, 255, 0));
    }
    */

    /*
    work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y - z - height / 2), width / 2, height / 2, color);
    int handx = camera.computeX(x + 10);
    int handy = camera.computeY(y - getHandPosition());
    Graphics::Color handColor = Graphics::makeColor(255, 255, 255);
    if (isCatching()){
        handColor = Graphics::makeColor(255, 255, 0);
    }
    work.ellipseFill(handx, handy, 16, 6, handColor);

    work.circleFill((int) camera.computeX(x + 3), (int) camera.computeY(y - z - height * 3 / 4), 5, Graphics::makeColor(255, 255, 255));
    */

    animation->draw(work, (int) camera.computeX(x), (int) camera.computeY(y - z), isFacingRight());

    if (!onSideline()){
        const Font & font = Font::getDefaultFont(24, 24);
        font.printf((int) camera.computeX(x - font.textLength(getName().c_str()) / 2), (int) camera.computeY(y - z), Graphics::makeColor(255, 255, 255), work, getName(), 0);
    }
}

double Player::getX() const {
    return x;
}
    
double Player::getZ() const {
    return z;
}
    
double Player::getY() const {
    return y;
}
    
void Player::setX(double x){
    this->x = x;
}

void Player::setY(double y){
    this->y = y;
}

Team::Team(Side side, const Field & field):
side(side){
    switch (side){
        case LeftSide: populateLeft(field); break;
        case RightSide: populateRight(field); break;
    }
}
    
int Team::mainPlayers() const {
    int count = 0;

    for (vector<Util::ReferenceCount<Player> >::const_iterator it = players.begin(); it != players.end(); it++){
        Util::ReferenceCount<Player> player = *it;
        if (!player->onSideline()){
            count += 1;
        }
    }

    return count;
}
    
void Team::removeDead(World & world){
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); /**/){
        Util::ReferenceCount<Player> player = *it;
        if (player->getHealth() > 0 || player->isDying()){
            it++;
        } else {
            if (player->hasBall()){
                player->dropBall(world.getBall());
            }
            it = players.erase(it);
        }
    }
}
    
Team::Side Team::getSide() const {
    return side;
}

static bool boxCollide( int zx1, int zy1, int zx2, int zy2, int zx3, int zy3, int zx4, int zy4 ){
    if (zx1 < zx3 && zx1 < zx4 &&
         zx2 < zx3 && zx2 < zx4) return false;
    if (zx1 > zx3 && zx1 > zx4 &&
         zx2 > zx3 && zx2 > zx4) return false;
    if (zy1 < zy3 && zy1 < zy4 &&
         zy2 < zy3 && zy2 < zy4) return false;
    if (zy1 > zy3 && zy1 > zy4 &&
         zy2 > zy3 && zy2 > zy4) return false;

    return true;
}

static bool boxCollide(double x1, double y1, const Box & box1,
                       double x2, double y2, const Box & box2){
    return boxCollide(x1 + box1.x1, y1 + box1.y1, x1 + box1.x2, y1 + box1.y2,
                      x2 + box2.x1, y2 + box2.y1, x2 + box2.x2, y2 + box2.y2);
}

template <class X>
static string toString(const X & x){
    std::ostringstream out;
    out << x;
    string result = out.str();
    if (result.size() == 1){
        return string("0") + result;
    }
    return result;
}

static bool isFacing(double x1, double y1, double angle1, double x2, double y2){
    double distance = Util::distance(x1, y1, x2, y2);
    double hx = (x2 - x1) / distance;
    double hy = (y2 - y1) / distance;
    double dot = hx * cos(angle1) + hy * sin(angle1);
    return dot > 0.1;
}

void Team::collisionDetection(World & world, Ball & ball){
    Box ballBox = ball.collisionBox();
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        Util::ReferenceCount<Player> player = *it;
        Box playerBox = player->collisionBox();
        
        if (fabs(player->getY() - ball.getY()) <= 5 &&
            boxCollide(player->getX1(), player->getY1(), playerBox,
                       ball.getX1(), ball.getY1(), ballBox)){

            /* TODO: handle when the ball is in the air but the player didn't catch it.
             * The ball should just bounce off of them without them taking damage
             * but they should show a slight getting-hit animation.
             */
            if (player->isCatching() && isFacing(player->getX(), player->getY(), Util::radians(player->getFacingAngle()), ball.getX(), ball.getY())){
                player->grabBall(ball);
            } else if (ball.isThrown()){
                SoundManager::instance()->getSound(Filesystem::RelativePath("beat1.wav"))->play();
                player->collided(ball);
                ball.collided(*player);
                world.addFloatingText(toString(ball.getPower()), player->getX(), player->getY(), player->getZ() + 5);
            }

            /* cannot hit multiple players. TODO: some specials can hit multiple players */
            break;
        }
    }
}

const std::vector<Util::ReferenceCount<Player> > & Team::getPlayers() const {
    return players;
}

static Util::ReferenceCount<Player> makePlayer(double x, double y, const Graphics::Color & color, const Box & box, const Util::ReferenceCount<Behavior> & behavior, bool sideline, double health){
    return Util::ReferenceCount<Player>(new Player(x, y, color, box, behavior, sideline, health));
}

void Team::populateLeft(const Field & field){
    map.set(Keyboard::Key_Q, Cycle);
    double width = field.getWidth() / 2;
    double height = field.getHeight();
    double health = 50;
    Graphics::Color color(Graphics::makeColor(255, 0, 0));
    players.push_back(makePlayer(width / 5, height / 2, color, Box(0, 0, width, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), false, health));
    players.push_back(makePlayer(width / 2, height / 4, color, Box(0, 0, width, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), false, health));
    players.push_back(makePlayer(width / 2, height * 3 / 4, color, Box(0, 0, width, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), false, health));

    players.push_back(makePlayer(field.getWidth() - 0, height / 2, color, Box(field.getWidth() - 0, 0, field.getWidth() - 0, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), true, health));
    players.push_back(makePlayer(field.getWidth() - width / 2, -10, color, Box(field.getWidth() - width, -10, field.getWidth(), -10), Util::ReferenceCount<Behavior>(new HumanBehavior()), true, health));
    players.push_back(makePlayer(field.getWidth() - width / 2, height + 10, color, Box(field.getWidth() - width, height + 10, field.getWidth(), height + 10), Util::ReferenceCount<Behavior>(new HumanBehavior()), true, health));
}

void Team::populateRight(const Field & field){
    double width = field.getWidth() / 2;
    double height = field.getHeight();
    double health = 50;
    Graphics::Color color(Graphics::makeColor(0x00, 0xaf, 0x64));
    players.push_back(makePlayer(field.getWidth() - width / 5, height / 2, color, Box(field.getWidth() - width, 0, field.getWidth(), height), Util::ReferenceCount<Behavior>(new AIBehavior()), false, health));
    players.push_back(makePlayer(field.getWidth() - width / 2, height / 4, color, Box(field.getWidth() - width, 0, field.getWidth(), height), Util::ReferenceCount<Behavior>(new AIBehavior()), false, health));
    players.push_back(makePlayer(field.getWidth() - width / 2, height * 3 / 4, color, Box(field.getWidth() - width, 0, field.getWidth(), height), Util::ReferenceCount<Behavior>(new AIBehavior()), false, health));

    players.push_back(makePlayer(-10, height / 2, color, Box(-10, 0, -10, height), Util::ReferenceCount<Behavior>(new AIBehavior()), true, health));
    players.push_back(makePlayer(width / 2, -10, color, Box(0, -10, width, -10), Util::ReferenceCount<Behavior>(new AIBehavior()), true, health));
    players.push_back(makePlayer(width / 2, height + 10, color, Box(0, height + 10, width, height + 10), Util::ReferenceCount<Behavior>(new AIBehavior()), true, health));
}

void Team::enableControl(){
    if (players.size() > 0){
        players[0]->setControl(true);
    }
}

bool Team::onTeam(const Player * who) const {
    for (vector<Util::ReferenceCount<Player> >::const_iterator it = getPlayers().begin(); it != getPlayers().end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        if (player == who){
            return true;
        }
    }
    return false;
}

void Team::cycleControl(World & world){
    Util::ReferenceCount<Player> use(NULL);
    double best = 9999;

    const Ball & ball = world.getBall();

    if (onTeam(ball.getHolder())){
        /* Can't cycle control if someone on the team is holding the ball,
         * instead you have to pass it.
         */
        return;
    }

    /* find closest player to the ball and give him control */
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        Util::ReferenceCount<Player> player = *it;
        bool control = player->hasControl();
        player->setControl(false);
        double distance = Util::distance(player->getX(), player->getY(), ball.getX(), ball.getY());
        if (!control && distance < best){
            use = player;
            best = distance;
        }
    }

    use->setControl(true);
}
    
void Team::giveControl(const Util::ReferenceCount<Player> & who){
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        Util::ReferenceCount<Player> player = *it;
        player->setControl(false);
    }

    who->setControl(true);
}

bool yPosition(const Util::ReferenceCount<Player> & a,
               const Util::ReferenceCount<Player> & b){
    return a->getY() < b->getY();
}

void Team::draw(const Graphics::Bitmap & work, const Camera & camera){
    vector<Util::ReferenceCount<Player> > order = players;
    sort(order.begin(), order.end(), yPosition);
    for (vector<Util::ReferenceCount<Player> >::iterator it = order.begin(); it != order.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        player->draw(work, camera);
    }
}

void Team::act(World & world){
    class Handler: public InputHandler<Input> {
    public:
        Handler(Team & team, World & world):
        team(team),
        world(world){
        }

        Team & team;
        World & world;

        void press(const Input & out, Keyboard::unicode_t unicode){
            switch (out){
                case Cycle: {
                    team.cycleControl(world);
                    break;
                }
            }
        }

        void release(const Input & out, Keyboard::unicode_t unicode){
        }
    };

    Handler handler(*this, world);
    InputManager::handleEvents(map, InputSource(0, 0), handler);

    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        player->act(world);
    }
}

Ball::Ball(double x, double y):
x(x),
y(y),
z(0),
angle(Util::rnd(360)),
velocityX(0),
velocityY(0),
velocityZ(0),
power(0),
timeInAir(0),
grabbed(false),
thrown(false),
air(false),
holder(NULL){
}
    
Player * Ball::getHolder() const {
    return holder;
}

void Ball::grab(Player * holder){
    grabbed = true;
    air = false;
    thrown = false;
    this->holder = holder;
}

void Ball::ungrab(){
    grabbed = false;
    holder = NULL;
}

int Ball::getPower() const {
    return sqrt(velocityX * velocityX + velocityY * velocityY + velocityZ * velocityZ);
    // power;
}
    
double Ball::getVelocityX() const {
    return velocityX;
}

double Ball::getVelocityY() const {
    return velocityY;
}

void Ball::collided(Player & player){
    thrown = false;
    air = false;
    timeInAir = 0;
    velocityX = -velocityX / 2;
    velocityY = -velocityY / 2;
}
    
double Ball::getX1() const {
    int size = 25;
    return getX() - size / 2;
}

double Ball::getY1() const {
    int size = 25;
    return getY() - getZ() - size / 2;
}
    
bool Ball::isThrown() const {
    return thrown;
}

void Ball::doThrow(World & world, Player & player, double velocityX, double velocityY, double velocityZ){
    power = 3;
    thrownBy = world.findTeam(player);
    ungrab();
    air = true;
    thrown = true;
    timeInAir = 200;
    this->velocityX = velocityX;
    this->velocityY = velocityY;
    this->velocityZ = velocityZ;
}

void Ball::doPass(World & world, Player & player, double velocityX, double velocityY, double velocityZ){
    ungrab();
    thrownBy = world.findTeam(player);
    timeInAir = 0;
    thrown = false;
    air = true;
    this->velocityX = velocityX;
    this->velocityY = velocityY;
    this->velocityZ = velocityZ;
}
    
bool Ball::inAir() const {
    return air;
}

double Ball::getX() const {
    return x;
}
    
double Ball::getZ() const {
    return z;
}

double Ball::getY() const {
    return y;
}

void Ball::act(const Field & field){
    if (grabbed && holder != NULL){
        this->x = holder->getX();
        /* add 0.1 to make sure the ball is drawn in front of the player */
        this->y = holder->getY() + 0.1;
        this->z = holder->getHandPosition();
    } else {
        x += velocityX;
        y += velocityY;
        z += velocityZ;

        if (z > 0){
            if (timeInAir <= 0){
                velocityZ -= gravity;
            } else {
                timeInAir -= 1;
            }
        } else {
            /* When the ball hits the ground its not being thrown anymore */
            thrown = false;
            air = false;
            timeInAir = 0;

            velocityZ = -velocityZ / 2;
            if (fabs(velocityZ) < gravity){
                velocityZ = 0;
            }
            z = 0;

            if (velocityX > field.getFriction()){
                velocityX -= field.getFriction();
            } else if (velocityX < -field.getFriction()){
                velocityX += field.getFriction();
            } else {
                velocityX = 0;
            }

            if (velocityY > field.getFriction()){
                velocityY -= field.getFriction();
            } else if (velocityY < -field.getFriction()){
                velocityY += field.getFriction();
            } else {
                velocityY = 0;
            }

        }

        angle += fabs(velocityX) + fabs(velocityY);
        if (angle > 360){
            angle -= 360;
        }
        if (angle < 0){
            angle += 360;
        }
    }

    if (x < -10 || x > field.getWidth() + 10){
        if (x < -10){
            x = -10;
        }
        if (x > field.getWidth() + 10){
            x = field.getWidth() + 10;
        }

        timeInAir = 0;
        velocityX = -velocityX / 2;
    }
    if (y < -10 || y > field.getHeight() + 10){
        timeInAir = 0;
        velocityY = -velocityY / 2;
    }
}

void Ball::draw(const Graphics::Bitmap & work, const Camera & camera){
    int size = 25;
    int middleX = camera.computeX(x);
    int middleY = camera.computeY(y - z - size);

    work.translucent(0, 0, 0, 128).ellipseFill(camera.computeX(x + size / 2), camera.computeY(y),
                     size, size / 2, Graphics::makeColor(32, 32, 32));

    work.circleFill(middleX, middleY, size,
                    Graphics::makeColor(255, 255, 255));

    double radians = Util::radians(angle);
    work.line(middleX - cos(radians) * size, middleY - sin(radians) * size,
              middleX + cos(radians) * size, middleY + sin(radians) * size,
              Graphics::makeColor(0, 0, 0));

    radians = Util::radians(angle + 90);
    work.line(middleX - cos(radians) * size, middleY - sin(radians) * size,
              middleX + cos(radians) * size, middleY + sin(radians) * size,
              Graphics::makeColor(0, 0, 0));
}
    
Box Ball::collisionBox() const {
    int size = 25;
    return Box(0, 0, size, size);
}

World::World():
field(1200, 600),
ball(400, 300),
team1(Team::LeftSide, field),
team2(Team::RightSide, field){
    camera.moveTo(field.getWidth() / 2, field.getHeight() / 2);
    map.set(Keyboard::Key_LEFT, Left);
    map.set(Keyboard::Key_RIGHT, Right);
    map.set(Keyboard::Key_UP, Up);
    map.set(Keyboard::Key_DOWN, Down);
    map.set(Keyboard::Key_EQUALS, ZoomIn);
    map.set(Keyboard::Key_MINUS, ZoomOut);
                
    /* preload the sounds */
    SoundManager::instance()->getSound(Filesystem::RelativePath("beat1.wav"));

    team1.enableControl();
}
    
bool World::isDone(){
    return team1.mainPlayers() == 0 ||
           team2.mainPlayers() == 0;
}

void World::collisionDetection(){
    if (ball.inAir()){
        if (ball.isThrown()){
            if (ball.thrownBy == team1.getSide()){
                team2.collisionDetection(*this, ball);
            } else {
                team1.collisionDetection(*this, ball);
            }
        } else {
            team1.collisionDetection(*this, ball);
            if (ball.inAir()){
                team2.collisionDetection(*this, ball);
            }
        }
    }
}

static void eraseDead(vector<Util::ReferenceCount<FloatingText> > & stuff){
    for (vector<Util::ReferenceCount<FloatingText> >::iterator it = stuff.begin(); it != stuff.end(); /**/){
        Util::ReferenceCount<FloatingText> text = *it;
        if (text->alive()){
            it++;
        } else {
            it = stuff.erase(it);
        }
    }
}

void World::run(){
    class Handler: public InputHandler<Input> {
    public:
        Handler(World & world):
        world(world){
        }

        void press(const Input & out, Keyboard::unicode_t unicode){
            switch (out){
                /*
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
                */
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

    time += 1;

    Handler handler(*this);

    InputManager::handleEvents(map, InputSource(0, 0), handler);
    team1.act(*this);
    team2.act(*this);
    ball.act(field);

    collisionDetection();

    for (vector<Util::ReferenceCount<FloatingText> >::iterator it = floatingText.begin(); it != floatingText.end(); it++){
        Util::ReferenceCount<FloatingText> text = *it;
        text->act();
    }

    eraseDead(floatingText);

    team1.removeDead(*this);
    team2.removeDead(*this);

    camera.moveTowards(ball.getX(), ball.getY());
    int xbounds = 50;
    int ybounds = 30;
    if (camera.getX1() < -xbounds && camera.getX2() < field.getWidth()){
        camera.setX(-xbounds + camera.getWidth() / 2);
    }

    if (camera.getX2() > field.getWidth() + xbounds && camera.getX1() > 0){
        camera.setX(field.getWidth() + xbounds - camera.getWidth() / 2);
    }

    /*
    if (camera.getY1() < -ybounds && camera.getY2() < field.getHeight() + ybounds){
        camera.setY(-ybounds + camera.getHeight() / 2);
    }

    if (camera.getY2() > field.getHeight() + ybounds && camera.getY1() > -ybounds){
        camera.setY(field.getHeight() + ybounds - camera.getHeight() / 2);
    }
    */
}

void World::moveLeft(){
    camera.moveLeft(5);
}

void World::moveRight(){
    camera.moveRight(5);
}
    
void World::moveUp(){
    camera.moveUp(5);
}
    
void World::moveDown(){
    camera.moveDown(5);
}

vector<Drawable*> World::getDrawables(){
    vector<Drawable*> out;

    out.push_back(&ball);

    vector<Util::ReferenceCount<Player> > players = team1.getPlayers();
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        Util::ReferenceCount<Player> player = *it;
        out.push_back(player.raw());
    }

    players = team2.getPlayers();
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        Util::ReferenceCount<Player> player = *it;
        out.push_back(player.raw());
    }

    for (vector<Util::ReferenceCount<FloatingText> >::iterator it = floatingText.begin(); it != floatingText.end(); it++){
        Util::ReferenceCount<FloatingText> text = *it;
        out.push_back(text.raw());
    }

    sort(out.begin(), out.end(), Drawable::order);

    return out;
}

void World::drawOverlay(const Graphics::Bitmap & work){
    const Font & font = Font::getDefaultFont(24, 24);
    work.translucent(0, 0, 0, 128).rectangleFill(0, 0, work.getWidth(), (font.getHeight() + 2) * 3 + 10, Graphics::makeColor(0, 0, 0));

    int x = 5;
    int y = 2;
    for (vector<Util::ReferenceCount<Player> >::const_iterator it = team1.getPlayers().begin(); it != team1.getPlayers().end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        if (!player->onSideline()){
            font.printf(x, y, Graphics::makeColor(255, 255, 255), work, player->getName(), 0);

            int healthX = x + font.textLength("aaaaaaa");
            int width = 3;
            for (int i = 0; i < player->getHealth() / 10; i++){
                work.rectangleFill(healthX, y + 1, healthX + width, y + font.getHeight() - 1, Graphics::makeColor(255, 255, 255));
                healthX += width * 3;
            }

            y += font.getHeight() + 2;
        }
    }

    x = work.getWidth() / 2;
    y = 2;
    for (vector<Util::ReferenceCount<Player> >::const_iterator it = team2.getPlayers().begin(); it != team2.getPlayers().end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        if (!player->onSideline()){
            font.printf(x, y, Graphics::makeColor(255, 255, 255), work, player->getName(), 0);
            int healthX = x + font.textLength("aaaaaaa");
            int width = 3;
            for (int i = 0; i < player->getHealth() / 10; i++){
                work.rectangleFill(healthX, y + 1, healthX + width, y + font.getHeight() - 1, Graphics::makeColor(255, 255, 255));
                healthX += width * 3;
            }

            y += font.getHeight() + 2;
        }
    }

}

void World::draw(const Graphics::Bitmap & screen){
    Graphics::StretchedBitmap work(camera.getWidth(), camera.getHeight(), screen);
    work.start();
    field.draw(work, camera);

    vector<Drawable*> draws = getDrawables();
    for (vector<Drawable*>::iterator it = draws.begin(); it != draws.end(); it++){
        Drawable * what = *it;
        what->draw(work, camera);
    }

    drawOverlay(work);

    work.finish();
}
    
Ball & World::getBall(){
    return ball;
}
    
const Field & World::getField() const {
    return field;
}

unsigned int World::getTime() const {
    return time;
}

bool World::onTeam(const Team & team, const Player & who){
    return team.onTeam(&who);
}

Util::ReferenceCount<Player> World::getTarget(const vector<Util::ReferenceCount<Player> > & players, Player & who){
    Util::ReferenceCount<Player> best(NULL);
    double bestDot = -999;

    double startingAngle = Util::radians(who.getFacingAngle());
    for (vector<Util::ReferenceCount<Player> >::const_iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        /* can't target sidelined players */
        if (!player->onSideline()){
            /* choose the player closest to angle 0, directly horizontal */

            /*
            double angle = atan2(player->getY() - who.getY(), player->getX() - who.getX());
            if (fabs(angle) < bestAngle){
                bestAngle = angle;
                best = player;
            }
            */
            double distance = Util::distance(player->getX(), player->getY(), who.getX(), who.getY());
            double hx = (player->getX() - who.getX()) / distance;
            double hy = (player->getY() - who.getY()) / distance;
            double dot = hx * cos(startingAngle) + hy * sin(startingAngle);
            if (dot > bestDot){
                bestDot = dot;
                best = player;
            }
        }
    }

    return best;
}

Util::ReferenceCount<Player> World::getTarget(Player & who){
    if (onTeam(team1, who)){
        return getTarget(team2.getPlayers(), who);
    }
    return getTarget(team1.getPlayers(), who);
}

void World::giveControl(const Util::ReferenceCount<Player> & enemy){
    if (onTeam(team1, *enemy)){
        team1.giveControl(enemy);
    } else {
        team2.giveControl(enemy);
    }
}

Team::Side World::findTeam(const Player & player){
    if (onTeam(team1, player)){
        return team1.getSide();
    }
    return team2.getSide();
}

/* low to high counter clockwise */
static bool inRange(double low, double high, double angle){

    if (low < high){
        return angle >= low && angle <= high;
    }

    return inRange(0, high, angle) && inRange(low, Util::pi * 2, angle);

    /*
    if (low < 0){
        if (angle < low + Util::pi * 2){
            return false;
        }
    }

    if (high > Util::pi * 2){
        if (angle > high - Util::pi * 2){
            return false;
        }
    }

    return angle > low && angle < high;
    */
}

static bool inFront(double angle1, double angle2){
    return inRange(angle1 - Util::pi/4, angle1 + Util::pi/4, angle2);
}

Util::ReferenceCount<Player> World::passTarget(const std::vector<Util::ReferenceCount<Player> > & players, Player & who){
    Util::ReferenceCount<Player> best(NULL);
    double closest = 9999;

    double startingAngle = Util::radians(who.getFacingAngle());

    for (vector<Util::ReferenceCount<Player> >::const_iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        if (player != &who){
            double distance = Util::distance(player->getX(), player->getY(), who.getX(), who.getY());
            // double angle = atan2(who.getY() - player->getY(), who.getX() - player->getX()) + Util::pi;
            double hx = (player->getX() - who.getX()) / distance;
            double hy = (player->getY() - who.getY()) / distance;
            double dot = hx * cos(startingAngle) + hy * sin(startingAngle);
            // Global::debug(0) << "Angle between " << who.getX() << ", " << who.getY() << " and " << player->getX() << ", " << player->getY() << " is " << angle << " starting angle " << startingAngle << std::endl;
            /* cull objects behind the player */
            if (dot > 0.3 && distance < closest){
                closest = distance;
                best = player;
            }
        }
    }

    if (best == NULL){
        vector<Util::ReferenceCount<Player> > copy = players;
        for (vector<Util::ReferenceCount<Player> >::iterator it = copy.begin(); it != copy.end(); /**/){
            if (*it == &who){
                it = copy.erase(it);
            } else {
                it++;
            }
        }

        return copy[Util::rnd(0, copy.size() - 1)];
    }

    return best;
}

Util::ReferenceCount<Player> World::passTarget(Player & who){
    if (onTeam(team1, who)){
        return passTarget(team1.getPlayers(), who);
    }
    return passTarget(team2.getPlayers(), who);
}
    
void World::addFloatingText(const std::string & text, double x, double y, double z){
    floatingText.push_back(Util::ReferenceCount<FloatingText>(new FloatingText(text, x, y, z)));
}

FloatingText::FloatingText(const std::string & text, double x, double y, double z):
x(x),
y(y),
z(z),
life(100),
angle(0),
text(text){
}
    
double FloatingText::getX() const {
    return x;
}

double FloatingText::getY() const {
    return y;
}

bool FloatingText::alive(){
    return life > 0;
}

void FloatingText::act(){
    life -= 1;
    angle += 1;
    z += 1;
}

void FloatingText::draw(const Graphics::Bitmap & work, const Camera & camera){
    int drawX = camera.computeX(x + cos(angle / 4) * 5);
    int drawY = camera.computeY(y - z);
    Font::getDefaultFont(40, 40).printf(drawX, drawY, Graphics::makeColor(255, 255, 255), work, text, 0);
}

SoundManager::SoundManager(){
}

SoundManager::~SoundManager(){
}
    
Util::ReferenceCount<SoundManager> SoundManager::manager;

void SoundManager::destroy(){
    manager = NULL;
}

Util::ReferenceCount<SoundManager> SoundManager::instance(){
    if (manager == NULL){
        manager = Util::ReferenceCount<SoundManager>(new SoundManager());
    }

    return manager;
}

Util::ReferenceCount<Sound> SoundManager::getSound(const Path::RelativePath & path){
    if (sounds.find(path) == sounds.end()){
        sounds[path] = Util::ReferenceCount<Sound>(new Sound(Storage::instance().find(path).path()));
    }

    return sounds[path];
}
    
AnimationEvent::AnimationEvent(){
}

AnimationEvent::~AnimationEvent(){
}

class DelayEvent: public AnimationEvent {
public:
    DelayEvent(const Token * token):
    delay(1){
        token->view() >> delay;
    }
    
    void invoke(Animation & animation){
        animation.setDelay(delay);
    }

    int delay;
};

class FrameEvent: public AnimationEvent {
public:
    FrameEvent(const Filesystem::AbsolutePath & directory, const Token * token){
        string path;
        token->view() >> path;
        frame = Graphics::Bitmap(directory.join(Filesystem::RelativePath(path)).path());
    }
    
    void invoke(Animation & animation){
        animation.setFrame(frame);
    }

    Graphics::Bitmap frame;
};

class OffsetEvent: public AnimationEvent {
public:
    OffsetEvent(const Token * token):
    x(0),
    y(0){
        token->view() >> x >> y;
    }
    
    void invoke(Animation & animation){
        animation.setOffset(x, y);
    }

    int x, y;
};
    
Animation::Animation(const Filesystem::AbsolutePath & directory, const Token * token, unsigned int id):
x(0), y(0),
delay(0),
counter(0),
loop(false),
id(id){
    TokenView view = token->view();
    while (view.hasMore()){
        const Token * next;
        view >> next;
        if (*next == "basedir"){
            string path;
            next->view() >> path;
            setBaseDirectory(directory.join(Filesystem::RelativePath(path)));
        } else if (*next == "delay"){
            events.push_back(Util::ReferenceCount<AnimationEvent>(new DelayEvent(next)));
        } else if (*next == "frame"){
            events.push_back(Util::ReferenceCount<AnimationEvent>(new FrameEvent(getBaseDirectory(), next)));
        } else if (*next == "offset"){
            events.push_back(Util::ReferenceCount<AnimationEvent>(new OffsetEvent(next)));
        }
    }

    current = events.begin();
}
    
Animation::Animation(const Animation & animation){
    copy(animation);
}

Animation & Animation::operator=(const Animation & animation){
    copy(animation);
    return *this;
}
    
bool Animation::isDone() const {
    return counter == 0 && current == events.end();
}

bool Animation::operator==(const Animation & who) const {
    return id == who.id;
}
    
bool Animation::operator!=(const Animation & who) const {
    return !(*this == who);
}

void Animation::copy(const Animation & animation){
    x = animation.x;
    y = animation.y;
    id = animation.id;
    frame = animation.frame;
    delay = animation.delay;
    loop = animation.loop;
    counter = 0;
    events = animation.events;
    current = events.begin();
    act();
}

void Animation::act(){
    if (counter > 0){
        counter -= 1;
    } else {
        if (current == events.end() && !loop){
        } else {
            if (current == events.end()){
                if (loop){
                    current = events.begin();
                } else {
                    return;
                }
            }
            do{
                (*current)->invoke(*this);
                current++;
                if (current == events.end()){
                    if (loop){
                        current = events.begin();
                    } else {
                        break;
                    }
                }
            } while (counter == 0);
        }
    }
}
    
void Animation::draw(const Graphics::Bitmap & work, int x, int y, bool faceRight){
    int trueX = x + this->x - frame.getWidth() / 2;
    int trueY = y + this->y - frame.getHeight();
    if (faceRight){
        frame.draw(trueX, trueY, work);
    } else {
        frame.drawHFlip(trueX, trueY, work);
    }
}
    
Util::ReferenceCount<Animation> Animation::clone(){
    return Util::ReferenceCount<Animation>(new Animation(*this));
}

void Animation::setLoop(bool what){
    this->loop = what;
}

void Animation::setOffset(int x, int y){
    this->x = x;
    this->y = y;
}

void Animation::setFrame(const Graphics::Bitmap & bitmap){
    this->frame = bitmap;
    counter = delay;
}

void Animation::setDelay(int delay){
    this->delay = delay;
}

const Filesystem::AbsolutePath & Animation::getBaseDirectory() const {
    return baseDirectory;
}
    
void Animation::setBaseDirectory(const Filesystem::AbsolutePath & path){
    this->baseDirectory = path;
}

Animation::~Animation(){
}

Util::ReferenceCount<AnimationManager> AnimationManager::manager; 
Util::ReferenceCount<AnimationManager> AnimationManager::instance(){
    if (manager == NULL){
        manager = Util::ReferenceCount<AnimationManager>(new AnimationManager());
    }

    return manager;
}
    
AnimationManager::AnimationManager():
id(0){
}
    
AnimationManager::~AnimationManager(){
}

map<string, Util::ReferenceCount<Animation> > AnimationManager::loadAnimations(const std::string & path){
    TokenReader reader;
    Filesystem::AbsolutePath directory = Storage::instance().find(Filesystem::RelativePath(path));
    Token * token = reader.readTokenFromFile(directory.join(Filesystem::RelativePath(path + ".txt")).path());
    
    map<string, Util::ReferenceCount<Animation> > animations;

    vector<const Token*> data = token->findTokens("_/anim");
    for (vector<const Token*>::iterator it = data.begin(); it != data.end(); it++){
        const Token * animationToken = *it;
        string name;
        if (animationToken->match("_/name", name)){
            animations[name] = new Animation(directory, animationToken, id);
            id += 1;
        }
    }

    return animations;
}
    
Util::ReferenceCount<Animation> AnimationManager::getAnimation(const std::string & path, const std::string & animation){
    if (sets.find(path) == sets.end()){
        sets[path] = loadAnimations(path);
    }

    return sets[path][animation];
}
    
void AnimationManager::destroy(){
    manager = NULL;
}

}

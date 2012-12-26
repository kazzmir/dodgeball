#include "world.h"
#include "util/graphics/bitmap.h"
#include "util/input/input-manager.h"
#include "util/funcs.h"
#include "util/font.h"
#include "util/sound/sound.h"

#include <sstream>
#include <math.h>

using std::vector;
using std::string;

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

        if (player.getZ() <= 0){
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
    wantY(0){
    }

    int wait;
    int wantX;
    int wantY;

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
        if (wait > 0){
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
                        if (wantX == 0 || wantY == 0 || Util::rnd(150) == 0){
                            wantX = Util::rnd(player.getLimit().x1, player.getLimit().x2);
                            wantY = Util::rnd(player.getLimit().y1, player.getLimit().y2);
                        } else if (Util::rnd(100) == 0){
                            player.doCatch();
                        }
                        if (Util::distance(player.getX(), player.getY(), wantX, wantY) > player.walkingSpeed()){
                            moveTowards(player, wantX, wantY);
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
    }

    void setControl(bool what){
    }

    bool hasControl() const {
        return true;
    }
};

Player::Player(double x, double y, const Graphics::Color & color, const Box & box, const Util::ReferenceCount<Behavior> & behavior, bool sideline):
x(x),
y(y),
z(0),
velocityX(0),
velocityY(0),
velocityZ(0),
hasBall_(false),
facing(FaceRight),
limit(box),
color(color),
sideline(sideline),
catching(0),
forceMove(false),
wantX(0),
wantY(0),
behavior(behavior){
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

void Player::act(World & world){
    if (catching > 0){
        catching -= 1;
        if (catching == 0){
            behavior->resetInput();
        }
    }

    if (forceMove && onGround()){
        if (Util::distance(getX(), getY(), wantX, wantY) < 3){
            forceMove = false;
            behavior->resetInput();
        } else {
            double angle = atan2(wantY - getY(), wantX - getX());
            velocityX = cos(angle) * walkingSpeed();
            velocityY = sin(angle) * walkingSpeed();
        }
    } else {
        if (catching == 0){
            behavior->act(world, *this);
        }
    }

    if (z > 0){
        velocityZ -= gravity;
    } else {
        z = 0;
        velocityZ = 0;

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
    }

    x += velocityX;
    y += velocityY;
    z += velocityZ;

    if (!forceMove){
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
    
void Player::collided(Ball & ball){
    velocityX = 8;
    if (ball.getVelocityX() < 0){
        velocityX *= -1;
    }
    velocityY = 0;
    velocityZ = 9;
    z = 0.1;
}
    
double Player::getWidth() const {
    return 30;
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
    velocityZ = jumpVelocity;
    /* set the z to some initial value above 0 so that it doesn't look like we
     * are hitting the ground.
     */
    z = 0.1;
}

double findAngle(double x1, double y1, double x2, double y2){
    return atan2(y2 - y1, x2 - x1);
}

void Player::throwBall(World & world, Ball & ball){
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
    
void Player::doCatch(int time){
    catching = time;
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

void Player::grabBall(Ball & ball){
    behavior->gotBall(ball);
    catching = 0;
    ball.grab(this);
    hasBall_ = true;
}

void Player::moveLeft(double speed){
    this->velocityX = -speed;
    // this->x -= speed;
}

void Player::moveRight(double speed){
    this->velocityX = speed;
    // this->x += speed;
}
    
double Player::maxRunSpeed = 9;
    
void Player::runRight(double speed){
    this->velocityX += speed;
    if (this->velocityX > maxRunSpeed){
        this->velocityX = maxRunSpeed;
    }
}

void Player::runLeft(double speed){
    this->velocityX -= speed;
    if (this->velocityX < -maxRunSpeed){
        this->velocityX = -maxRunSpeed;
    }
}

void Player::moveUp(double speed){
    this->velocityY = -speed;
    // this->y -= speed;
}

void Player::moveDown(double speed){
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

void Player::draw(const Graphics::Bitmap & work, const Camera & camera){
    int width = getWidth();
    int height = getHeight();
    work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 10, 5, Graphics::makeColor(32, 32, 32));

    double facingX = cos(Util::radians(getFacingAngle())) * 40;
    double facingY = -sin(Util::radians(getFacingAngle())) * 40;
    work.line((int) camera.computeX(x), (int) camera.computeY(y),
              (int) camera.computeX(x + facingX), (int) camera.computeY(y + facingY),
              Graphics::makeColor(255, 0, 0));

    if (hasControl()){
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 20, 10, Graphics::makeColor(0, 0, 255));
    }
    if (hasBall()){
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 20, 10, Graphics::makeColor(255, 255, 0));
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 21, 11, Graphics::makeColor(255, 255, 0));
    }
    work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y - z - height / 2), width / 2, height / 2, color);

    int handx = camera.computeX(x + 10);
    int handy = camera.computeY(y - getHandPosition());
    Graphics::Color handColor = Graphics::makeColor(255, 255, 255);
    if (isCatching()){
        handColor = Graphics::makeColor(255, 255, 0);
    }
    work.ellipseFill(handx, handy, 16, 6, handColor);

    work.circleFill((int) camera.computeX(x + 3), (int) camera.computeY(y - z - height * 3 / 4), 5, Graphics::makeColor(255, 255, 255));
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
            if (player->isCatching()){
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

static Util::ReferenceCount<Player> makePlayer(double x, double y, const Graphics::Color & color, const Box & box, const Util::ReferenceCount<Behavior> & behavior, bool sideline){
    return Util::ReferenceCount<Player>(new Player(x, y, color, box, behavior, sideline));
}

void Team::populateLeft(const Field & field){
    map.set(Keyboard::Key_Q, Cycle);
    double width = field.getWidth() / 2;
    double height = field.getHeight();
    Graphics::Color color(Graphics::makeColor(255, 0, 0));
    players.push_back(makePlayer(width / 5, height / 2, color, Box(0, 0, width, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), false));
    players.push_back(makePlayer(width / 2, height / 4, color, Box(0, 0, width, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), false));
    players.push_back(makePlayer(width / 2, height * 3 / 4, color, Box(0, 0, width, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), false));

    players.push_back(makePlayer(field.getWidth() - 0, height / 2, color, Box(field.getWidth() - 0, 0, field.getWidth() - 0, height), Util::ReferenceCount<Behavior>(new HumanBehavior()), true));
    players.push_back(makePlayer(field.getWidth() - width / 2, -10, color, Box(field.getWidth() - width, -10, field.getWidth(), -10), Util::ReferenceCount<Behavior>(new HumanBehavior()), true));
    players.push_back(makePlayer(field.getWidth() - width / 2, height + 10, color, Box(field.getWidth() - width, height + 10, field.getWidth(), height + 10), Util::ReferenceCount<Behavior>(new HumanBehavior()), true));
}

void Team::populateRight(const Field & field){
    double width = field.getWidth() / 2;
    double height = field.getHeight();
    Graphics::Color color(Graphics::makeColor(0x00, 0xaf, 0x64));
    players.push_back(makePlayer(field.getWidth() - width / 5, height / 2, color, Box(field.getWidth() - width, 0, field.getWidth(), height), Util::ReferenceCount<Behavior>(new AIBehavior()), false));
    players.push_back(makePlayer(field.getWidth() - width / 2, height / 4, color, Box(field.getWidth() - width, 0, field.getWidth(), height), Util::ReferenceCount<Behavior>(new AIBehavior()), false));
    players.push_back(makePlayer(field.getWidth() - width / 2, height * 3 / 4, color, Box(field.getWidth() - width, 0, field.getWidth(), height), Util::ReferenceCount<Behavior>(new AIBehavior()), false));

    players.push_back(makePlayer(-10, height / 2, color, Box(-10, 0, -10, height), Util::ReferenceCount<Behavior>(new AIBehavior()), true));
    players.push_back(makePlayer(width / 2, -10, color, Box(0, -10, width, -10), Util::ReferenceCount<Behavior>(new AIBehavior()), true));
    players.push_back(makePlayer(width / 2, height + 10, color, Box(0, height + 10, width, height + 10), Util::ReferenceCount<Behavior>(new AIBehavior()), true));
}

void Team::enableControl(){
    if (players.size() > 0){
        players[0]->setControl(true);
    }
}

void Team::cycleControl(World & world){
    Util::ReferenceCount<Player> use(NULL);
    double best = 9999;

    const Ball & ball = world.getBall();

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
    return power;
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
        this->y = holder->getY();
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

    work.ellipseFill(camera.computeX(x + size / 2), camera.computeY(y),
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

    team1.enableControl();
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

void World::drawPlayers(const Graphics::Bitmap & work){
    team1.draw(work, camera);
    team2.draw(work, camera);
}

void World::drawText(const Graphics::Bitmap & work, const Camera & camera){
    for (vector<Util::ReferenceCount<FloatingText> >::iterator it = floatingText.begin(); it != floatingText.end(); it++){
        Util::ReferenceCount<FloatingText> text = *it;
        text->draw(work, camera);
    }
}

void World::draw(const Graphics::Bitmap & screen){
    Graphics::StretchedBitmap work(camera.getWidth(), camera.getHeight(), screen);
    work.start();

    field.draw(work, camera);
    drawPlayers(work);
    ball.draw(work, camera);
    drawText(work, camera);

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
    for (vector<Util::ReferenceCount<Player> >::const_iterator it = team.getPlayers().begin(); it != team.getPlayers().end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        if (player == &who){
            return true;
        }
    }
    return false;
}

Util::ReferenceCount<Player> World::getTarget(const vector<Util::ReferenceCount<Player> > & players, Player & who){
    Util::ReferenceCount<Player> best(NULL);
    double bestAngle = 999;

    for (vector<Util::ReferenceCount<Player> >::const_iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        /* can't target sidelined players */
        if (!player->onSideline()){
            /* choose the player closest to angle 0, directly horizontal */
            double angle = atan2(player->getY() - who.getY(), player->getX() - who.getX());
            if (fabs(angle) < bestAngle){
                bestAngle = angle;
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
    
Util::ReferenceCount<Player> World::passTarget(const std::vector<Util::ReferenceCount<Player> > & players, Player & who){
    Util::ReferenceCount<Player> best(NULL);
    double closest = 9999;

    for (vector<Util::ReferenceCount<Player> >::const_iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        if (player != &who){
            double distance = Util::distance(player->getX(), player->getY(), who.getX(), who.getY());
            if (distance < closest){
                closest = distance;
                best = player;
            }
        }
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

}
